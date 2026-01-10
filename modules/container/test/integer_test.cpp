// SPDX-License-Identifier: Apache-2.0
#include <gtest/gtest.h>

#include <simcoe/container/integer.hpp>

using namespace sm::numeric;

template <typename T>
class AllIntegerUtilsTest : public testing::Test {};

using AllIntegerTypes = testing::Types<int8_t, int16_t, int32_t, int64_t, uint8_t, uint16_t, uint32_t, uint64_t, intptr_t, uintptr_t>;

TYPED_TEST_SUITE(AllIntegerUtilsTest, AllIntegerTypes);

TYPED_TEST(AllIntegerUtilsTest, AddOverflow) {
    TypeParam maximum{std::numeric_limits<TypeParam>::max()};
    TypeParam one{1};
    TypeParam result{};

    // add_overflow should report overflow and wrap the result around
    bool overflows{sm::numeric::add_overflow(maximum, one, result)};
    ASSERT_TRUE(overflows);
    ASSERT_EQ(result, std::numeric_limits<TypeParam>::min());
}

TYPED_TEST(AllIntegerUtilsTest, AddOverflowNoOverflow) {
    TypeParam ten{10};
    TypeParam five{5};
    TypeParam result{};

    // add_overflow should not report overflow and produce the correct result
    bool overflows{sm::numeric::add_overflow(ten, five, result)};
    ASSERT_FALSE(overflows);
    ASSERT_EQ(result, 15);
}

TYPED_TEST(AllIntegerUtilsTest, AddSaturate) {
    TypeParam maximum{std::numeric_limits<TypeParam>::max()};
    TypeParam one{1};

    TypeParam result{sm::numeric::add_saturate(maximum, one)};
    ASSERT_EQ(result, std::numeric_limits<TypeParam>::max());
}

TYPED_TEST(AllIntegerUtilsTest, AddSaturateNoOverflow) {
    TypeParam ten{10};
    TypeParam five{5};

    TypeParam result{sm::numeric::add_saturate(ten, five)};
    ASSERT_EQ(result, 15);
}

TYPED_TEST(AllIntegerUtilsTest, SubSaturate) {
    TypeParam minimum{std::numeric_limits<TypeParam>::min()};
    TypeParam one{1};

    TypeParam result{sm::numeric::sub_saturate(minimum, one)};
    ASSERT_EQ(result, std::numeric_limits<TypeParam>::min());
}

TYPED_TEST(AllIntegerUtilsTest, SubSaturateNoOverflow) {
    TypeParam ten{10};
    TypeParam five{5};

    TypeParam result{sm::numeric::sub_saturate(ten, five)};
    ASSERT_EQ(result, 5);
}

TYPED_TEST(AllIntegerUtilsTest, MulOverflow) {
    TypeParam maximum{std::numeric_limits<TypeParam>::max()};
    TypeParam two{2};
    TypeParam result{};

    // mul_overflow should report overflow and wrap the result around
    bool overflows{sm::numeric::mul_overflow(maximum, two, result)};
    ASSERT_TRUE(overflows);
    ASSERT_EQ(result, static_cast<TypeParam>(-2));
}

TYPED_TEST(AllIntegerUtilsTest, MulOverflowNoOverflow) {
    TypeParam ten{10};
    TypeParam five{5};
    TypeParam result{};

    // mul_overflow should not report overflow and produce the correct result
    bool overflows{sm::numeric::mul_overflow(ten, five, result)};
    ASSERT_FALSE(overflows);
    ASSERT_EQ(result, static_cast<TypeParam>(50));
}

TYPED_TEST(AllIntegerUtilsTest, MulSaturate) {
    TypeParam maximum{std::numeric_limits<TypeParam>::max()};
    TypeParam two{2};

    TypeParam result{sm::numeric::mul_saturate(maximum, two)};
    ASSERT_EQ(result, std::numeric_limits<TypeParam>::max());
}

TYPED_TEST(AllIntegerUtilsTest, MulSaturateNoOverflow) {
    TypeParam ten{10};
    TypeParam five{5};

    TypeParam result{sm::numeric::mul_saturate(ten, five)};
    ASSERT_EQ(result, static_cast<TypeParam>(50));
}

template <typename T>
class SignedIntegerUtilsTest : public testing::Test {};

using SignedIntegerTypes = testing::Types<int8_t, int16_t, int32_t, int64_t, intptr_t>;

TYPED_TEST_SUITE(SignedIntegerUtilsTest, SignedIntegerTypes);

TYPED_TEST(SignedIntegerUtilsTest, AddOverflowNegative) {
    TypeParam minimum{std::numeric_limits<TypeParam>::min()};
    TypeParam one{-1};
    TypeParam result{};

    // add_overflow should report overflow and wrap the result around
    bool overflows{sm::numeric::add_overflow(minimum, one, result)};
    ASSERT_TRUE(overflows);
    ASSERT_EQ(result, std::numeric_limits<TypeParam>::max());
}

TYPED_TEST(SignedIntegerUtilsTest, AddSaturateNegative) {
    TypeParam minimum{std::numeric_limits<TypeParam>::min()};
    TypeParam one{-1};
    TypeParam result{sm::numeric::add_saturate(minimum, one)};

    ASSERT_EQ(result, std::numeric_limits<TypeParam>::min());
}

