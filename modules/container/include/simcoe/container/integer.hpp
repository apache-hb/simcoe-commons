// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <cstdint>
#include <cstdlib>
#include <limits>

// Test for std::unreachable added in c++23
#if __cplusplus >= 202302L
#    include <utility>
#endif

#if __cpp_concepts >= 201907L
#    include <concepts>
#endif

#if __STDC_HOSTED__
#    include <iostream>
#endif

namespace sm::numeric {
/**
 * @brief Overflow behaviour for integer options.
 */
enum class OverflowBehaviour {
    /**
     * @brief Assert when an integer operation cannot be represented exactly.
     */
    Trap,

    /**
     * @brief Assume that the result of the integer operation can be represented exactly, behaviour is undefined otherwise.
     */
    Exact,

    /**
     * @brief Perform modular arithmetic on overflow or truncation.
     */
    Wrap,

    /**
     * @brief Saturate to the minimum or maximum representable value on overflow or truncation.
     */
    Saturate,
};

/**
 * @brief Alias for OverflowBehaviour::Trap
 * @see OverflowBehaviour
 */
inline constexpr OverflowBehaviour overflow_trap = OverflowBehaviour::Trap;

/**
 * @brief Alias for OverflowBehaviour::Exact
 * @see OverflowBehaviour
 */
inline constexpr OverflowBehaviour overflow_exact = OverflowBehaviour::Exact;

/**
 * @brief Alias for OverflowBehaviour::Wrap
 * @see OverflowBehaviour
 */
inline constexpr OverflowBehaviour overflow_wrap = OverflowBehaviour::Wrap;

/**
 * @brief Alias for OverflowBehaviour::Saturate
 * @see OverflowBehaviour
 */
inline constexpr OverflowBehaviour overflow_saturate = OverflowBehaviour::Saturate;

namespace detail {
[[noreturn]]
inline void unreachable() noexcept {
#if __cpp_lib_unreachable >= 202202L
    std::unreachable();
#else
    __builtin_unreachable();
#endif
}
} // namespace detail

/**
 * @brief Check if a value can be represented by the target integer type.
 *
 * @code{.cpp}
 * println("300 can fit in int8_t: {}", sm::numeric::value_fits<int8_t>(300)); // false
 * println("30 can fit in int8_t: {}", sm::numeric::value_fits<int8_t>(30)); // true
 * @endcode
 *
 * @tparam T The target integer type.
 * @tparam U The source integer type.
 *
 * @param value The value to check.
 * @return true if the value can be represented by the target type, false otherwise.
 */
template <typename T, typename U>
#if __cpp_concepts >= 201907L
    requires std::integral<T> && std::integral<U>
#endif
constexpr bool value_fits(U value) noexcept {
    if constexpr (std::is_signed_v<T> == std::is_signed_v<U>) {
        return (value >= static_cast<U>(std::numeric_limits<T>::min())) && (value <= static_cast<U>(std::numeric_limits<T>::max()));
    } else if constexpr (std::is_signed_v<T> && !std::is_signed_v<U>) {
        return (value <= static_cast<U>(std::numeric_limits<T>::max()));
    } else {
        // T is unsigned, U is signed
        return (value >= static_cast<U>(0)) && (static_cast<std::make_unsigned_t<U>>(value) <= std::numeric_limits<T>::max());
    }
}

/**
 * @brief Add two integers with overflow detection.
 *
 * If overflow occurs the result is the modular result of the addition.
 * @code{.cpp}
 * int8_t result;
 * bool overflow = sm::numeric::add_overflow<int8_t>(100, 50, result); // overflow == true, result == -106
 * @endcode
 *
 * @tparam T The integer type.
 * @param a The first integer.
 * @param b The second integer.
 * @param[out] result The result of the addition.
 *
 * @return true if overflow occurred, false otherwise.
 */
template <typename T>
#if __cpp_concepts >= 201907L
    requires std::integral<T>
#endif
constexpr bool add_overflow(T a, T b, T& result) noexcept {
    return __builtin_add_overflow(a, b, &result);
}

/**
 * @brief Add two integers with saturation on overflow.
 *
 * If overflow occurs the result is saturated to the maximum or minimum representable value.
 *
 * @code{.cpp}
 * int8_t result = sm::numeric::add_saturate<int8_t>(100, 50); // result == 127
 * @endcode
 *
 * @tparam T The integer type.
 * @param a The first integer.
 * @param b The second integer.
 * @return The result of the addition, saturated on overflow.
 */
template <typename T>
#if __cpp_concepts >= 201907L
    requires std::integral<T>
#endif
constexpr T add_saturate(T a, T b) noexcept {
    T result{};
    if (add_overflow(a, b, result)) {
        if (b > 0) {
            return std::numeric_limits<T>::max();
        } else {
            return std::numeric_limits<T>::min();
        }
    }
    return result;
}

/**
 * @brief Subtract two integers with overflow detection.
 *
 * If overflow occurs the result is the modular result of the subtraction.
 *
 * @code{.cpp}
 * int8_t result;
 * bool overflow = sm::numeric::sub_overflow<int8_t>(-100, 50, result); // overflow == true, result == 106
 * @endcode
 *
 * @tparam T The integer type.
 * @param a The first integer.
 * @param b The second integer.
 * @param[out] result The result of the subtraction.
 * @return true if overflow occurred, false otherwise.
 */
template <typename T>
#if __cpp_concepts >= 201907L
    requires std::integral<T>
#endif
constexpr bool sub_overflow(T a, T b, T& result) noexcept {
    return __builtin_sub_overflow(a, b, &result);
}

/**
 * @brief Subtract two integers with saturation on overflow.
 *
 * If overflow occurs the result is saturated to the maximum or minimum representable value.
 *
 * @code{.cpp}
 * int8_t result = sm::numeric::sub_saturate<int8_t>(-100, 50); // result == -128
 * @endcode
 *
 * @tparam T The integer type.
 * @param a The first integer.
 * @param b The second integer.
 * @return The result of the subtraction, saturated on overflow.
 */
template <typename T>
#if __cpp_concepts >= 201907L
    requires std::integral<T>
#endif
constexpr T sub_saturate(T a, T b) noexcept {
    T result{};
    if (sub_overflow(a, b, result)) {
        if (b > 0) {
            return std::numeric_limits<T>::min();
        } else {
            return std::numeric_limits<T>::max();
        }
    }
    return result;
}

/**
 * @brief Multiply two integers with overflow detection.
 *
 * If overflow occurs the result is the modular result of the multiplication.
 *
 * @code{.cpp}
 * int8_t result;
 * bool overflow = sm::numeric::mul_overflow<int8_t>(100, 30, result); // overflow == true, result == -44
 * @endcode
 *
 * @tparam T The integer type.
 * @param a The first integer.
 * @param b The second integer.
 * @param[out] result The result of the multiplication.
 * @return true if overflow occurred, false otherwise.
 */
template <typename T>
#if __cpp_concepts >= 201907L
    requires std::integral<T>
#endif
constexpr bool mul_overflow(T a, T b, T& result) noexcept {
    return __builtin_mul_overflow(a, b, &result);
}

/**
 * @brief Multiply two integers with saturation on overflow.
 *
 * If overflow occurs the result is saturated to the maximum or minimum representable value.
 *
 * @code{.cpp}
 * int8_t result = sm::numeric::mul_saturate<int8_t>(100, 30); // result == 127
 * @endcode
 *
 * @tparam T The integer type.
 * @param a The first integer.
 * @param b The second integer.
 * @return The result of the multiplication, saturated on overflow.
 */
template <typename T>
#if __cpp_concepts >= 201907L
    requires std::integral<T>
#endif
constexpr T mul_saturate(T a, T b) noexcept {
    T result{};
    if (mul_overflow(a, b, result)) {
        if ((a > 0) == (b > 0)) {
            return std::numeric_limits<T>::max();
        } else {
            return std::numeric_limits<T>::min();
        }
    }
    return result;
}

/**
 * @brief Traits for Integer class to define overflow handling behavior.
 */
struct IntegerTraits {
    /**
     * @brief The function to call when a trap on overflow is needed.
     */
    [[noreturn]]
    static void enact_trap() noexcept = delete;

