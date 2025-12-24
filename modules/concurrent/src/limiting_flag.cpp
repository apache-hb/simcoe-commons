#include <simcoe/concurrent/limiting_flag.hpp>

namespace {

constexpr uint64_t kPrecision = 100; // 100ns units

/**
 * @brief Convert time point to interval of 100ns units
 *
 * Time is stored in 100ns units to prevent integer overflow.
 *
 * @param interval The time interval
 * @return uint64_t Time in 100ns units
 */
uint64_t timeInInterval(std::chrono::nanoseconds interval) noexcept [[SM_CLANG_NONBLOCKING]] {
    return static_cast<uint64_t>(interval.count() / kPrecision);
}

std::chrono::nanoseconds currentTime() noexcept [[SM_CLANG_NONBLOCKING]] {
    return std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::high_resolution_clock::now().time_since_epoch());
}

} // namespace

bool sm::concurrent::AtMostEvery::isActive() noexcept [[SM_CLANG_NONBLOCKING]] {
    auto now = currentTime();

    uint64_t current = timeInInterval(now);

    auto atLeast = now - mInterval;
    auto point = timeInInterval(atLeast);
    auto initialValue = mLastActive.load(std::memory_order_acquire);
    auto initial = initialValue & kTimeMask;
    if (initial > point) {
        return false;
    }

    bool toggle = (initialValue & kIsSet) == 0;
    uint64_t nextValue = current | (toggle ? kIsSet : 0);

    return mLastActive.compare_exchange_strong(initialValue, nextValue, std::memory_order_release);
}
