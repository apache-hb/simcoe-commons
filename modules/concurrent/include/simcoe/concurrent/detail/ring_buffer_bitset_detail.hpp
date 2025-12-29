#pragma once

#include <atomic>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <simcoe/concurrent/annotations.hpp>

namespace sm::concurrent::detail {
constexpr size_t requiredBitsetSize(size_t capacity) noexcept {
    return (capacity + 63) / 64;
}

static_assert(requiredBitsetSize(1) == 1);
static_assert(requiredBitsetSize(64) == 1);
static_assert(requiredBitsetSize(65) == 2);

using size_type = uint32_t;
using BitsetWord = std::atomic<uint64_t>;
using ElementIndex = std::atomic<size_type>;

template <typename T>
constexpr T roundup(T value, T multiple) noexcept {
    return (value + multiple - 1) / multiple * multiple;
}

static_assert(roundup<size_t>(1, 4) == 4);
static_assert(roundup<size_t>(4, 4) == 4);
static_assert(roundup<size_t>(5, 4) == 8);

template <typename T>
constexpr size_t underlyingStorageElementCount(size_type capacity) noexcept {
    size_t size = sizeof(T) * (capacity + 1);

    size = roundup(size, alignof(BitsetWord));
    size += sizeof(BitsetWord) * detail::requiredBitsetSize(capacity);

    size = roundup(size, alignof(ElementIndex));
    size += sizeof(ElementIndex) * capacity;

    size = roundup(size, sizeof(T));
    return (size / sizeof(T));
}

static_assert(underlyingStorageElementCount<uint8_t>(1) == (2 + 6 + 8 + sizeof(size_type)));
static_assert(underlyingStorageElementCount<uint64_t>(1) == (2 + 1 + 1));
static_assert(underlyingStorageElementCount<uint32_t>(1) == 5);

inline size_t atomicScanAndSet(std::atomic<uint64_t>* bits, size_t size) noexcept SM_CLANG_NONBLOCKING SM_CLANG_REENTRANT {
    constexpr size_t kBitsPerElement = std::numeric_limits<uint64_t>::digits;

    for (size_t i = 0; i < requiredBitsetSize(size); i++) {
        uint64_t oldValue = bits[i].load(std::memory_order_acquire);

        for (size_t bit = 0; bit < kBitsPerElement; bit++) {
            if ((i * kBitsPerElement + bit) >= size) {
                break;
            }

            uint64_t mask = uint64_t{1} << bit;
            if ((oldValue & mask) == 0) {
                uint64_t newValue = oldValue | mask;
                if (bits[i].compare_exchange_strong(oldValue, newValue)) {
                    return i * kBitsPerElement + bit;
                } else {
                    //
                    // CAS failed, retry with updated oldValue
                    //
                    bit--;
                }
            }
        }
    }

    return (std::numeric_limits<size_t>::max)();
}

inline void atomicClearBit(std::atomic<uint64_t>* bits, size_t index) noexcept SM_CLANG_NONBLOCKING SM_CLANG_REENTRANT {
    constexpr size_t kBitsPerElement = std::numeric_limits<uint64_t>::digits;

    size_t elementIndex = index / kBitsPerElement;
    size_t bitIndex = index % kBitsPerElement;

    uint64_t mask = ~(uint64_t{1} << bitIndex);
    bits[elementIndex].fetch_and(mask, std::memory_order_acq_rel);
}
} // namespace sm::concurrent::detail