    /**
     * @brief The function to call when an inexact conversion is detected.
     */
    [[noreturn]]
    static void enact_inexact() noexcept = delete;
};

/**
 * @brief Default integer traits that abort on trap and invoke undefined behavior on inexact.
 */
struct DefaultIntegerTraits : public IntegerTraits {
    /**
     * @brief Abort the program on overflow trap.
     */
    [[noreturn]]
    static void enact_trap() noexcept {
        std::abort();
    }

    /**
     * @brief Invoke undefined behavior on inexact conversion.
     */
    [[noreturn]]
    static void enact_inexact() noexcept {
        detail::unreachable();
    }
};

/**
 * @brief An integer wrapper with configurable overflow handling and checked conversions.
 *
 * This can be used to perform checked integer operations to help prevent UB from signed integer overflow.
 * Perhaps more useful, this integer class can be used to perform unsigned addition with UB on overflow. This can
 * help UBsan catch overflow issues, and can produce better code generation if you do not require the modular arithmetic
 * that unsigned types have by default.
 * For an example of the improved code generation see <a href="https://godbolt.org/z/zqPbYq37n">this Godbolt demo</a>.
 *
 * @tparam T The underlying integer type.
 * @tparam Traits The traits class defining overflow handling behavior.
 */
template <typename T, typename Traits = DefaultIntegerTraits>
#if __cpp_concepts >= 201907L
    requires std::integral<T>
#endif
class Integer {
    T mValue{};

