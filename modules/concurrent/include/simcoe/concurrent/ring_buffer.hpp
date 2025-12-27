// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <atomic>
#include <cassert>
#include <cstdint>
#include <cstring>
#include <limits>
#include <memory>
#include <optional>
#include <simcoe/concurrent/annotations.hpp>
#include <type_traits>

namespace sm::concurrent {
/**
 * @brief A fixed size, multi-producer, single-consumer atomic ringbuffer.
 * @cite FreeBSDRingBuffer FreeBSD
 */
template <typename T, typename Allocator = std::allocator<T>>
#if __cpp_concepts >= 201907L
    requires std::is_nothrow_move_constructible_v<T> && std::is_nothrow_move_assignable_v<T> && std::is_nothrow_destructible_v<T> && std::is_nothrow_default_constructible_v<Allocator>
#endif
class RingBuffer {
public:
    using size_type = uint32_t;
    using difference_type = int32_t;
    using value_type = T;
    using allocator_type = Allocator;

public:
    Allocator mAllocator{};

    // The storage for the ring buffer, only the range of [mConsumerHead, mProducerHead) is valid initialized, all other slots are uninitialized.
    T* mStorage{nullptr};

    // The capacity of the ring buffer + 1.
    size_type mCapacity{0};

    std::atomic<difference_type> mFreeSize{0};
    std::atomic<difference_type> mUsedSize{0};

    std::atomic<size_type> mProducerHead{0};
    std::atomic<size_type> mProducerTail{0};

    std::atomic<size_type> mConsumerHead{0};
    std::atomic<size_type> mConsumerTail{0};

    void clear() noexcept {
        if (mStorage) {
            //
            // Destroy the remaining elements in the buffer
            //
            size_type front = mConsumerTail.load();
            size_type back = mProducerHead.load();
            for (size_type i = front; i != back; i = incrementIndex(i)) {
                std::destroy_at(&mStorage[i]);
            }

            mAllocator.deallocate(mStorage, mCapacity);
            mStorage = nullptr;
            mCapacity = 0;
        }
    }

    T& getElement(size_type index) noexcept SM_CLANG_NONBLOCKING {
        assert(index < mCapacity);
        return mStorage[index];
    }

    size_type incrementIndex(size_type index) noexcept SM_CLANG_NONBLOCKING {
        return (index + 1) % mCapacity;
    }

    size_type acquireWriteBlock() {
        while (true) {
            auto oldProducerHead = mProducerHead.load(std::memory_order_acquire);

            auto free = mFreeSize.load();
            if (free < 1) {
                return (std::numeric_limits<size_type>::max)();
            }

            mFreeSize.fetch_sub(1);

            auto newProducerHead = incrementIndex(oldProducerHead);
            if (mProducerHead.compare_exchange_strong(oldProducerHead, newProducerHead)) {
                return oldProducerHead;
            }

            mFreeSize.fetch_add(1);
        }
    }

    void releaseWriteBlock(size_type index) {
        auto next = incrementIndex(index);

        size_type expected;
        do {
            expected = index;
        } while (!mProducerTail.compare_exchange_strong(expected, next));

        mUsedSize.fetch_add(1);
    }

    size_type acquireReadBlock() {
        while (true) {
            auto used = mUsedSize.load(std::memory_order_acquire);
            if (used < 1) {
                return (std::numeric_limits<size_type>::max)();
            }

            mUsedSize.fetch_sub(1);

            auto oldConsumerHead = mConsumerTail.load(std::memory_order_acquire);
            auto newConsumerHead = incrementIndex(oldConsumerHead);
            if (mConsumerTail.compare_exchange_strong(oldConsumerHead, newConsumerHead, std::memory_order_acq_rel)) {
                return oldConsumerHead;
            }

            mUsedSize.fetch_add(1);
        }
    }

    void releaseReadBlock(size_type index) {
        auto next = incrementIndex(index);
        size_type expected;

        do {
            expected = index;
        } while (mConsumerHead.compare_exchange_strong(expected, next));

        mFreeSize.fetch_add(1, std::memory_order_acq_rel);
    }

