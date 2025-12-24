// SPDX-License-Identifier: Apache-2.0

#include <gtest/gtest.h>

#include <simcoe/concurrent/limiting_flag.hpp>
#include <thread>
#include <vector>

class LimitingFlagTest : public testing::Test {};

TEST_F(LimitingFlagTest, AtMostEveryConcurrent) {
    int activations = 0;

    auto duration = std::chrono::milliseconds(500);
    auto interval = std::chrono::milliseconds(10);
    sm::concurrent::AtMostEvery flag(interval);

    auto thread_count = std::clamp(std::thread::hardware_concurrency(), 2u, 8u);

    auto start = std::chrono::high_resolution_clock::now();
    auto end = start + duration;

    {
        std::vector<std::jthread> threads;
        for (unsigned int i = 0; i < thread_count; i++) {
            threads.emplace_back([&]() {
                while (std::chrono::high_resolution_clock::now() < end) {
                    if (flag.isActive()) {
                        activations++;
                    }
                }
            });
        }
    }

    // Theres a band of ideal values to account for scheduling jitter
    auto ideal = duration / interval;
    auto lower_bound = ideal * 0.8;
    auto upper_bound = ideal * 1.2;

    EXPECT_GE(activations, lower_bound);
    EXPECT_LE(activations, upper_bound);
}

TEST_F(LimitingFlagTest, AtMostEverySingleThread) {
    int activations = 0;

    auto duration = std::chrono::milliseconds(500);
    auto interval = std::chrono::milliseconds(10);
    sm::concurrent::AtMostEvery flag(interval);

    auto start = std::chrono::high_resolution_clock::now();
    auto end = start + duration;
    while (std::chrono::high_resolution_clock::now() < end) {
        if (flag.isActive()) {
            activations++;
        }
    }

    // Theres a band of ideal values to account for scheduling jitter
    auto ideal = duration / interval;
    auto lower_bound = ideal * 0.8;
    auto upper_bound = ideal * 1.2;

    EXPECT_GE(activations, lower_bound);
    EXPECT_LE(activations, upper_bound);
}