    [[noreturn]]
    void enact_trap() const noexcept {
        Traits::enact_trap();
    }

    [[noreturn]]
    void enact_inexact() const noexcept {
        Traits::enact_inexact();
    }

public:
    /**
     * @brief Construct a new Integer with value 0.
     * @return The constructed Integer.
     */
    [[nodiscard]]
    constexpr Integer() noexcept = default;

    /**
     * @brief Construct a new Integer from a value.
     *
     * @param value The value to initialize the Integer with.
     * @return The constructed Integer.
     */
    [[nodiscard]]
    constexpr Integer(T value) noexcept
        : mValue(value) {}

    /**
     * @brief Load the underlying integer value.
     *
     * @return The underlying integer value.
     */
    [[nodiscard]]
    constexpr T load() const noexcept {
        return mValue;
    }

    /**
     * @brief Test if the underlying value can be represented by type U.
     *
     * @tparam U The target integer type.
     * @return true if the value can be represented by type U,
     * @return false otherwise.
     */
    template <typename U>
    [[nodiscard]]
    constexpr bool value_fits() const noexcept {
        return sm::numeric::value_fits<U>(mValue);
    }

    /**
     * @brief Load the underlying integer value with trap on overflow.
     *
     * @tparam U The target integer type.
     * @return The underlying integer value.
     */
    template <typename U>
    [[nodiscard]]
    constexpr U load_trap() const noexcept {
        if (!value_fits<U>()) {
            enact_trap();
        }
        return static_cast<U>(mValue);
    }

    /**
     * @brief Load the underlying integer value with exact conversion.
     *
     * @tparam U The target integer type.
     * @return The underlying integer value.
     */
    template <typename U>
    [[nodiscard]]
    constexpr U load_exact() const noexcept {
        if (!value_fits<U>()) {
            enact_inexact();
        }
        return static_cast<U>(mValue);
    }

    /**
     * @brief Load the underlying integer value with truncation.
     *
     * @tparam U The target integer type.
     * @return The underlying integer value.
     */
    template <typename U>
    [[nodiscard]]
    constexpr U load_truncate() const noexcept {
        return static_cast<U>(mValue);
    }

    /**
     * @brief Load the underlying integer value, clamping to min/max on overflow.
     *
     * @tparam U The target integer type.
     * @return The underlying integer value.
     */
    template <typename U>
    [[nodiscard]]
    constexpr U load_saturate() const noexcept {
        if (mValue > static_cast<T>(std::numeric_limits<U>::max())) {
            return std::numeric_limits<U>::max();
        } else if (mValue < static_cast<T>(std::numeric_limits<U>::min())) {
            return std::numeric_limits<U>::min();
        } else {
            return static_cast<U>(mValue);
        }
    }

    /**
     * @brief Load the underlying integer value with specified overflow behaviour.
     *
     * @tparam U The target integer type.
     * @param behaviour The overflow behaviour to use.
     * @return The underlying integer value.
     */
    template <typename U = T>
    [[nodiscard]]
    constexpr U load(OverflowBehaviour behaviour = overflow_trap) const noexcept {
        switch (behaviour) {
            case OverflowBehaviour::Trap:
                return load_trap<U>();
            case OverflowBehaviour::Exact:
                return load_exact<U>();
            case OverflowBehaviour::Wrap:
                return load_truncate<U>();
            case OverflowBehaviour::Saturate:
                return load_saturate<U>();
        }

        detail::unreachable();
    }

    /**
     * @brief Store a value into the underlying integer and trap if it cannot be represented.
     *
     * @tparam U The source integer type.
     * @param other The value to store.
     */
    template <typename U>
    constexpr void store_trap(U other) noexcept {
        if (!sm::numeric::value_fits<T>(other)) {
            enact_trap();
        }

        mValue = static_cast<T>(other);
    }

    /**
     * @brief Store a value into the underlying integer and invoke undefined behaviour if it cannot be represented.
     *
     * @tparam U The source integer type.
     * @param other The value to store.
     */
    template <typename U>
    constexpr void store_exact(U other) noexcept {
        if (!sm::numeric::value_fits<T>(other)) {
            enact_inexact();
        }

        mValue = static_cast<T>(other);
    }