TYPED_TEST(SignedIntegerUtilsTest, SubSaturatePositive) {
    TypeParam minimum{std::numeric_limits<TypeParam>::min()};
    TypeParam one{1};
    TypeParam result{sm::numeric::sub_saturate(minimum, one)};

    ASSERT_EQ(result, std::numeric_limits<TypeParam>::min());
}

TYPED_TEST(SignedIntegerUtilsTest, SubSaturateNegative) {
    TypeParam minimum{std::numeric_limits<TypeParam>::max()};
    TypeParam one{-1};
    TypeParam result{sm::numeric::sub_saturate(minimum, one)};

    ASSERT_EQ(result, std::numeric_limits<TypeParam>::max());
}

TYPED_TEST(SignedIntegerUtilsTest, MulSaturateNegative) {
    TypeParam minimum{std::numeric_limits<TypeParam>::min()};
    TypeParam two{-2};

    TypeParam result{sm::numeric::mul_saturate(minimum, two)};
    ASSERT_EQ(result, std::numeric_limits<TypeParam>::max());
}

TYPED_TEST(SignedIntegerUtilsTest, MulSaturateInverseSigns) {
    TypeParam minimum{std::numeric_limits<TypeParam>::max()};
    TypeParam two{-2};

    TypeParam result{sm::numeric::mul_saturate(minimum, two)};
    ASSERT_EQ(result, std::numeric_limits<TypeParam>::min());
}

class IntegerUtilsTest : public testing::Test {};

TEST_F(IntegerUtilsTest, ValueFits) {
    // Test various combinations of integral types for value_fits
    ASSERT_TRUE(sm::numeric::value_fits<int8_t>(127));
    ASSERT_FALSE(sm::numeric::value_fits<int8_t>(128));
    ASSERT_TRUE(sm::numeric::value_fits<uint8_t>(255));
    ASSERT_FALSE(sm::numeric::value_fits<uint8_t>(256));
    ASSERT_TRUE(sm::numeric::value_fits<int16_t>(-32768));
    ASSERT_FALSE(sm::numeric::value_fits<int16_t>(32768));
    ASSERT_TRUE(sm::numeric::value_fits<uint16_t>(65535));
    ASSERT_FALSE(sm::numeric::value_fits<uint16_t>(65536));
    ASSERT_TRUE(sm::numeric::value_fits<int32_t>(-2147483648));
    ASSERT_FALSE(sm::numeric::value_fits<int32_t>(2147483648LL));
    ASSERT_TRUE(sm::numeric::value_fits<uint32_t>(4294967295U));
    ASSERT_FALSE(sm::numeric::value_fits<uint32_t>(4294967296ULL));
    ASSERT_TRUE(sm::numeric::value_fits<int64_t>(-9223372036854775807LL - 1));
    ASSERT_FALSE(sm::numeric::value_fits<int64_t>(9223372036854775808ULL));
    ASSERT_TRUE(sm::numeric::value_fits<uint64_t>(18446744073709551615ULL));
}

TEST_F(IntegerUtilsTest, ValueFitsSignedUnsigned) {
    // Signed to unsigned
    ASSERT_TRUE(sm::numeric::value_fits<uint32_t>(0));
    ASSERT_FALSE(sm::numeric::value_fits<uint32_t>(-1));

    // Unsigned to signed
    ASSERT_TRUE(sm::numeric::value_fits<int32_t>(2147483647U));
    ASSERT_FALSE(sm::numeric::value_fits<int32_t>(2147483648U));
}

template <typename T>
class AllIntegerTest : public testing::Test {};

TYPED_TEST_SUITE(AllIntegerTest, AllIntegerTypes);

TYPED_TEST(AllIntegerTest, DefaultConstruct) {
    sm::numeric::Integer<TypeParam> integer{};
    ASSERT_EQ(integer.load(), static_cast<TypeParam>(0));
}

TYPED_TEST(AllIntegerTest, ConstructFromValue) {
    sm::numeric::Integer<TypeParam> integer{42};
    ASSERT_EQ(integer.load(), static_cast<TypeParam>(42));
}

TYPED_TEST(AllIntegerTest, CopyConstruct) {
    sm::numeric::Integer<TypeParam> original{42};
    sm::numeric::Integer<TypeParam> copy{original};
    ASSERT_EQ(copy.load(), static_cast<TypeParam>(42));
}

TYPED_TEST(AllIntegerTest, MoveConstruct) {
    sm::numeric::Integer<TypeParam> original{42};
    sm::numeric::Integer<TypeParam> moved{std::move(original)};
    ASSERT_EQ(moved.load(), static_cast<TypeParam>(42));
}

TYPED_TEST(AllIntegerTest, CopyAssign) {
    sm::numeric::Integer<TypeParam> original{42};
    sm::numeric::Integer<TypeParam> copy{};
    copy = original;
    ASSERT_EQ(copy.load(), static_cast<TypeParam>(42));
}