    constexpr RingBuffer(T* storage, size_type capacity, Allocator allocator) noexcept
        : mAllocator(std::move(allocator))
        , mStorage(storage)
        , mCapacity(capacity + 1)
        , mFreeSize(capacity)
        , mUsedSize(0)
        , mProducerHead(0)
        , mProducerTail(0)
        , mConsumerHead(0)
        , mConsumerTail(0) {}

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
        , mFreeSize(other.mFreeSize.load())
        , mUsedSize(other.mUsedSize.load())
        , mProducerHead(other.mProducerHead.load())
        , mProducerTail(other.mProducerTail.load())
        , mConsumerHead(other.mConsumerHead.load())
        , mConsumerTail(other.mConsumerTail.load()) {
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
            mUsedSize.store(other.mUsedSize.load());
            mFreeSize.store(other.mFreeSize.load());
            mProducerHead.store(other.mProducerHead.load());
            mConsumerHead.store(other.mConsumerHead.load());
            mProducerTail.store(other.mProducerTail.load());
            mConsumerTail.store(other.mConsumerTail.load());

            other.mStorage = nullptr;
            other.mCapacity = 0;
        }
        return *this;
    }

    /**
     * @brief Destroy the ring buffer.
     */
    ~RingBuffer() noexcept {
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
    [[nodiscard]]
    bool tryPush(T& value) noexcept SM_CLANG_NONBLOCKING {
        auto index = acquireWriteBlock();
        if (index == (std::numeric_limits<size_type>::max)()) {
            return false;
        }

        auto& storage = getElement(index);
        std::construct_at(&storage, std::move(value));

        releaseWriteBlock(index);
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
    [[nodiscard]]
    bool tryPop(T& value) noexcept SM_CLANG_NONBLOCKING {
        auto index = acquireReadBlock();
        if (index == (std::numeric_limits<size_type>::max)()) {
            return false;
        }

        T& element = getElement(index);
        value = std::move(element);

        releaseReadBlock(index);
        return true;
    }

    /**
     * @brief Get an estimate of the number of items in the queue.
     *
     * @warning As this is a lock-free structure the count will be immediately out of date.
     *
     * @return The number of items in the queue.
     */
    size_type count() const noexcept SM_CLANG_NONBLOCKING {
        size_type producerTail = mProducerTail.load();
        size_type consumerTail = mConsumerHead.load();
        return (mCapacity + producerTail - consumerTail) % mCapacity;
    }

    /**
     * @brief Get the maximum capacity of the queue.
     *
     * @return The maximum number of items the queue can hold.
     */
    size_type capacity() const noexcept SM_CLANG_NONBLOCKING {
        return mCapacity - 1;
    }

    /**
     * @brief Get the Allocator object used by the ring buffer.
     *
     * @return The allocator.
     */
    allocator_type getAllocator() const noexcept {
        return mAllocator;
    }

    /**
     * @brief Provided for compatibility with standard containers.
     *
     * This is equivalent to `getAllocator()`.
     *
     * @return The allocator.
     */
    allocator_type get_allocator() const noexcept {
        return mAllocator;
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
    void reset(T* storage, size_type capacity, Allocator allocator) noexcept {
        clear();

        mAllocator = std::move(allocator);
        mStorage = storage;
        mCapacity = capacity + 1;
        mFreeSize.store(capacity);
        mUsedSize.store(0);
        mProducerHead.store(0);
        mProducerTail.store(0);
        mConsumerTail.store(0);
        mConsumerHead.store(0);
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
    static std::optional<RingBuffer<T, Allocator>> create(size_type capacity, Allocator allocator = Allocator{}) noexcept {
        assert(capacity > 0);

        T* storage = allocator.allocate(capacity + 1);
        if (storage == nullptr) {
            return std::nullopt;
        }

        return RingBuffer{storage, capacity, std::move(allocator)};
    }
};
} // namespace sm::concurrent