    /**
     * @brief Store a value into the underlying integer with truncation.
     *
     * @tparam U The source integer type.
     * @param other The value to store.
     */
    template <typename U>
    constexpr void store_truncate(U other) noexcept {
        mValue = static_cast<T>(other);
    }

    /**
     * @brief Store a value into the underlying integer, clamping to min/max on overflow.
     *
     * @tparam U The source integer type.
     * @param other The value to store.
     */
    template <typename U>
    constexpr void store_saturate(U other) noexcept {
        if (other > static_cast<U>(std::numeric_limits<T>::max())) {
            mValue = std::numeric_limits<T>::max();
        } else if (other < static_cast<U>(std::numeric_limits<T>::min())) {
            mValue = std::numeric_limits<T>::min();
        } else {
            mValue = static_cast<T>(other);
        }
    }

    /**
     * @brief Store a value into the underlying integer with specified overflow behaviour.
     *
     * @tparam U The source integer type.
     * @param other The value to store.
     * @param behaviour The overflow behaviour to use.
     */
    template <typename U>
    constexpr void store(U other, OverflowBehaviour behaviour = overflow_trap) noexcept {
        switch (behaviour) {
            case OverflowBehaviour::Trap:
                store_trap(other);
                break;
            case OverflowBehaviour::Exact:
                store_exact(other);
                break;
            case OverflowBehaviour::Wrap:
                store_truncate(other);
                break;
            case OverflowBehaviour::Saturate:
                store_saturate(other);
                break;
        }
    }

    /**
     * @brief Return a new Integer which is the sum of this and other, with trap on overflow.
     *
     * @param other The value to add.
     * @return The resulting Integer.
     */
    [[nodiscard]]
    constexpr Integer plus_trap(T other) const noexcept {
        Integer result{*this};
        result.add_trap(other);
        return result;
    }

    /**
     * @brief Return a new Integer which is the sum of this and other, invoking undefined behaviour on overflow.
     *
     * @param other The value to add.
     * @return The resulting Integer.
     */
    [[nodiscard]]
    constexpr Integer plus_exact(T other) const noexcept {
        Integer result{*this};
        result.add_exact(other);
        return result;
    }

    /**
     * @brief Return a new Integer which is the sum of this and other, with wrapping on overflow.
     *
     * @param other The value to add.
     * @return The resulting Integer.
     */
    [[nodiscard]]
    constexpr Integer plus_wrap(T other) const noexcept {
        Integer result{*this};
        result.add_wrap(other);
        return result;
    }

    /**
     * @brief Return a new Integer which is the sum of this and other, with saturation on overflow.
     *
     * @param other The value to add.
     * @return The resulting Integer.
     */
    [[nodiscard]]
    constexpr Integer plus_saturate(T other) const noexcept {
        Integer result{*this};
        result.add_saturate(other);
        return result;
    }

    /**
     * @brief Return a new Integer which is the sum of this and other, with specified overflow behaviour.
     *
     * @param other The value to add.
     * @param behaviour The overflow behaviour to use.
     * @return The resulting Integer.
     */
    [[nodiscard]]
    constexpr Integer plus(T other, OverflowBehaviour behaviour = overflow_trap) const noexcept {
        Integer result{*this};
        result.add(other, behaviour);
        return result;
    }

    /**
     * @brief Return a new Integer which is the difference of this and other, with trap on overflow.
     *
     * @param other The value to subtract.
     * @return The resulting Integer.
     */
    [[nodiscard]]
    constexpr Integer minus_trap(T other) const noexcept {
        Integer result{*this};
        result.sub_trap(other);
        return result;
    }

    /**
     * @brief Return a new Integer which is the difference of this and other, invoking undefined behaviour on overflow.
     *
     * @param other The value to subtract.
     * @return The resulting Integer.
     */
    [[nodiscard]]
    constexpr Integer minus_exact(T other) const noexcept {
        Integer result{*this};
        result.sub_exact(other);
        return result;
    }

    /**
     * @brief Return a new Integer which is the difference of this and other, with wrapping on overflow.
     *
     * @param other The value to subtract.
     * @return The resulting Integer.
     */
    [[nodiscard]]
    constexpr Integer minus_wrap(T other) const noexcept {
        Integer result{*this};
        result.sub_wrap(other);
        return result;
    }

    /**
     * @brief Return a new Integer which is the difference of this and other, with saturation on overflow.
     *
     * @param other The value to subtract.
     * @return The resulting Integer.
     */
    [[nodiscard]]
    constexpr Integer minus_saturate(T other) const noexcept {
        Integer result{*this};
        result.sub_saturate(other);
        return result;
    }

