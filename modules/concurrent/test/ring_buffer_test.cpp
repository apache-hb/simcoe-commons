#include <gtest/gtest.h>

#include <algorithm>
#include <latch>
#include <thread>

// strings move assignment isnt nonblocking so clang rightly complains
// this is fine for tests, but in real code only nonblocking move assignable types
// should be allowed in the ring buffer
#if defined(__clang__)
#    pragma clang diagnostic push
#    pragma clang diagnostic ignored "-Wfunction-effects"
#endif

#include <simcoe/concurrent/ring_buffer.hpp>

#if defined(__clang__)
#    pragma clang diagnostic pop
#endif

template <typename T>
class TestAllocator {
    std::allocator<T> mInnerAllocator;

    template <typename U>
    friend class TestAllocator;

public:
    bool shouldAllocate = true;
    using value_type = T;

    TestAllocator() = default;

    template <typename U>
    TestAllocator(const TestAllocator<U>& other) noexcept
        : mInnerAllocator(other.mInnerAllocator)
        , shouldAllocate(other.shouldAllocate) {}

    T* allocate(std::size_t n) noexcept {
        if (shouldAllocate) {
            return mInnerAllocator.allocate(n);
        }

        return nullptr;
    }

    void deallocate(T* ptr, std::size_t n) noexcept {
        mInnerAllocator.deallocate(ptr, n);
    }
};

template <typename T>
using TestRingBuffer = sm::concurrent::RingBuffer<T, TestAllocator<T>>;

class RingBufferConstructTest : public testing::Test {};

TEST_F(RingBufferConstructTest, Construct) {
    sm::concurrent::RingBuffer<int> queue = sm::concurrent::RingBuffer<int>::create(1024).value();
    ASSERT_EQ(queue.capacity(), 1024);
    ASSERT_EQ(queue.count(), 0);
}

TEST_F(RingBufferConstructTest, ConstructString) {
    sm::concurrent::RingBuffer<std::string> queue = sm::concurrent::RingBuffer<std::string>::create(1024).value();
    ASSERT_EQ(queue.capacity(), 1024);
    ASSERT_EQ(queue.count(), 0);
}

TEST_F(RingBufferConstructTest, ConstructFail) {
    TestAllocator<int> allocator;
    allocator.shouldAllocate = false;
    auto result = TestRingBuffer<int>::create(1024, allocator);
    ASSERT_EQ(result.has_value(), false);
}

class RingBufferDetailsTest : public testing::TestWithParam<size_t> {
public:
    std::unique_ptr<std::atomic<uint64_t>[]> bitset;

    void SetUp() override {
        size_t size = getSize();
        bitset = std::make_unique<std::atomic<uint64_t>[]>(size);
        std::uninitialized_fill_n(bitset.get(), size, 0);
    }

    size_t getSize() const {
        return sm::concurrent::detail::requiredBitsetSize(getCapacity());
    }

    size_t getCapacity() const {
        return GetParam();
    }
};

TEST_P(RingBufferDetailsTest, AtomicScanAndSet) {
    std::vector<bool> allocated;
    allocated.resize(getCapacity(), false);

    for (size_t i = 0; i < getCapacity(); i++) {
        size_t index = sm::concurrent::detail::atomicScanAndSet(bitset.get(), getCapacity());

        // we don't care *which* index we get, just that we get a valid one
        ASSERT_NE(index, (std::numeric_limits<size_t>::max)()) << "Failed to allocate at iteration " << i;

        ASSERT_FALSE(allocated[index]) << "Allocated index " << index << " twice";
        allocated[index] = true;
    }

    // now the bitset should be full
    size_t index = sm::concurrent::detail::atomicScanAndSet(bitset.get(), getCapacity());
    ASSERT_EQ(index, (std::numeric_limits<size_t>::max)()) << "Allocated index when full";
}

