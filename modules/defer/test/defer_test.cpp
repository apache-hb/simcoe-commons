#define SM_DEFER_AS_KEYWORD 1
#include <gtest/gtest.h>

#include <simcoe/defer/defer.hpp>

class DeferTest : public testing::Test {};

TEST_F(DeferTest, BasicDefer) {
    int x = 0;

    {
        defer {
            x += 1;
        };
        EXPECT_EQ(x, 0);
    }

    EXPECT_EQ(x, 1);
}

TEST_F(DeferTest, MultipleDefers) {
    int x = 0;

    {
        defer {
            x += 1;
        };
        defer {
            x += 2;
        };
        defer {
            x += 3;
        };
        EXPECT_EQ(x, 0);
    }

    EXPECT_EQ(x, 6);
}

#if __cpp_exceptions >= 199711L
TEST_F(DeferTest, DeferOnException) {
    int x = 0;

    try {
        defer {
            x += 1;
        };
        throw std::runtime_error("Test");
    } catch (...) {
    }

    EXPECT_EQ(x, 1);
}

TEST_F(DeferTest, NoDeferOnNoException) {
    int x = 0;

    {
        errdefer {
            x += 1;
        };
    }

    EXPECT_EQ(x, 0);
}

TEST_F(DeferTest, MultipleErrDefers) {
    int x = 0;

    try {
        errdefer {
            x += 1;
        };
        errdefer {
            x += 2;
        };
        errdefer {
            x += 3;
        };
        throw std::runtime_error("Test");
    } catch (...) {
    }

    EXPECT_EQ(x, 6);
}

#endif