    /**
     * @brief Return a new Integer which is the difference of this and other, with specified overflow behaviour.
     *
     * @param other The value to subtract.
     * @param behaviour The overflow behaviour to use.
     * @return The resulting Integer.
     */
    [[nodiscard]]
    constexpr Integer minus(T other, OverflowBehaviour behaviour = overflow_trap) const noexcept {
        Integer result{*this};
        result.sub(other, behaviour);
        return result;
    }

    /**
     * @brief Return a new Integer which is the product of this and other, with trap on overflow.
     *
     * @param other The value to multiply.
     * @return The resulting Integer.
     */
    [[nodiscard]]
    constexpr Integer mul_trap(T other) const noexcept {
        T result{};
        if (mul_overflow(mValue, other, result)) {
            enact_trap();
        }
        return Integer{result};
    }

    /**
     * @brief Return a new Integer which is the product of this and other, invoking undefined behaviour on overflow.
     *
     * @param other The value to multiply.
     * @return The resulting Integer.
     */
    [[nodiscard]]
    constexpr Integer mul_exact(T other) const noexcept {
        T result{};
        if (mul_overflow(mValue, other, result)) {
            enact_inexact();
        }
        return Integer{result};
    }

    /**
     * @brief Return a new Integer which is the product of this and other, with wrapping on overflow.
     *
     * @param other The value to multiply.
     * @return The resulting Integer.
     */
    [[nodiscard]]
    constexpr Integer mul_wrap(T other) const noexcept {
        T result{};
        mul_overflow(mValue, other, result);
        return Integer{result};
    }

    /**
     * @brief Return a new Integer which is the product of this and other, with saturation on overflow.
     *
     * @param other The value to multiply.
     * @return The resulting Integer.
     */
    [[nodiscard]]
    constexpr Integer mul_saturate(T other) const noexcept {
        return Integer{sm::numeric::mul_saturate(mValue, other)};
    }

    /**
     * @brief Return a new Integer which is the product of this and other, with specified overflow behaviour.
     *
     * @param other The value to multiply.
     * @param behaviour The overflow behaviour to use.
     * @return The resulting Integer.
     */
    [[nodiscard]]
    constexpr Integer mul(T other, OverflowBehaviour behaviour = overflow_trap) const noexcept {
        switch (behaviour) {
            case OverflowBehaviour::Trap:
                return mul_trap(other);
            case OverflowBehaviour::Exact:
                return mul_exact(other);
            case OverflowBehaviour::Wrap:
                return mul_wrap(other);
            case OverflowBehaviour::Saturate:
                return mul_saturate(other);
        }

        detail::unreachable();
    }

    /**
     * @brief Return a new Integer which is the quotient of this and other, with trap on division by zero.
     *
     * @param other The value to divide by.
     * @return The resulting Integer.
     */
    [[nodiscard]]
    constexpr Integer div_trap(T other) const noexcept {
        if (other == 0) {
            enact_trap();
        }

        return Integer{static_cast<T>(mValue / other)};
    }

    /**
     * @brief Return a new Integer which is the quotient of this and other, invoking undefined behaviour on division by zero.
     *
     * @param other The value to divide by.
     * @return The resulting Integer.
     */
    [[nodiscard]]
    constexpr Integer div_exact(T other) const noexcept {
        if (other == 0) {
            enact_inexact();
        }

        return Integer{static_cast<T>(mValue / other)};
    }

    /**
     * @brief Return a new Integer which is the quotient of this and other, traps on division by zero.
     *
     * @note This is provided to maintain a consistent interface, but division by zero must always trap.
     *
     * @param other The value to divide by.
     * @return The resulting Integer.
     */
    [[nodiscard]]
    constexpr Integer div_wrap(T other) const noexcept {
        // Division by zero cannot be sensibly defined, so it must always trap
        if (other == 0) {
            enact_trap();
        }

        return Integer{static_cast<T>(mValue / other)};
    }

    /**
     * @brief Return a new Integer which is the quotient of this and other, traps on division by zero.
     *
     * @note This is provided to maintain a consistent interface, but division by zero must always trap.
     *
     * @param other The value to divide by.
     * @return The resulting Integer.
     */
    [[nodiscard]]
    constexpr Integer div_saturate(T other) const noexcept {
        // Theres no saturation possible in division by zero, so it must always trap
        if (other == 0) {
            enact_trap();
        }

        return Integer{static_cast<T>(mValue / other)};
    }

    /**
     * @brief Return a new Integer which is the quotient of this and other, with specified overflow behaviour.
     *
     * @param other The value to divide by.
     * @param behaviour The overflow behaviour to use.
     * @return The resulting Integer.
     */
    [[nodiscard]]
    constexpr Integer div(T other, OverflowBehaviour behaviour = overflow_trap) const noexcept {
        switch (behaviour) {
            case OverflowBehaviour::Trap:
                return div_trap(other);
            case OverflowBehaviour::Exact:
                return div_exact(other);
            case OverflowBehaviour::Wrap:
                return div_wrap(other);
            case OverflowBehaviour::Saturate:
                return div_saturate(other);
        }

        detail::unreachable();
    }