INSTANTIATE_TEST_SUITE_P(RingBufferDetailsTests, RingBufferDetailsTest, testing::Values(1, 2, 4, 8, 16, 32, 64, 65, 128, 256, 512, 1024));

class RingBufferSizedTest : public testing::TestWithParam<size_t> {
public:
    sm::concurrent::RingBuffer<std::string> queue;

    static void SetUpTestSuite() {
        setbuf(stdout, nullptr);
        setbuf(stderr, nullptr);
    }

    void SetUp() override {
        size_t capacity = GetParam();
        auto result = sm::concurrent::RingBuffer<std::string>::create(capacity);
        ASSERT_TRUE(result.has_value()) << "Failed to create ring buffer with capacity " << capacity;
        queue = std::move(result.value());

        ASSERT_EQ(queue.capacity(), capacity) << "Ring buffer capacity mismatch";
        ASSERT_EQ(queue.count(), 0) << "Ring buffer initial count is not zero";
    }
};

TEST_P(RingBufferSizedTest, Push) {
    std::string value = "Hello, World!";
    ASSERT_TRUE(queue.tryPush(value));
    ASSERT_EQ(queue.count(), 1);
}

TEST_P(RingBufferSizedTest, Pop) {
    std::string data = "Hello, World!";
    std::string value = data;
    ASSERT_TRUE(queue.tryPush(value));
    ASSERT_EQ(queue.count(), 1);

    std::string poppedValue;
    ASSERT_TRUE(queue.tryPop(poppedValue));
    ASSERT_EQ(poppedValue, data);
    ASSERT_EQ(queue.count(), 0);
}

TEST_P(RingBufferSizedTest, PushFull) {
    for (size_t i = 0; i < queue.capacity(); ++i) {
        std::string value = "Hello, World!";
        ASSERT_TRUE(queue.tryPush(value)) << "Failed to push at index " << i;
    }
    ASSERT_EQ(queue.count(), queue.capacity());

    std::string value = "This should not be pushed";
    ASSERT_FALSE(queue.tryPush(value));
}

TEST_P(RingBufferSizedTest, PushInOrder) {
    for (size_t i = 0; i < queue.capacity(); i++) {
        std::string value = "Hello, World! " + std::to_string(i);
        ASSERT_TRUE(queue.tryPush(value)) << "Failed to push at index " << i;
    }
    ASSERT_EQ(queue.count(), queue.capacity());

    for (size_t i = 0; i < queue.capacity(); i++) {
        std::string expected = "Hello, World! " + std::to_string(i);
        std::string value;
        ASSERT_TRUE(queue.tryPop(value));
        ASSERT_EQ(expected, value) << "Unexpected value at index " << i;
    }

    ASSERT_EQ(queue.count(), 0);

    std::string value;
    ASSERT_FALSE(queue.tryPop(value));
}

TEST_P(RingBufferSizedTest, PopEmpty) {
    std::string value;
    ASSERT_FALSE(queue.tryPop(value));
    ASSERT_EQ(queue.count(), 0);
}

INSTANTIATE_TEST_SUITE_P(RingBufferTests, RingBufferSizedTest, testing::Values(1, 2, 4, 8, 16, 32, 64, 128, 256, 512, 1024));

template <typename T, size_t N>
struct MessageValues {
    static constexpr size_t kMaxValues = N;

    std::array<T, kMaxValues> consumed{};
    std::atomic<size_t> consumedIndex{0};

    std::array<T, kMaxValues> produced{};
    std::atomic<size_t> producedIndex{0};

    void recordConsumed(T value) {
        size_t index = consumedIndex.fetch_add(1);
        if (index < kMaxValues) {
            consumed[index] = value;
        }
    }

    void recordProduced(T value) {
        size_t index = producedIndex.fetch_add(1);
        if (index < kMaxValues) {
            produced[index] = value;
        }
    }

