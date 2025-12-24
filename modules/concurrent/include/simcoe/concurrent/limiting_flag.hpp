// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <simcoe/concurrent/exports.hpp>

#include <atomic>
#include <chrono>
#include <cstdint>

#if __cpp_concepts >= 201907L
#    include <concepts>
#endif

#include <simcoe/concurrent/annotations.hpp>

namespace sm::concurrent {

#if __cpp_concepts >= 201907L
template <typename T>
concept LimitingFlag = requires(T a) {
    { a.isActive() } -> std::convertible_to<bool>;
};
#endif

/**
 * @brief A limiting flag that activates at most once every specified interval.
 */
class AtMostEvery {
    static constexpr uint64_t kIsSet = (1ull << 63);
    static constexpr uint64_t kTimeMask = ~kIsSet;

    /**
     * @brief The atomic state of the flag.
     * The low 63 bits store the last activation time in 100ns units.
     * The high bit stores whether the flag is currently set or not.
     */
    std::atomic<uint64_t> mLastActive;

    /**
     * @brief The minimum interval between activations.
     */
    std::chrono::nanoseconds mInterval;

public:
    /**
     * @brief Construct a new instance of AtMostEvery.
     *
     * The flag starts inactive and can be activated immediately after construction.
     *
     * @param interval The minimum interval between activations
     */
    constexpr AtMostEvery(std::chrono::nanoseconds interval) noexcept
        : mLastActive(0)
        , mInterval(interval) {}

    /**
     * @brief Attempts to activate the flag.
     *
     * @return If the flag was activated by this call.
     * @retval true if the flag was activated
     * @retval false if the flag was not activated
     */
    SM_CONCURRENT_API bool isActive() noexcept [[SM_CLANG_NONBLOCKING]];
};

#if __cpp_concepts >= 201907L
static_assert(LimitingFlag<AtMostEvery>);
#endif

} // namespace sm::concurrent