    /**
     * @brief Add other to this Integer with trap on overflow.
     *
     * @param other The value to add.
     * @return The resulting Integer.
     */
    constexpr Integer& add_trap(T other) noexcept {
        T result{};
        if (add_overflow(mValue, other, result)) {
            enact_trap();
        }
        mValue = result;
        return *this;
    }

    /**
     * @brief Add other to this, invoking undefined behaviour on overflow.
     *
     * @param other The value to add.
     * @return The resulting Integer.
     */
    constexpr Integer& add_exact(T other) noexcept {
        T result{};
        if (add_overflow(mValue, other, result)) {
            enact_inexact();
        }
        mValue = result;
        return *this;
    }

    /**
     * @brief Add other to this, wrapping on overflow.
     *
     * @param other The value to add.
     * @return The resulting Integer.
     */
    constexpr Integer& add_wrap(T other) noexcept {
        add_overflow(mValue, other, mValue);
        return *this;
    }

    /**
     * @brief Add other to this, saturating on overflow.
     *
     * @param other The value to add.
     * @return The resulting Integer.
     */
    constexpr Integer& add_saturate(T other) noexcept {
        mValue = sm::numeric::add_saturate(mValue, other);
        return *this;
    }

    /**
     * @brief Add other to this with specified overflow behaviour.
     *
     * @param other The value to add.
     * @param behaviour The overflow behaviour to use.
     * @return The resulting Integer.
     */
    constexpr Integer& add(T other, OverflowBehaviour behaviour = overflow_trap) noexcept {
        switch (behaviour) {
            case OverflowBehaviour::Trap:
                return add_trap(other);
            case OverflowBehaviour::Exact:
                return add_exact(other);
            case OverflowBehaviour::Wrap:
                return add_wrap(other);
            case OverflowBehaviour::Saturate:
                return add_saturate(other);
        }

        detail::unreachable();
    }

    /**
     * @brief Subtract other from this Integer with trap on underflow.
     *
     * @param other The value to subtract.
     * @return The resulting Integer.
     */
    constexpr Integer& sub_trap(T other) noexcept {
        T result{};
        if (sub_overflow(mValue, other, result)) {
            enact_trap();
        }
        mValue = result;
        return *this;
    }

    /**
     * @brief Subtract other from this, invoking undefined behaviour on underflow.
     *
     * @param other The value to subtract.
     * @return The resulting Integer.
     */
    constexpr Integer& sub_exact(T other) noexcept {
        T result{};
        if (sub_overflow(mValue, other, result)) {
            enact_inexact();
        }
        mValue = result;
        return *this;
    }

    /**
     * @brief Subtract other from this, wrapping on underflow.
     *
     * @param other The value to subtract.
     * @return The resulting Integer.
     */
    constexpr Integer& sub_wrap(T other) noexcept {
        T result{};
        sub_overflow(mValue, other, result);
        mValue = result;
        return *this;
    }

    /**
     * @brief Subtract other from this, saturating on underflow.
     *
     * @param other The value to subtract.
     * @return The resulting Integer.
     */
    constexpr Integer& sub_saturate(T other) noexcept {
        mValue = sm::numeric::sub_saturate(mValue, other);
        return *this;
    }

    /**
     * @brief Subtract other from this with specified overflow behaviour.
     *
     * @param other The value to subtract.
     * @param behaviour The overflow behaviour to use.
     * @return The resulting Integer.
     */
    constexpr Integer& sub(T other, OverflowBehaviour behaviour = overflow_trap) noexcept {
        switch (behaviour) {
            case OverflowBehaviour::Trap:
                return sub_trap(other);
            case OverflowBehaviour::Exact:
                return sub_exact(other);
            case OverflowBehaviour::Wrap:
                return sub_wrap(other);
            case OverflowBehaviour::Saturate:
                return sub_saturate(other);
        }

        detail::unreachable();
    }

    /**
     * @brief Assignment operator for Integer.
     * Traps if the value cannot be represented exactly.
     *
     * @tparam U The source integer type.
     * @param other The value to assign.
     * @return The assigned Integer.
     */
    template <typename U>
    Integer& operator=(U other) noexcept {
        store(other);
        return *this;
    }

    /**
     * @brief Addition operator for Integer.
     * Traps on overflow.
     *
     * @param other The value to add.
     * @return The resulting Integer.
     */
    Integer operator+(T other) const noexcept {
        return plus(other);
    }