TYPED_TEST(AllIntegerTest, MoveAssign) {
    sm::numeric::Integer<TypeParam> original{42};
    sm::numeric::Integer<TypeParam> moved{};
    moved = std::move(original);
    ASSERT_EQ(moved.load(), static_cast<TypeParam>(42));
}

TYPED_TEST(AllIntegerTest, StoreTrapNoOverflow) {
    sm::numeric::Integer<TypeParam> integer{};
    integer.store_trap(static_cast<TypeParam>(42));
    ASSERT_EQ(integer.load(), static_cast<TypeParam>(42));
}

struct TestIntegerTraits : public sm::numeric::IntegerTraits {
    [[noreturn]]
    static void enact_trap() noexcept {
        fprintf(stderr, "Test::enact_trap\n");
        std::exit(1);
    }

    [[noreturn]]
    static void enact_inexact() noexcept {
        fprintf(stderr, "Test::enact_inexact\n");
        std::exit(1);
    }
};

class IntegerTest : public testing::Test {
public:
    static void SetUpTestSuite() {
        setvbuf(stderr, nullptr, _IONBF, 0);
    }
};

using IntegerDeathTest = IntegerTest;

TEST_F(IntegerDeathTest, LoadTrapOutOfRange) {
    Integer<int32_t, TestIntegerTraits> integer{9999};
    EXPECT_DEATH((void)integer.load_trap<uint8_t>(), "Test::enact_trap");
}

TEST_F(IntegerTest, LoadTrapInRange) {
    Integer<int32_t, TestIntegerTraits> integer{55};
    uint8_t value = integer.load_trap<uint8_t>();
    ASSERT_EQ(value, static_cast<uint8_t>(55));
}

TEST_F(IntegerDeathTest, LoadExactOutOfRange) {
    Integer<int32_t, TestIntegerTraits> integer{9999};
    EXPECT_DEATH((void)integer.load_exact<uint8_t>(), "Test::enact_inexact");
}

TEST_F(IntegerTest, LoadExactInRange) {
    Integer<int32_t, TestIntegerTraits> integer{100};
    uint8_t value = integer.load_exact<uint8_t>();
    ASSERT_EQ(value, 100);
}

TEST_F(IntegerTest, LoadTruncate) {
    Integer<int32_t, TestIntegerTraits> integer{300};
    uint8_t value = integer.load_truncate<uint8_t>();
    ASSERT_EQ(value, static_cast<uint8_t>(300));
}

TEST_F(IntegerTest, LoadSaturate) {
    Integer<int32_t, TestIntegerTraits> integer{300};
    uint8_t value = integer.load_saturate<uint8_t>();
    ASSERT_EQ(value, std::numeric_limits<uint8_t>::max());
}

TEST_F(IntegerTest, LoadSaturateExact) {
    Integer<int32_t, TestIntegerTraits> integer{100};
    uint8_t value = integer.load_saturate<uint8_t>();
    ASSERT_EQ(value, 100);
}

TEST_F(IntegerTest, LoadSaturateNegative) {
    Integer<int32_t, TestIntegerTraits> integer{-300};
    uint8_t value = integer.load_saturate<uint8_t>();
    ASSERT_EQ(value, std::numeric_limits<uint8_t>::min());
}

TEST_F(IntegerDeathTest, LoadDefaultOutOfRange) {
    Integer<int32_t, TestIntegerTraits> integer{9999};
    EXPECT_DEATH((void)integer.load<uint8_t>(), "Test::enact_trap");
}

TEST_F(IntegerTest, LoadDefaultInRange) {
    Integer<int32_t, TestIntegerTraits> integer{100};
    uint8_t value = integer.load<uint8_t>();
    ASSERT_EQ(value, 100);
}

TEST_F(IntegerTest, LoadDefaultTrapInRange) {
    Integer<int32_t, TestIntegerTraits> integer{100};
    uint8_t value = integer.load<uint8_t>(sm::numeric::overflow_trap);
    ASSERT_EQ(value, 100);
}

TEST_F(IntegerTest, LoadDefaultExactInRange) {
    Integer<int32_t, TestIntegerTraits> integer{100};
    uint8_t value = integer.load<uint8_t>(sm::numeric::overflow_exact);
    ASSERT_EQ(value, 100);
}

TEST_F(IntegerTest, LoadDefaultTruncate) {
    Integer<int32_t, TestIntegerTraits> integer{300};
    uint8_t value = integer.load<uint8_t>(sm::numeric::overflow_wrap);
    ASSERT_EQ(value, static_cast<uint8_t>(300));
}

TEST_F(IntegerTest, LoadDefaultSaturate) {
    Integer<int32_t, TestIntegerTraits> integer{300};
    uint8_t value = integer.load<uint8_t>(sm::numeric::overflow_saturate);
    ASSERT_EQ(value, std::numeric_limits<uint8_t>::max());
}

TEST_F(IntegerTest, StoreTrap) {
    Integer<int8_t, TestIntegerTraits> integer{};
    integer.store_trap(100);
    ASSERT_EQ(integer.load(), 100);
}

