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
#include <simcoe/concurrent/detail/ring_buffer_bitset_detail.hpp>
#include <type_traits>

namespace sm::concurrent {
/**
 * @brief A fixed size, multi-producer single-consumer, wait free, reentrant atomic ringbuffer.
 *
 * This ring buffer is reentrant unlike existing implementations because it must be safe to produce elements when
 * in an interrupt handler as well as in a normal execution context. This complicates its design as it is
 * not possible to implement a reentrant ring buffer without the elements in the ringbuffer being atomic
 * themselves. This means that this ring buffer also contains an atomic bitmap allocator to store its
 * contents.
 *
 * @tparam T The type of elements stored in the ring buffer. Must be MoveAssignable and MoveConstructible.
 * @tparam Allocator The allocator type used to allocate and deallocate memory for the ring buffer.
 *
 * @cite FreeBSDRingBuffer FreeBSD ring_buf implementation
 * @cite WaitFreeMpScQueue waitfree-mpsc-queue
 */
template <typename T, typename Allocator = std::allocator<T>>
#if __cpp_concepts >= 201907L
    requires std::is_nothrow_move_constructible_v<T> && std::is_nothrow_move_assignable_v<T> && std::is_nothrow_destructible_v<T> && std::is_nothrow_default_constructible_v<Allocator>
#endif
class RingBuffer {
public:
    using size_type = uint32_t;
    using value_type = T;
    using allocator_type = Allocator;

public:
    struct alignas(alignof(T)) Storage {
        std::byte data[sizeof(T)];
    };

    using BitsetWord = detail::BitsetWord;
    using ElementIndex = detail::ElementIndex;

    using StorageAllocator = std::allocator_traits<Allocator>::template rebind_alloc<Storage>;

    [[no_unique_address]] StorageAllocator mAllocator{};

    std::byte* mStorage{};

    // The capacity of the ring buffer + 1.
    size_type mCapacity{};

    std::atomic<size_type> mCount{};

    std::atomic<size_type> mHead{};
    std::atomic<size_type> mTail{};

    static constexpr size_t storageOffsetForBitset(size_type capacity) noexcept SM_CLANG_NONBLOCKING SM_CLANG_REENTRANT {
        size_t offset = sizeof(T) * (capacity + 1);
        offset = detail::roundup(offset, alignof(BitsetWord));
        return offset;
    }

    static constexpr size_t storageOffsetForElements(size_type capacity) noexcept SM_CLANG_NONBLOCKING SM_CLANG_REENTRANT {
        size_t offset = storageOffsetForBitset(capacity);
        offset += sizeof(BitsetWord) * detail::requiredBitsetSize(capacity);
        offset = detail::roundup(offset, alignof(ElementIndex));
        return offset;
    }

    T* getStorageAddress() noexcept SM_CLANG_NONBLOCKING SM_CLANG_REENTRANT {
        return reinterpret_cast<T*>(mStorage);
    }

    std::atomic<uint64_t>* getBitsetAddress() noexcept SM_CLANG_NONBLOCKING SM_CLANG_REENTRANT {
        return reinterpret_cast<std::atomic<uint64_t>*>(mStorage + storageOffsetForBitset(capacity()));
    }

    std::atomic<size_type>* getElementAddress() noexcept SM_CLANG_NONBLOCKING SM_CLANG_REENTRANT {
        return reinterpret_cast<std::atomic<size_type>*>(mStorage + storageOffsetForElements(capacity()));
    }

    T& getElementAt(size_type index) noexcept SM_CLANG_NONBLOCKING SM_CLANG_REENTRANT {
        T* storage = getStorageAddress();
        return storage[index];
    }

    void clear() noexcept {
        if (auto storage = getStorageAddress()) {
            //
            // Destroy the remaining elements in the buffer
            //
            constexpr size_t kBitsPerElement = std::numeric_limits<uint64_t>::digits;

            auto bitset = getBitsetAddress();
            for (size_t i = 0; i < (capacity() / kBitsPerElement); i++) {
                uint64_t word = bitset[i].load();
                for (size_t bit = 0; bit < kBitsPerElement; bit++) {
                    size_t index = i * kBitsPerElement + bit;
                    if (index >= capacity()) {
                        break;
                    }

                    if (word & (uint64_t{1} << bit)) {
                        std::destroy_at(&storage[index]);
                    }
                }
            }

            mAllocator.deallocate(reinterpret_cast<Storage*>(mStorage), detail::underlyingStorageElementCount<T>(capacity()));

            mStorage = nullptr;
            mCapacity = 0;
        }
    }