    /**
     * @brief Subtraction operator for Integer.
     * Traps on overflow.
     *
     * @param other The value to subtract.
     * @return The resulting Integer.
     */
    Integer operator-(T other) const noexcept {
        return minus(other);
    }

    /**
     * @brief Multiplication operator for Integer.
     * Traps on overflow.
     *
     * @param other The value to multiply.
     * @return The resulting Integer.
     */
    Integer operator*(T other) const noexcept {
        return mul(other);
    }

    /**
     * @brief Division operator for Integer.
     * Traps on overflow.
     *
     * @param other The value to divide by.
     * @return The resulting Integer.
     */
    Integer operator/(T other) const noexcept {
        return div(other);
    }

    /**
     * @brief Addition assignment operator for Integer.
     * Traps on overflow.
     *
     * @param other The value to add.
     * @return The resulting Integer.
     */
    Integer& operator+=(T other) noexcept {
        return add(other);
    }

    /**
     * @brief Subtraction assignment operator for Integer.
     * Traps on overflow.
     *
     * @param other The value to subtract.
     * @return The resulting Integer.
     */
    Integer& operator-=(T other) noexcept {
        return sub(other);
    }

    /**
     * @brief Multiplication assignment operator for Integer.
     * Traps on overflow.
     *
     * @param other The value to multiply.
     * @return The resulting Integer.
     */
    Integer& operator*=(T other) noexcept {
        *this = mul(other);
        return *this;
    }

    /**
     * @brief Division assignment operator for Integer.
     * Traps on overflow.
     *
     * @param other The value to divide by.
     * @return The resulting Integer.
     */
    Integer& operator/=(T other) noexcept {
        *this = div(other);
        return *this;
    }

    /**
     * @brief Compare two Integer instances for equality.
     *
     * This performs exact comparison and avoid issues with signed/unsigned comparisons by extending both
     * to a common type.
     *
     * @tparam L The type of the left-hand side Integer.
     * @tparam R The type of the right-hand side Integer.
     * @param lhs The left-hand side Integer.
     * @param rhs The right-hand side Integer.
     * @return true if equal,
     * @return false otherwise.
     */
    template <typename L, typename R>
    friend constexpr bool operator==(const Integer<L>& lhs, const Integer<R>& rhs) noexcept;

    /**
     * @brief Compare two Integer instances for inequality.
     *
     * This performs exact comparison and avoid issues with signed/unsigned comparisons by extending both
     * to a common type.
     *
     * @tparam L The type of the left-hand side Integer.
     * @tparam R The type of the right-hand side Integer.
     * @param lhs The left-hand side Integer.
     * @param rhs The right-hand side Integer.
     * @return true if not equal,
     * @return false otherwise.
     */
    template <typename L, typename R>
    friend constexpr bool operator!=(const Integer<L>& lhs, const Integer<R>& rhs) noexcept;

    /**
     * @brief Compare two Integer instances to see if the left-hand side is less than the right-hand side.
     *
     * This performs exact comparison and avoid issues with signed/unsigned comparisons by extending both
     * to a common type.
     *
     * @tparam L The type of the left-hand side Integer.
     * @tparam R The type of the right-hand side Integer.
     * @param lhs The left-hand side Integer.
     * @param rhs The right-hand side Integer.
     * @return true if lhs is less than rhs,
     * @return false otherwise.
     */
    template <typename L, typename R>
    friend constexpr bool operator<(const Integer<L>& lhs, const Integer<R>& rhs) noexcept;

    /**
     * @brief Compare two Integer instances to see if the left-hand side is less than or equal to the right-hand side.
     *
     * This performs exact comparison and avoid issues with signed/unsigned comparisons by extending both
     * to a common type.
     *
     * @tparam L The type of the left-hand side Integer.
     * @tparam R The type of the right-hand side Integer.
     * @param lhs The left-hand side Integer.
     * @param rhs The right-hand side Integer.
     * @return true if lhs is less than or equal to rhs,
     * @return false otherwise.
     */
    template <typename L, typename R>
    friend constexpr bool operator<=(const Integer<L>& lhs, const Integer<R>& rhs) noexcept;

    /**
     * @brief Compare two Integer instances to see if the left-hand side is greater than the right-hand side.
     *
     * This performs exact comparison and avoid issues with signed/unsigned comparisons by extending both
     * to a common type.
     *
     * @tparam L The type of the left-hand side Integer.
     * @tparam R The type of the right-hand side Integer.
     * @param lhs The left-hand side Integer.
     * @param rhs The right-hand side Integer.
     * @return true if lhs is greater than rhs,
     * @return false otherwise.
     */
    template <typename L, typename R>
    friend constexpr bool operator>(const Integer<L>& lhs, const Integer<R>& rhs) noexcept;