TEST_F(IntegerDeathTest, StoreTrapOutOfRange) {
    Integer<int8_t, TestIntegerTraits> integer{};
    EXPECT_DEATH(integer.store_trap(128), "Test::enact_trap");
}

TEST_F(IntegerTest, StoreExact) {
    Integer<int8_t, TestIntegerTraits> integer{};
    integer.store_exact(100);
    ASSERT_EQ(integer.load(), 100);
}

TEST_F(IntegerDeathTest, StoreExactOutOfRange) {
    Integer<int8_t, TestIntegerTraits> integer{};
    EXPECT_DEATH(integer.store_exact(128), "Test::enact_inexact");
}

TEST_F(IntegerTest, StoreTruncate) {
    Integer<int8_t, TestIntegerTraits> integer{};
    integer.store_truncate(128);
    ASSERT_EQ(integer.load(), -128);
}

TEST_F(IntegerTest, StoreSaturate) {
    Integer<int8_t, TestIntegerTraits> integer{};
    integer.store_saturate(128);
    ASSERT_EQ(integer.load(), 127);
}

TEST_F(IntegerTest, StoreSaturateNegative) {
    Integer<int8_t, TestIntegerTraits> integer{};
    integer.store_saturate(-999);
    ASSERT_EQ(integer.load(), std::numeric_limits<int8_t>::min());
}

TEST_F(IntegerTest, StoreSaturateExact) {
    Integer<int8_t, TestIntegerTraits> integer{};
    integer.store_saturate(-100);
    ASSERT_EQ(integer.load(), -100);
}

TEST_F(IntegerDeathTest, StoreDefaultOutOfRange) {
    Integer<int8_t, TestIntegerTraits> integer{};
    EXPECT_DEATH(integer.store(900), "Test::enact_trap");
}

TEST_F(IntegerTest, StoreDefaultInRange) {
    Integer<int8_t, TestIntegerTraits> integer{};
    integer.store(100);
    ASSERT_EQ(integer.load(), 100);
}

TEST_F(IntegerTest, Store) {
    Integer<int8_t, TestIntegerTraits> integer{};
    integer.store(100);
    ASSERT_EQ(integer.load(), 100);

    integer.store(100, overflow_trap);
    ASSERT_EQ(integer.load(), 100);

    integer.store(100, overflow_exact);
    ASSERT_EQ(integer.load(), 100);

    integer.store(128, overflow_saturate);
    ASSERT_EQ(integer.load(), 127);

    integer.store(128, overflow_wrap);
    ASSERT_EQ(integer.load(), -128);
}

TEST_F(IntegerTest, Plus) {
    Integer<int8_t, TestIntegerTraits> integer{100};
    Integer<int8_t, TestIntegerTraits> result{integer.plus(27)};
    ASSERT_EQ(result.load(), 127);
}

TEST_F(IntegerDeathTest, PlusDefaultOverflow) {
    Integer<int8_t, TestIntegerTraits> integer{100};
    EXPECT_DEATH((void)integer.plus(50), "Test::enact_trap");
}

TEST_F(IntegerDeathTest, PlusTrapOverflow) {
    Integer<int8_t, TestIntegerTraits> integer{100};
    EXPECT_DEATH((void)integer.plus_trap(50), "Test::enact_trap");
}

TEST_F(IntegerDeathTest, PlusExactOverflow) {
    Integer<int8_t, TestIntegerTraits> integer{100};
    EXPECT_DEATH((void)integer.plus_exact(50), "Test::enact_inexact");
}

TEST_F(IntegerTest, PlusWrapOverflow) {
    Integer<int8_t, TestIntegerTraits> integer{100};
    Integer<int8_t, TestIntegerTraits> result{integer.plus_wrap(50)};
    ASSERT_EQ(result.load(), -106);
}

TEST_F(IntegerTest, PlusSaturateOverflow) {
    Integer<int8_t, TestIntegerTraits> integer{100};
    Integer<int8_t, TestIntegerTraits> result{integer.plus_saturate(50)};
    ASSERT_EQ(result.load(), 127);
}

TEST_F(IntegerDeathTest, PlusDefaultTrapOverflow) {
    Integer<int8_t, TestIntegerTraits> integer{100};
    EXPECT_DEATH((void)integer.plus(50, overflow_trap), "Test::enact_trap");
}

TEST_F(IntegerDeathTest, PlusDefaultExactOverflow) {
    Integer<int8_t, TestIntegerTraits> integer{100};
    EXPECT_DEATH((void)integer.plus(50, overflow_exact), "Test::enact_inexact");
}

TEST_F(IntegerTest, PlusDefaultWrapOverflow) {
    Integer<int8_t, TestIntegerTraits> integer{100};
    Integer<int8_t, TestIntegerTraits> result{integer.plus(50, overflow_wrap)};
    ASSERT_EQ(result.load(), -106);
}

TEST_F(IntegerTest, PlusDefaultSaturateOverflow) {
    Integer<int8_t, TestIntegerTraits> integer{100};
    Integer<int8_t, TestIntegerTraits> result{integer.plus(50, overflow_saturate)};
    ASSERT_EQ(result.load(), 127);
}

