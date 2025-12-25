// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <atomic>

#if __cpp_concepts >= 201907L
#    include <type_traits>
#endif

#include <simcoe/concurrent/annotations.hpp>

namespace sm::concurrent {
/**
 * @brief Single producer single consumer non-blocking mailbox.
 *
 * Uses two slots to ensure that the reader never blocks.
 * The writer may block if it tries to write while a read is in progress.
 * Requires T to be default initializable and either noexcept move constructible
 * or noexcept copy constructible.
 *
 * @code{.cpp}
 * NonBlockingMailBox<MyData> mailbox;
 *
 * // Writer thread
 * {
 *     mailbox.write(MyData{...});
 * }
 *
 * // Reader thread
 * {
 *     std::lock_guard guard(mailbox);
 *     const MyData& data = mailbox.read();
 *     // process data...
 * }
 * @endcode
 *
 * @tparam T The type of data to be communicated.
 */
template <typename T>
#if __cpp_concepts >= 201907L
    requires std::is_default_constructible_v<T> && std::is_nothrow_move_assignable_v<T>
#endif
class AtomicMailbox {
    static constexpr int kIndexBit = (1 << 0);
    static constexpr int kWriteBit = (1 << 1);

    // clang bug maybe, on linux adding this alignas causes clang to
    // generate all kinds of strange layout info for this class.
    // on windows it works fine, the performance difference is well into
    // placebo territory so i'll leave it out for now.
    // alignas(std::hardware_destructive_interference_size)
    std::atomic<int> mState{};

    T mSlots[2]{};

public:
    constexpr AtomicMailbox() noexcept = default;

    /**
     * @brief Locks the mailbox for reading.
     *
     * @note This function is a no-op and exists to allow usage with std::lock_guard.
     */
    void lock() noexcept SM_CLANG_NONBLOCKING {}

    /**
     * @brief Unlocks the mailbox after reading.
     */
    void unlock() noexcept SM_CLANG_NONBLOCKING {
        mState.fetch_xor(kWriteBit, std::memory_order_release);
    }

    /**
     * @brief Reads the latest data from the mailbox.
     * @note Requires the mailbox to be locked.
     *
     * @return The most recent data.
     */
    const T& read() const noexcept SM_CLANG_NONBLOCKING {
        std::size_t index = !(mState.load(std::memory_order_acquire) & kIndexBit);
        return mSlots[index];
    }

    /**
     * @brief Writes data to the mailbox.
     *
     * @param data The data to write.
     */
    void write(T data) SM_CLANG_BLOCKING {
        int state = 0;
        while ((state = mState.load(std::memory_order_acquire)) & kWriteBit) {
            /* spin */
        }

        int newIndex = (state & kIndexBit);

        mSlots[newIndex] = std::move(data);

        mState.store(state ^ (kIndexBit | kWriteBit), std::memory_order_release);
    }
};
} // namespace sm::concurrent