    /**
     * @brief Compare two Integer instances to see if the left-hand side is greater than or equal to the right-hand side.
     *
     * This performs exact comparison and avoid issues with signed/unsigned comparisons by extending both
     * to a common type.
     *
     * @tparam L The type of the left-hand side Integer.
     * @tparam R The type of the right-hand side Integer.
     * @param lhs The left-hand side Integer.
     * @param rhs The right-hand side Integer.
     * @return true if lhs is greater than or equal to rhs,
     * @return false otherwise.
     */
    template <typename L, typename R>
    friend constexpr bool operator>=(const Integer<L>& lhs, const Integer<R>& rhs) noexcept;

#if __STDC_HOSTED__
    /**
     * @brief Stream output operator for Integer.
     *
     * @param os The output stream.
     * @param integer The Integer to output.
     * @return The output stream.
     */
    friend std::ostream& operator<<(std::ostream& os, const Integer& integer) {
        os << integer.load();
        return os;
    }
#endif
};

namespace detail {
template <typename L, typename R, typename F>
constexpr bool applyOperator(const Integer<L>& lhs, const Integer<R>& rhs, F&& func) noexcept {
    if constexpr (std::is_signed_v<L> == std::is_signed_v<R>) {
        return func(lhs.load(), rhs.load());
    } else {
        using common = std::common_type_t<L, R>;
        return func(lhs.template load<common>(), rhs.template load<common>());
    }
}
} // namespace detail

template <typename L, typename R>
constexpr bool operator==(const Integer<L>& lhs, const Integer<R>& rhs) noexcept {
    return detail::applyOperator(lhs, rhs, [](auto l, auto r) { return l == r; });
}

template <typename L, typename R>
constexpr bool operator!=(const Integer<L>& lhs, const Integer<R>& rhs) noexcept {
    return detail::applyOperator(lhs, rhs, [](auto l, auto r) { return l != r; });
}

template <typename L, typename R>
constexpr bool operator<(const Integer<L>& lhs, const Integer<R>& rhs) noexcept {
    return detail::applyOperator(lhs, rhs, [](auto l, auto r) { return l < r; });
}

template <typename L, typename R>
constexpr bool operator<=(const Integer<L>& lhs, const Integer<R>& rhs) noexcept {
    return detail::applyOperator(lhs, rhs, [](auto l, auto r) { return l <= r; });
}

template <typename L, typename R>
constexpr bool operator>(const Integer<L>& lhs, const Integer<R>& rhs) noexcept {
    return detail::applyOperator(lhs, rhs, [](auto l, auto r) { return l > r; });
}

template <typename L, typename R>
constexpr bool operator>=(const Integer<L>& lhs, const Integer<R>& rhs) noexcept {
    return detail::applyOperator(lhs, rhs, [](auto l, auto r) { return l >= r; });
}

namespace aliases {
using Char = Integer<char>;
using Short = Integer<short>;
using Int = Integer<int>;
using Long = Integer<long>;
using LongLong = Integer<long long>;

using UChar = Integer<unsigned char>;
using UShort = Integer<unsigned short>;
using UInt = Integer<unsigned int>;
using ULong = Integer<unsigned long>;
using ULongLong = Integer<unsigned long long>;

using u8 = Integer<uint8_t>;
using u16 = Integer<uint16_t>;
using u32 = Integer<uint32_t>;
using u64 = Integer<uint64_t>;

using i8 = Integer<int8_t>;
using i16 = Integer<int16_t>;
using i32 = Integer<int32_t>;
using i64 = Integer<int64_t>;

using u8_fast = Integer<uint_fast8_t>;
using u16_fast = Integer<uint_fast16_t>;
using u32_fast = Integer<uint_fast32_t>;
using u64_fast = Integer<uint_fast64_t>;

using i8_fast = Integer<int_fast8_t>;
using i16_fast = Integer<int_fast16_t>;
using i32_fast = Integer<int_fast32_t>;
using i64_fast = Integer<int_fast64_t>;

using u8_least = Integer<uint_least8_t>;
using u16_least = Integer<uint_least16_t>;
using u32_least = Integer<uint_least32_t>;
using u64_least = Integer<uint_least64_t>;

using i8_least = Integer<int_least8_t>;
using i16_least = Integer<int_least16_t>;
using i32_least = Integer<int_least32_t>;
using i64_least = Integer<int_least64_t>;

using usize = Integer<std::size_t>;
using ssize = Integer<std::make_signed_t<std::size_t>>;
using ptrdiff = Integer<std::ptrdiff_t>;
} // namespace aliases

} // namespace sm::numeric