TEST_F(IntegerTest, Minus) {
    Integer<int8_t, TestIntegerTraits> integer{-100};
    Integer<int8_t, TestIntegerTraits> result{integer.minus(27)};
    ASSERT_EQ(result.load(), -127);
}

TEST_F(IntegerDeathTest, MinusDefaultOverflow) {
    Integer<int8_t, TestIntegerTraits> integer{-100};
    EXPECT_DEATH((void)integer.minus(50), "Test::enact_trap");
}

TEST_F(IntegerDeathTest, MinusTrapOverflow) {
    Integer<int8_t, TestIntegerTraits> integer{-100};
    EXPECT_DEATH((void)integer.minus_trap(50), "Test::enact_trap");
}

TEST_F(IntegerDeathTest, MinusExactOverflow) {
    Integer<int8_t, TestIntegerTraits> integer{-100};
    EXPECT_DEATH((void)integer.minus_exact(50), "Test::enact_inexact");
}

TEST_F(IntegerTest, MinusWrapOverflow) {
    Integer<int8_t, TestIntegerTraits> integer{-100};
    Integer<int8_t, TestIntegerTraits> result{integer.minus_wrap(50)};
    ASSERT_EQ(result.load(), 106);
}

TEST_F(IntegerTest, MinusSaturateOverflow) {
    Integer<int8_t, TestIntegerTraits> integer{-100};
    Integer<int8_t, TestIntegerTraits> result{integer.minus_saturate(50)};
    ASSERT_EQ(result.load(), -128);
}

TEST_F(IntegerDeathTest, MinusDefaultTrapOverflow) {
    Integer<int8_t, TestIntegerTraits> integer{-100};
    EXPECT_DEATH((void)integer.minus(50, overflow_trap), "Test::enact_trap");
}

TEST_F(IntegerDeathTest, MinusDefaultExactOverflow) {
    Integer<int8_t, TestIntegerTraits> integer{-100};
    EXPECT_DEATH((void)integer.minus(50, overflow_exact), "Test::enact_inexact");
}

TEST_F(IntegerTest, MinusDefaultWrapOverflow) {
    Integer<int8_t, TestIntegerTraits> integer{-100};
    Integer<int8_t, TestIntegerTraits> result{integer.minus(50, overflow_wrap)};
    ASSERT_EQ(result.load(), 106);
}

TEST_F(IntegerTest, MinusDefaultSaturateOverflow) {
    Integer<int8_t, TestIntegerTraits> integer{-100};
    Integer<int8_t, TestIntegerTraits> result{integer.minus(50, overflow_saturate)};
    ASSERT_EQ(result.load(), -128);
}

TEST_F(IntegerTest, Mul) {
    Integer<int8_t, TestIntegerTraits> integer{10};
    Integer<int8_t, TestIntegerTraits> result{integer.mul(12)};
    ASSERT_EQ(result.load(), 120);
}

TEST_F(IntegerDeathTest, MulDefaultOverflow) {
    Integer<int8_t, TestIntegerTraits> integer{10};
    EXPECT_DEATH((void)integer.mul(13), "Test::enact_trap");
}

TEST_F(IntegerDeathTest, MulTrapOverflow) {
    Integer<int8_t, TestIntegerTraits> integer{10};
    EXPECT_DEATH((void)integer.mul(13, overflow_trap), "Test::enact_trap");
}

TEST_F(IntegerTest, MulExact) {
    Integer<int8_t, TestIntegerTraits> integer{10};
    Integer<int8_t, TestIntegerTraits> result{integer.mul_exact(12)};
    ASSERT_EQ(result.load(), 120);
}

TEST_F(IntegerDeathTest, MulExactOverflow) {
    Integer<int8_t, TestIntegerTraits> integer{10};
    EXPECT_DEATH((void)integer.mul(13, overflow_exact), "Test::enact_inexact");
}

TEST_F(IntegerTest, MulWrapOverflow) {
    Integer<int8_t, TestIntegerTraits> integer{10};
    Integer<int8_t, TestIntegerTraits> result{integer.mul(13, overflow_wrap)};
    ASSERT_EQ(result.load(), -126);
}

TEST_F(IntegerTest, MulSaturateOverflow) {
    Integer<int8_t, TestIntegerTraits> integer{10};
    Integer<int8_t, TestIntegerTraits> result{integer.mul(13, overflow_saturate)};
    ASSERT_EQ(result.load(), 127);
}

TEST_F(IntegerTest, Div) {
    Integer<int8_t, TestIntegerTraits> integer{100};
    Integer<int8_t, TestIntegerTraits> result{integer.div(4)};
    ASSERT_EQ(result.load(), 25);
}

TEST_F(IntegerDeathTest, DivDefaultByZero) {
    Integer<int8_t, TestIntegerTraits> integer{100};
    EXPECT_DEATH((void)integer.div(0), "Test::enact_trap");
}

TEST_F(IntegerTest, DivTrap) {
    Integer<int8_t, TestIntegerTraits> integer{100};
    Integer<int8_t, TestIntegerTraits> result{integer.div(4, overflow_trap)};
    ASSERT_EQ(result.load(), 25);
}

