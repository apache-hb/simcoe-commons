#include <gtest/gtest.h>

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

class RingBufferTest : public testing::Test {
public:
    sm::concurrent::RingBuffer<std::string> queue;
    static constexpr size_t kCapacity = 1024;

    void SetUp() override {
        queue = sm::concurrent::RingBuffer<std::string>::create(1024).value();
        ASSERT_EQ(queue.capacity(), 1024);
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

TEST_F(RingBufferTest, ThreadSafe) {
    constexpr size_t kProducerCount = 8;
    std::vector<std::jthread> producers;
    std::latch latch(kProducerCount + 1);

    std::atomic<size_t> producedCount = 0;
    std::atomic<size_t> consumedCount = 0;
    std::atomic<size_t> droppedCount = 0;

    for (size_t i = 0; i < kProducerCount; ++i) {
        producers.emplace_back([&] {
            latch.arrive_and_wait();

            for (size_t j = 0; j < 1000; ++j) {
                std::string value = "Hello, World!";
                if (queue.tryPush(value)) {
                    producedCount += 1;
                } else {
                    droppedCount += 1;
                }
            }
        });
    }

    std::jthread consumer([&](std::stop_token stop) {
        latch.arrive_and_wait();

        std::string value;
        while (!stop.stop_requested()) {
            if (queue.tryPop(value)) {
                consumedCount += 1;
            }
        }
    });

    producers.clear();
    consumer.request_stop();
    consumer.join();

    std::string value;
    while (queue.tryPop(value)) {
        consumedCount += 1;
    }

    ASSERT_NE(producedCount.load(), 0);
    ASSERT_EQ(consumedCount.load(), producedCount.load());
}

TEST(RingBufferOrderTest, Order) {
    sm::concurrent::RingBuffer<size_t> queue = sm::concurrent::RingBuffer<size_t>::create(64).value();

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
