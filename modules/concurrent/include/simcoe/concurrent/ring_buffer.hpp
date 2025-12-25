// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <atomic>
#include <cassert>
#include <cstdint>
#include <memory>
#include <optional>
#include <simcoe/concurrent/annotations.hpp>
#include <type_traits>

namespace sm::concurrent {
/**
 * @brief A fixed size, multi-producer, single-consumer reentrant atomic ringbuffer.
 * @cite FreeBSDRingBuffer FreeBSD
 */
template <typename T, typename Allocator = std::allocator<T>>
#if __cpp_concepts >= 201907L
    requires std::is_nothrow_move_constructible_v<T> && std::is_nothrow_move_assignable_v<T> && std::is_nothrow_destructible_v<T> && std::is_nothrow_default_constructible_v<Allocator>
#endif
class RingBuffer {
    Allocator mAllocator{};

    // The storage for the ring buffer, only the range of [mConsumerHead, mProducerHead) is valid initialized, all other slots are uninitialized.
    T* mStorage{nullptr};

    // The capacity of the ring buffer + 1.
    uint32_t mCapacity{0};

    std::atomic<uint32_t> mProducerHead{0};
    std::atomic<uint32_t> mConsumerTail{0};
    std::atomic<uint32_t> mProducerTail{0};
    std::atomic<uint32_t> mConsumerHead{0};

    void clear() noexcept {
        if (mStorage) {
            //
            // Destroy the remaining elements in the buffer
            //
            std::destroy(mStorage + mConsumerHead, mStorage + mProducerHead);
            mAllocator.deallocate(mStorage, mCapacity);
            mStorage = nullptr;
            mCapacity = 0;
        }
    }

    constexpr RingBuffer(T* storage, uint32_t capacity, Allocator allocator) noexcept
        : mAllocator(std::move(allocator))
        , mStorage(storage)
        , mCapacity(capacity + 1)
        , mProducerHead(0)
        , mConsumerTail(0)
        , mProducerTail(0)
        , mConsumerHead(0) {}

public:
    constexpr RingBuffer() noexcept = default;

    RingBuffer(const RingBuffer& other) = delete;
    RingBuffer& operator=(const RingBuffer& other) = delete;

    /**
     * @brief Move construct a ring buffer.
     *
     * @note This function is not thread-safe and should only be called when no other threads are accessing the queue.
     *
     * @param other The other ring buffer to move from.
     */
    constexpr RingBuffer(RingBuffer&& other) noexcept
        : mAllocator(std::move(other.mAllocator))
        , mStorage(other.mStorage)
        , mCapacity(other.mCapacity)
        , mProducerHead(other.mProducerHead.load())
        , mConsumerTail(other.mConsumerTail.load())
        , mProducerTail(other.mProducerTail.load())
        , mConsumerHead(other.mConsumerHead.load()) {
        other.mStorage = nullptr;
        other.mCapacity = 0;
    }

    /**
     * @brief Move assign a ring buffer.
     *
     * @note This function is not thread-safe and should only be called when no other threads are accessing the queue.
     *
     * @param other The other ring buffer to move from.
     * @return The moved ring buffer.
     */
    constexpr RingBuffer& operator=(RingBuffer&& other) noexcept {
        if (this != &other) {
            clear();

            mAllocator = std::move(other.mAllocator);
            mStorage = other.mStorage;
            mCapacity = other.mCapacity;
            mProducerHead.store(other.mProducerHead.load());
            mConsumerTail.store(other.mConsumerTail.load());
            mProducerTail.store(other.mProducerTail.load());
            mConsumerHead.store(other.mConsumerHead.load());

            other.mStorage = nullptr;
            other.mCapacity = 0;
        }
        return *this;
    }

    /**
     * @brief Destroy the ring buffer.
     */
    constexpr ~RingBuffer() noexcept {
        clear();
    }