TEST_F(IntegerTest, DivExact) {
    Integer<int8_t, TestIntegerTraits> integer{100};
    Integer<int8_t, TestIntegerTraits> result{integer.div(4, overflow_exact)};
    ASSERT_EQ(result.load(), 25);
}

TEST_F(IntegerTest, DivWrap) {
    Integer<int8_t, TestIntegerTraits> integer{100};
    Integer<int8_t, TestIntegerTraits> result{integer.div(4, overflow_wrap)};
    ASSERT_EQ(result.load(), 25);
}

TEST_F(IntegerTest, DivSaturate) {
    Integer<int8_t, TestIntegerTraits> integer{100};
    Integer<int8_t, TestIntegerTraits> result{integer.div(4, overflow_saturate)};
    ASSERT_EQ(result.load(), 25);
}

TEST_F(IntegerDeathTest, DivTrapByZero) {
    Integer<int8_t, TestIntegerTraits> integer{100};
    EXPECT_DEATH((void)integer.div(0, overflow_trap), "Test::enact_trap");
}

TEST_F(IntegerDeathTest, DivExactByZero) {
    Integer<int8_t, TestIntegerTraits> integer{100};
    EXPECT_DEATH((void)integer.div(0, overflow_exact), "Test::enact_inexact");
}

TEST_F(IntegerTest, DivWrapByZero) {
    Integer<int8_t, TestIntegerTraits> integer{100};
    EXPECT_DEATH((void)integer.div(0, overflow_wrap), "Test::enact_trap");
}

TEST_F(IntegerTest, DivDSaturateByZero) {
    Integer<int8_t, TestIntegerTraits> integer{100};
    EXPECT_DEATH((void)integer.div(0, overflow_saturate), "Test::enact_trap");
}

TEST_F(IntegerTest, DivNegative) {
    Integer<int8_t, TestIntegerTraits> integer{-100};
    Integer<int8_t, TestIntegerTraits> result{integer.div(-4)};
    ASSERT_EQ(result.load(), 25);
}

TEST_F(IntegerTest, DivMixedSign) {
    Integer<int8_t, TestIntegerTraits> integer{-100};
    Integer<int8_t, TestIntegerTraits> result{integer.div(4)};
    ASSERT_EQ(result.load(), -25);
}

TEST_F(IntegerTest, AddTrap) {
    Integer<int8_t, TestIntegerTraits> integer{100};
    integer.add_trap(27);
    ASSERT_EQ(integer.load(), 127);
}

TEST_F(IntegerDeathTest, AddTrapOverflow) {
    Integer<int8_t, TestIntegerTraits> integer{100};
    EXPECT_DEATH(integer.add_trap(50), "Test::enact_trap");
}

TEST_F(IntegerTest, AddExact) {
    Integer<int8_t, TestIntegerTraits> integer{100};
    integer.add_exact(27);
    ASSERT_EQ(integer.load(), 127);
}

TEST_F(IntegerDeathTest, AddExactOverflow) {
    Integer<int8_t, TestIntegerTraits> integer{100};
    EXPECT_DEATH(integer.add_exact(50), "Test::enact_inexact");
}

TEST_F(IntegerTest, AddWrap) {
    Integer<int8_t, TestIntegerTraits> integer{100};
    integer.add_wrap(50);
    ASSERT_EQ(integer.load(), -106);
}

TEST_F(IntegerTest, AddSaturate) {
    Integer<int8_t, TestIntegerTraits> integer{100};
    integer.add_saturate(50);
    ASSERT_EQ(integer.load(), 127);
}

TEST_F(IntegerTest, AddSaturateNegative) {
    Integer<int8_t, TestIntegerTraits> integer{-50};
    integer.add_saturate(-100);
    ASSERT_EQ(integer.load(), -128);
}

TEST_F(IntegerTest, AddDefault) {
    Integer<int8_t, TestIntegerTraits> integer{100};
    integer.add(27);
    ASSERT_EQ(integer.load(), 127);
}

TEST_F(IntegerDeathTest, AddDefaultOverflow) {
    Integer<int8_t, TestIntegerTraits> integer{100};
    EXPECT_DEATH(integer.add(50), "Test::enact_trap");
}

TEST_F(IntegerDeathTest, AddDefaultExactOverflow) {
    Integer<int8_t, TestIntegerTraits> integer{100};
    EXPECT_DEATH(integer.add(50, overflow_exact), "Test::enact_inexact");
}

TEST_F(IntegerTest, AddDefaultWrapOverflow) {
    Integer<int8_t, TestIntegerTraits> integer{100};
    integer.add(50, overflow_wrap);
    ASSERT_EQ(integer.load(), -106);
}

TEST_F(IntegerTest, AddDefaultSaturateOverflow) {
    Integer<int8_t, TestIntegerTraits> integer{100};
    integer.add(50, overflow_saturate);
    ASSERT_EQ(integer.load(), 127);
}

TEST_F(IntegerTest, SubTrap) {
    Integer<int8_t, TestIntegerTraits> integer{-100};
    integer.sub_trap(27);
    ASSERT_EQ(integer.load(), -127);
}

