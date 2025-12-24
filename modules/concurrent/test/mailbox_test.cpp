// SPDX-License-Identifier: Apache-2.0

#include <gtest/gtest.h>

#include <array>
#include <latch>
#include <mutex>
#include <simcoe/concurrent/mailbox.hpp>
#include <thread>

using namespace std::chrono_literals;

class MailboxTest : public testing::Test {};

TEST_F(MailboxTest, LargeData) {
    static constexpr size_t kArraySize = 0x10000;
    using BigArray = std::array<uint8_t, kArraySize>;
    using BigArrayMailbox = sm::concurrent::AtomicMailbox<BigArray>;

    // a little too big for the stack
    auto ptr = std::make_unique<BigArrayMailbox>();

    {
        std::latch latch{1};

        std::jthread reader = std::jthread([ptr = ptr.get(), &latch](const std::stop_token& stop) {
            latch.wait();

            while (!stop.stop_requested()) {
                std::lock_guard guard(*ptr);

                const BigArray& data = ptr->read();
                uint8_t first = data[0];
                uint8_t last = data[kArraySize - 1];

                ASSERT_NE(first, 0);
                ASSERT_NE(last, 0);
                ASSERT_EQ(first, last);
            }
        });

        std::jthread writer = std::jthread([ptr = ptr.get(), &latch](const std::stop_token& stop) {
            uint8_t value = 1;
            std::once_flag once;
            while (!stop.stop_requested()) {
                BigArray data;
                uint8_t next = value++;
                if (next == 0) {
                    value = 1;
                    next = 1;
                }

                data.fill(next);

                ptr->write(data);

                std::call_once(once, [&] { latch.count_down(); });
            }
        });

        std::this_thread::sleep_for(3s);
    }

    SUCCEED() << "No assertions or torn reads";
}