    void assertEqual() {
        ASSERT_EQ(producedIndex.load(), consumedIndex.load());

        std::sort(consumed.begin(), consumed.begin() + consumedIndex.load());
        std::sort(produced.begin(), produced.begin() + producedIndex.load());

        for (size_t i = 0; i < consumedIndex.load(); i++) {
            ASSERT_EQ(consumed[i], produced[i]) << "Mismatch at index " << i << " (" << producedIndex.load() << "/" << consumedIndex.load() << ")";
        }
    }
};

class RingBufferThreadTest : public testing::Test {
public:
    using Element = size_t;
    using TestQueue = sm::concurrent::RingBuffer<Element>;

    TestQueue queue;
    static constexpr size_t kCapacity = 1024;
    static constexpr size_t kProducerCount = 8;

    MessageValues<Element, 0x1000 * 4> messageValues;
    std::atomic<Element> nextValue{1};

    void SetUp() override {
        auto result = TestQueue::create(kCapacity);
        ASSERT_TRUE(result.has_value());
        queue = std::move(result.value());
        ASSERT_EQ(queue.capacity(), 1024);
        ASSERT_EQ(queue.count(), 0);
    }

    void recordConsumed(Element value) {
        messageValues.recordConsumed(value);
    }

    void recordProduced(Element value) {
        messageValues.recordProduced(value);
    }
};

TEST_F(RingBufferThreadTest, ThreadSafe) {
    std::vector<std::jthread> producers;
    std::latch latch{kProducerCount + 1};

    std::atomic<size_t> producedCount = 0;
    std::atomic<size_t> consumedCount = 0;
    std::atomic<size_t> droppedCount = 0;

    for (size_t i = 0; i < kProducerCount; ++i) {
        producers.emplace_back([&] {
            latch.arrive_and_wait();

            for (size_t j = 0; j < 1000; ++j) {
                Element value = nextValue.fetch_add(1);
                if (queue.tryPush(value)) {
                    producedCount += 1;
                    recordProduced(value);
                } else {
                    droppedCount += 1;
                }
            }
        });
    }

    {
        std::jthread consumer([&](std::stop_token stop) {
            latch.arrive_and_wait();

            Element value{std::numeric_limits<Element>::max()};
            while (!stop.stop_requested()) {
                if (queue.tryPop(value)) {
                    consumedCount += 1;
                    recordConsumed(value);
                }
            }
        });

        producers.clear();
    }

    Element value{std::numeric_limits<Element>::max()};
    while (queue.tryPop(value)) {
        consumedCount += 1;
        recordConsumed(value);
    }

    {
        auto it = std::find(messageValues.produced.begin(), messageValues.produced.end(), std::numeric_limits<Element>::max());
        ASSERT_EQ(it, messageValues.produced.end()) << "Produced sentinel value found in produced messages at " << std::distance(messageValues.produced.begin(), it);
    }

    {
        auto it = std::find(messageValues.consumed.begin(), messageValues.consumed.end(), std::numeric_limits<Element>::max());
        ASSERT_EQ(it, messageValues.consumed.end()) << "Consumed sentinel value found in consumed messages at " << std::distance(messageValues.consumed.begin(), it);
    }

    messageValues.assertEqual();

    ASSERT_NE(producedCount.load(), 0);
    ASSERT_EQ(consumedCount.load(), producedCount.load());
}

TEST(RingBufferOrderTest, Order) {
    TestRingBuffer<size_t> queue = TestRingBuffer<size_t>::create(64).value();

    for (size_t i = 0; i < 64; i++) {
        size_t value = i * 10;
        ASSERT_TRUE(queue.tryPush(value));
    }

    ASSERT_EQ(queue.count(), 64);
    for (size_t i = 0; i < 64; i++) {
        size_t value;
        ASSERT_TRUE(queue.tryPop(value));
        ASSERT_EQ(value, i * 10) << "Value at index " << i << " is incorrect";
    }
}