TEST_F(IntegerDeathTest, SubTrapOverflow) {
    Integer<int8_t, TestIntegerTraits> integer{-100};
    EXPECT_DEATH(integer.sub_trap(50), "Test::enact_trap");
}

TEST_F(IntegerTest, SubExact) {
    Integer<int8_t, TestIntegerTraits> integer{-100};
    integer.sub_exact(27);
    ASSERT_EQ(integer.load(), -127);
}

TEST_F(IntegerDeathTest, SubExactOverflow) {
    Integer<int8_t, TestIntegerTraits> integer{-100};
    EXPECT_DEATH(integer.sub_exact(50), "Test::enact_inexact");
}

TEST_F(IntegerTest, SubWrap) {
    Integer<int8_t, TestIntegerTraits> integer{-100};
    integer.sub_wrap(50);
    ASSERT_EQ(integer.load(), 106);
}

TEST_F(IntegerTest, SubSaturate) {
    Integer<int8_t, TestIntegerTraits> integer{-100};
    integer.sub_saturate(50);
    ASSERT_EQ(integer.load(), -128);
}

TEST_F(IntegerTest, SubSaturateNegative) {
    Integer<int8_t, TestIntegerTraits> integer{50};
    integer.sub_saturate(-100);
    ASSERT_EQ(integer.load(), 127);
}

TEST_F(IntegerTest, SubDefault) {
    Integer<int8_t, TestIntegerTraits> integer{-100};
    integer.sub(27);
    ASSERT_EQ(integer.load(), -127);
}

TEST_F(IntegerDeathTest, SubDefaultOverflow) {
    Integer<int8_t, TestIntegerTraits> integer{-100};
    EXPECT_DEATH(integer.sub(50), "Test::enact_trap");
}

TEST_F(IntegerDeathTest, SubDefaultExactOverflow) {
    Integer<int8_t, TestIntegerTraits> integer{-100};
    EXPECT_DEATH(integer.sub(50, overflow_exact), "Test::enact_inexact");
}

TEST_F(IntegerTest, SubDefaultWrapOverflow) {
    Integer<int8_t, TestIntegerTraits> integer{-100};
    integer.sub(50, overflow_wrap);
    ASSERT_EQ(integer.load(), 106);
}

TEST_F(IntegerTest, SubDefaultSaturateOverflow) {
    Integer<int8_t, TestIntegerTraits> integer{-100};
    integer.sub(50, overflow_saturate);
    ASSERT_EQ(integer.load(), -128);
}

TEST_F(IntegerTest, OperatorAssign) {
    Integer<int8_t, TestIntegerTraits> integer{};
    integer = 42;
    ASSERT_EQ(integer.load(), 42);
}

TEST_F(IntegerDeathTest, OperatorAssignOverflow) {
    Integer<int8_t, TestIntegerTraits> integer{};
    EXPECT_DEATH(integer = 128, "Test::enact_trap");
}

TEST_F(IntegerTest, OperatorAdd) {
    Integer<int8_t, TestIntegerTraits> integer{100};
    Integer<int8_t, TestIntegerTraits> result{integer + 27};
    ASSERT_EQ(result.load(), 127);
}

TEST_F(IntegerDeathTest, OperatorAddOverflow) {
    Integer<int8_t, TestIntegerTraits> integer{100};
    EXPECT_DEATH((void)(integer + 50), "Test::enact_trap");
}

TEST_F(IntegerTest, OperatorSub) {
    Integer<int8_t, TestIntegerTraits> integer{-100};
    Integer<int8_t, TestIntegerTraits> result{integer - 27};
    ASSERT_EQ(result.load(), -127);
}

TEST_F(IntegerDeathTest, OperatorSubOverflow) {
    Integer<int8_t, TestIntegerTraits> integer{-100};
    EXPECT_DEATH((void)(integer - 50), "Test::enact_trap");
}

TEST_F(IntegerTest, OperatorMul) {
    Integer<int8_t, TestIntegerTraits> integer{10};
    Integer<int8_t, TestIntegerTraits> result{integer * 12};
    ASSERT_EQ(result.load(), 120);
}

TEST_F(IntegerDeathTest, OperatorMulOverflow) {
    Integer<int8_t, TestIntegerTraits> integer{10};
    EXPECT_DEATH((void)(integer * 13), "Test::enact_trap");
}

TEST_F(IntegerTest, OperatorDiv) {
    Integer<int8_t, TestIntegerTraits> integer{100};
    Integer<int8_t, TestIntegerTraits> result{integer / 4};
    ASSERT_EQ(result.load(), 25);
}

TEST_F(IntegerDeathTest, OperatorDivByZero) {
    Integer<int8_t, TestIntegerTraits> integer{100};
    EXPECT_DEATH((void)(integer / 0), "Test::enact_trap");
}

TEST_F(IntegerTest, OperatorAddAssign) {
    Integer<int8_t, TestIntegerTraits> integer{100};
    integer += 27;
    ASSERT_EQ(integer.load(), 127);
}

TEST_F(IntegerDeathTest, OperatorAddAssignOverflow) {
    Integer<int8_t, TestIntegerTraits> integer{100};
    EXPECT_DEATH(integer += 50, "Test::enact_trap");
}