    /**
     * @brief Try to push a value onto the queue.
     *
     * Attempts to push a value onto the queue. If the queue is full, the value is not pushed and false is returned.
     * If the value is successfully pushed then @p value is moved from, otherwise it is left unchanged.
     *
     * @param value The value to push.
     *
     * @return true if the value was pushed, false if the queue was full.
     */
    bool tryPush(T& value) noexcept SM_CLANG_NONBLOCKING {
        uint32_t producerHead;
        uint32_t producerTail;
        uint32_t producerTailNext;
        uint32_t producerNext;
        uint32_t consumerTail;

        do {
            producerHead = mProducerHead.load();
            consumerTail = mConsumerTail.load();

            producerNext = (producerHead + 1) % mCapacity;
            if (producerNext == consumerTail) {
                return false; // Queue is full
            }
        } while (!mProducerHead.compare_exchange_strong(producerHead, producerNext));

        T& storage = mStorage[producerHead];
        std::construct_at(&storage, std::move(value));

        //
        // Advance the producer tail to the next position, this is implemented differently from what the
        // freebsd ring buffer does because that implementation is not reentrant. This avoids the
        // deadlock that would occur if a producer is interrupted here and then the interrupt service routine
        // also tries to push to the same ring buffer.
        //
        do {
            producerTail = mProducerTail.load();
            producerTailNext = (producerTail + 1) % mCapacity;
        } while (!mProducerTail.compare_exchange_strong(producerTail, producerTailNext));

        return true;
    }

    /**
     * @brief Try to pop a value from the queue.
     *
     * Attempts to pop a value from the queue. If the queue is empty, false is returned and @p value is left unchanged.
     * If a value is successfully popped, it is moved into @p value.
     *
     * @param value The value to pop into.
     *
     * @return true if a value was popped, false if the queue was empty.
     */
    bool tryPop(T& value) noexcept SM_CLANG_NONBLOCKING {
        uint32_t consumerHead = mConsumerHead.load();
        uint32_t producerTail = mProducerTail.load();

        uint32_t consumerNext = (consumerHead + 1) % mCapacity;

        if (consumerHead == producerTail) {
            return false; // Queue is empty
        }

        mConsumerHead.store(consumerNext);

        value = std::move(mStorage[consumerHead]);
        std::destroy_at(&mStorage[consumerHead]);

        mConsumerTail.store(consumerNext);
        return true;
    }

    /**
     * @brief Get an estimate of the number of items in the queue.
     *
     * @warning As this is a lock-free structure the count will be immediately out of date.
     *
     * @return The number of items in the queue.
     */
    uint32_t count() const noexcept SM_CLANG_NONBLOCKING {
        uint32_t producerTail = mProducerTail.load();
        uint32_t consumerTail = mConsumerTail.load();
        return (mCapacity + producerTail - consumerTail) % mCapacity;
    }

    /**
     * @brief Get the maximum capacity of the queue.
     *
     * @return The maximum number of items the queue can hold.
     */
    uint32_t capacity() const noexcept SM_CLANG_NONBLOCKING {
        return mCapacity - 1;
    }

    /**
     * @brief Check if the queue has been setup.
     *
     * @return true if the queue has been setup, false otherwise.
     */
    bool isSetup() const noexcept SM_CLANG_NONBLOCKING {
        return mStorage != nullptr;
    }

    /**
     * @brief Reset the queue to an empty state with the given storage and capacity.
     *
     * @pre @p capacity must be greater than zero.
     * @pre @p storage must point to valid storage of at least @p capacity + 1 elements.
     *
     * The storage must be at least capacity + 1 elements in size. The queue takes ownership of the storage.
     *
     * @note This function is not thread-safe and should only be called when no other threads are accessing the queue.
     *
     * @param storage The storage to use for the queue.
     * @param capacity The maximum number of elements the queue can hold.
     * @param allocator The allocator used to allocate and deallocate the storage.
     */
    void reset(T* storage, uint32_t capacity, Allocator allocator) noexcept {
        clear();

        mAllocator = std::move(allocator);
        mStorage = storage;
        mCapacity = capacity + 1;
        mProducerHead.store(0);
        mProducerTail.store(0);
        mConsumerHead.store(0);
        mConsumerTail.store(0);
    }

    /**
     * @brief Create a new queue with the given capacity.
     *
     * Constructs a new ring buffer with the specified capacity using the provided allocator.
     * Returns an optional containing the created ring buffer on success, or std::nullopt on failure.
     *
     * @pre @p capacity must be greater than zero.
     *
     * @param capacity The maximum number of elements the queue can hold.
     * @param allocator The allocator used to allocate and deallocate the storage.
     *
     * @return The construction result.
     * @retval std::nullopt The queue could not be created.
     */
    [[nodiscard]]
    static std::optional<RingBuffer<T>> create(uint32_t capacity, Allocator allocator = Allocator{}) noexcept {
        assert(capacity > 0);

        T* storage = allocator.allocate(capacity + 1);
        if (storage == nullptr) {
            return std::nullopt;
        }

        return RingBuffer{storage, capacity, std::move(allocator)};
    }
};
} // namespace sm::concurrent
