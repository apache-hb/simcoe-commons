#include <gtest/gtest.h>

#include <bitset>
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

public:
    bool shouldAllocate = true;

    TestAllocator() = default;

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

class RingBufferTest : public testing::Test {
public:
    sm::concurrent::RingBuffer<std::string> queue;
    static constexpr size_t kCapacity = 64;

    void SetUp() override {
        queue = sm::concurrent::RingBuffer<std::string>::create(kCapacity).value();
        ASSERT_EQ(queue.capacity(), kCapacity);
        ASSERT_EQ(queue.count(), 0);
    }
};

TEST_F(RingBufferTest, Push) {
    std::string value = "Hello, World!";
    ASSERT_TRUE(queue.tryPush(value));
    ASSERT_EQ(queue.count(), 1);
}

TEST_F(RingBufferTest, Pop) {
    std::string data = "Hello, World!";
    std::string value = data;
    ASSERT_TRUE(queue.tryPush(value));
    ASSERT_EQ(queue.count(), 1);

    std::string poppedValue;
    ASSERT_TRUE(queue.tryPop(poppedValue));
    ASSERT_EQ(poppedValue, data);
    ASSERT_EQ(queue.count(), 0);
}

TEST_F(RingBufferTest, PushFull) {
    for (size_t i = 0; i < kCapacity; ++i) {
        std::string value = "Hello, World!";
        ASSERT_TRUE(queue.tryPush(value));
    }
    ASSERT_EQ(queue.count(), kCapacity);

    std::string value = "This should not be pushed";
    ASSERT_FALSE(queue.tryPush(value));
}

TEST_F(RingBufferTest, PopEmpty) {
    std::string value;
    ASSERT_FALSE(queue.tryPop(value));
    ASSERT_EQ(queue.count(), 0);
}

struct Entry {
    size_t tid;
    size_t index;
};

class RingBufferThreadTest : public testing::Test {
public:
    using RingBuffer = TestRingBuffer<Entry>;
    RingBuffer queue;
    static constexpr size_t kCapacity = 64;
    static constexpr size_t kProducerCount = 8;
    static constexpr size_t kIterations = 1000;

    void SetUp() override {
        queue = RingBuffer::create(kCapacity).value();
        ASSERT_EQ(queue.capacity(), kCapacity);
        ASSERT_EQ(queue.count(), 0);
    }

    std::bitset<kIterations * kProducerCount> sent{};
    std::bitset<kIterations * kProducerCount> received{};
};

TEST_F(RingBufferThreadTest, ThreadSafe) {
    std::vector<std::jthread> producers;
    std::latch latch{kProducerCount + 1};

    std::atomic<size_t> producedCount = 0;
    std::atomic<size_t> consumedCount = 0;
    std::atomic<size_t> droppedCount = 0;

    std::atomic<int> currentSize = 0;

    for (size_t i = 0; i < kProducerCount; ++i) {
        producers.emplace_back([&, tid = i] {
            latch.arrive_and_wait();

            for (size_t j = 0; j < kIterations; ++j) {
                Entry value{tid, j};
                if (queue.tryPush(value)) {
                    sent.set(tid * kIterations + j);

                    producedCount += 1;
                    // !race here with the consumer decrementing, meaning we can't test this value
                    currentSize += 1;
                } else {
                    droppedCount += 1;
                }
            }
        });
    }

    {
        std::jthread consumer([&](std::stop_token stop) {
            latch.arrive_and_wait();

            while (!stop.stop_requested()) {
                Entry value{};
                if (queue.tryPop(value)) {
                    received.set(value.tid * kIterations + value.index);
                    consumedCount += 1;
                    // I'd love to test to ensure currentSize never goes negative here
                    // but that would race with the producers incrementing it
                    // the race is marked with !race in the line above
                    currentSize -= 1;
                }
            }
        });

        producers.clear();
    }

    Entry value;
    while (queue.tryPop(value)) {
        received.set(value.tid * kIterations + value.index);
        consumedCount += 1;
        currentSize -= 1;
    }

    if (received != sent) {
        int limit = 10;
        for (size_t i = 0; i < kIterations * kProducerCount; ++i) {
            if (sent.test(i) && !received.test(i)) {
                size_t tid = i / kIterations;
                size_t index = i % kIterations;
                ADD_FAILURE() << "Missing entry from tid " << tid << " index " << index;

                if (--limit == 0) {
                    break;
                }
            }
        }
    }

    ASSERT_NE(producedCount.load(), 0) << "No items were produced";
    ASSERT_EQ(consumedCount.load(), producedCount.load()) << "Produced items were lost";
    ASSERT_EQ(currentSize.load(), 0) << "Final queue size is not zero";
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