TEST_F(IntegerTest, OperatorSubAssign) {
    Integer<int8_t, TestIntegerTraits> integer{-100};
    integer -= 27;
    ASSERT_EQ(integer.load(), -127);
}

TEST_F(IntegerDeathTest, OperatorSubAssignOverflow) {
    Integer<int8_t, TestIntegerTraits> integer{-100};
    EXPECT_DEATH(integer -= 50, "Test::enact_trap");
}

TEST_F(IntegerTest, OperatorMulAssign) {
    Integer<int8_t, TestIntegerTraits> integer{10};
    integer *= 12;
    ASSERT_EQ(integer.load(), 120);
}

TEST_F(IntegerDeathTest, OperatorMulAssignOverflow) {
    Integer<int8_t, TestIntegerTraits> integer{10};
    EXPECT_DEATH(integer *= 13, "Test::enact_trap");
}

TEST_F(IntegerTest, OperatorDivAssign) {
    Integer<int8_t, TestIntegerTraits> integer{100};
    integer /= 4;
    ASSERT_EQ(integer.load(), 25);
}

TEST_F(IntegerDeathTest, OperatorDivAssignByZero) {
    Integer<int8_t, TestIntegerTraits> integer{100};
    EXPECT_DEATH(integer /= 0, "Test::enact_trap");
}

TYPED_TEST(AllIntegerTest, OperatorEq) {
    sm::numeric::Integer<TypeParam> integer1{42};
    sm::numeric::Integer<TypeParam> integer2{42};
    sm::numeric::Integer<TypeParam> integer3{43};

    ASSERT_TRUE(integer1 == integer2);
    ASSERT_FALSE(integer1 == integer3);
}

TYPED_TEST(AllIntegerTest, OperatorNeq) {
    sm::numeric::Integer<TypeParam> integer1{42};
    sm::numeric::Integer<TypeParam> integer2{42};
    sm::numeric::Integer<TypeParam> integer3{43};

    ASSERT_FALSE(integer1 != integer2);
    ASSERT_TRUE(integer1 != integer3);
}

TYPED_TEST(AllIntegerTest, OperatorLt) {
    sm::numeric::Integer<TypeParam> integer1{42};
    sm::numeric::Integer<TypeParam> integer2{43};

    ASSERT_TRUE(integer1 < integer2);
    ASSERT_FALSE(integer2 < integer1);
}

TYPED_TEST(AllIntegerTest, OperatorLte) {
    sm::numeric::Integer<TypeParam> integer1{42};
    sm::numeric::Integer<TypeParam> integer2{42};
    sm::numeric::Integer<TypeParam> integer3{43};

    ASSERT_TRUE(integer1 <= integer2);
    ASSERT_TRUE(integer1 <= integer3);
    ASSERT_FALSE(integer3 <= integer1);
}

TYPED_TEST(AllIntegerTest, OperatorGt) {
    sm::numeric::Integer<TypeParam> integer1{43};
    sm::numeric::Integer<TypeParam> integer2{42};

    ASSERT_TRUE(integer1 > integer2);
    ASSERT_FALSE(integer2 > integer1);
}

TYPED_TEST(AllIntegerTest, OperatorGte) {
    sm::numeric::Integer<TypeParam> integer1{43};
    sm::numeric::Integer<TypeParam> integer2{42};
    sm::numeric::Integer<TypeParam> integer3{43};

    ASSERT_TRUE(integer1 >= integer2);
    ASSERT_TRUE(integer1 >= integer3);
    ASSERT_FALSE(integer2 >= integer1);
}

TEST_F(IntegerTest, MixedTypeEquality) {
    Integer<int32_t> int32_integer{100};
    Integer<int64_t> int64_integer{100};
    Integer<uint32_t> uint32_integer{100};
    Integer<uint64_t> uint64_integer{100};

    ASSERT_TRUE(int32_integer == int64_integer);
    ASSERT_TRUE(int32_integer == uint32_integer);
    ASSERT_TRUE(int32_integer == uint64_integer);

    ASSERT_FALSE(int32_integer != int64_integer);
    ASSERT_FALSE(int32_integer != uint32_integer);
    ASSERT_FALSE(int32_integer != uint64_integer);
}

TEST_F(IntegerTest, MixedTypeCompareOverflow) {
    Integer<int8_t> small{100};
    Integer<int32_t> large{10000};

    ASSERT_FALSE(small == large);
    ASSERT_TRUE(small != large);
    ASSERT_TRUE(small < large);
    ASSERT_TRUE(small <= large);
    ASSERT_FALSE(small > large);
    ASSERT_FALSE(small >= large);
}

TEST_F(IntegerTest, MixedTypeCompareOverflowLargeTypes) {
    Integer<int64_t> small{(std::numeric_limits<int64_t>::max)()};
    Integer<uint64_t> large{(std::numeric_limits<uint64_t>::max)()};

    ASSERT_FALSE(small == large);
    ASSERT_TRUE(small != large);
    ASSERT_TRUE(small < large);
    ASSERT_TRUE(small <= large);
    ASSERT_FALSE(small > large);
    ASSERT_FALSE(small >= large);
}