    size_type normalize(size_type index) noexcept SM_CLANG_NONBLOCKING SM_CLANG_REENTRANT {
        return index % capacity();
    }

    size_t allocateElement() noexcept SM_CLANG_NONBLOCKING SM_CLANG_REENTRANT {
        return detail::atomicScanAndSet(getBitsetAddress(), capacity());
    }

    constexpr RingBuffer(std::byte* storage, size_type capacity, Allocator allocator) noexcept
        : mAllocator(std::move(allocator))
        , mStorage(storage)
        , mCapacity(capacity + 1)
        , mCount(0)
        , mHead(0)
        , mTail(0) {}

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
        , mCount(other.mCount.load())
        , mHead(other.mHead.load())
        , mTail(other.mTail.load()) {
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
            mCount.store(other.mCount.load());
            mCount.store(other.mCount.load());
            mHead.store(other.mHead.load());
            mTail.store(other.mTail.load());

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
    bool tryPush(T& value) noexcept SM_CLANG_NONBLOCKING SM_CLANG_REENTRANT {
        auto index = allocateElement();
        if (index == (std::numeric_limits<size_t>::max)()) {
            return false;
        }

        //
        // Optimistic increment of count, if we exceed capacity, roll back.
        //
        auto count = mCount.fetch_add(1);
        if (count >= capacity()) {
            mCount.fetch_sub(1);
            detail::atomicClearBit(getBitsetAddress(), index);
            return false;
        }

        std::construct_at(&getElementAt(index), std::move(value));

        auto head = mHead.fetch_add(1);
        auto elements = getElementAddress();
        elements[normalize(head)].store(index);

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
    bool tryPop(T& value) noexcept SM_CLANG_NONBLOCKING SM_CLANG_REENTRANT {
        auto elements = getElementAddress();
        auto index = elements[normalize(mTail.load())].exchange(std::numeric_limits<size_type>::max());
        if (index == std::numeric_limits<size_type>::max()) {
            return false;
        }

        //
        // Move the value out of storage before we mark the slot as free.
        //
        T& underlying = getElementAt(index);
        value = std::move(underlying);
        std::destroy_at(&underlying);

        detail::atomicClearBit(getBitsetAddress(), index);

        mTail.fetch_add(1);
        mCount.fetch_sub(1);
        return true;
    }

    /**
     * @brief Get an estimate of the number of items in the queue.
     *
     * @warning As this is a lock-free structure the count will be immediately out of date.
     *
     * @param order The memory order to use when loading the count.
     *
     * @return The number of items in the queue.
     */
    size_type count(std::memory_order order = std::memory_order_seq_cst) const noexcept SM_CLANG_NONBLOCKING SM_CLANG_REENTRANT {
        return mCount.load(order);
    }

    /**
     * @brief Get the maximum capacity of the queue.
     *
     * @return The maximum number of items the queue can hold.
     */
    size_type capacity() const noexcept SM_CLANG_NONBLOCKING SM_CLANG_REENTRANT {
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
     * @brief Create a new queue with the given capacity.
     *
     * @param capacity The maximum number of elements the queue can hold.
     * @param allocator The allocator used to allocate and deallocate the storage.
     *
     * @return The ring buffer if it was created successfully, std::nullopt otherwise.
     */
    [[nodiscard]]
    static std::optional<RingBuffer> create(size_type capacity, Allocator allocator = Allocator{}) noexcept {
        if (capacity == 0) {
            return std::nullopt;
        }

        StorageAllocator storageAllocator{allocator};
        size_t elementCount = detail::underlyingStorageElementCount<T>(capacity);
        auto storage = storageAllocator.allocate(elementCount);
        if (storage == nullptr) {
            return std::nullopt;
        }

        BitsetWord* bitset = reinterpret_cast<BitsetWord*>(reinterpret_cast<std::byte*>(storage) + storageOffsetForBitset(capacity));

        ElementIndex* elements = reinterpret_cast<ElementIndex*>(reinterpret_cast<std::byte*>(storage) + storageOffsetForElements(capacity));

        std::uninitialized_fill_n(bitset, detail::requiredBitsetSize(capacity), 0);
        std::uninitialized_fill_n(elements, capacity, std::numeric_limits<size_type>::max());

        return RingBuffer{reinterpret_cast<std::byte*>(storage), capacity, std::move(allocator)};
    }
};
} // namespace sm::concurrent
