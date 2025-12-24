// SPDX-License-Identifier: Apache-2.0

#pragma once

#if __cpp_exceptions >= 199711L
#   include <exception>
#endif // __cpp_exceptions >= 199711L

namespace sm::detail {
enum class DeferTag { eTag };

#if __cpp_exceptions >= 199711L
enum class ErrDeferTag { eTag };
#endif // __cpp_exceptions >= 199711L

} // namespace sm::detail

template<typename F>
constexpr auto operator+(sm::detail::DeferTag, F fn) {
    struct Inner {
        F fn;
        ~Inner() noexcept { fn(); }
    };

    return Inner{fn};
}

#if __cpp_exceptions >= 199711L
template<typename F>
constexpr auto operator+(sm::detail::ErrDeferTag, F fn) {
    struct Inner {
        F fn;
        ~Inner() {
            if (std::uncaught_exceptions() > 0) {
                fn();
            }
        }
    };

    return Inner{fn};
}
#endif // __cpp_exceptions >= 199711L

#define SM_DETAIL_CONCAT2(a, b) a##b
#define SM_DETAIL_CONCAT(a, b) SM_DETAIL_CONCAT2(a, b)
#define SM_DETAIL_EXPAND(x) x

/**
 * @defgroup Defer Defer Macros
 *
 * @brief Macros for deferring execution of code until scope exit.
 *
 * Define `SM_DEFER_AS_KEYWORD` to enable `defer` and `errdefer` as keywords.
 * @code{.cpp}
 * #define SM_DEFER_AS_KEYWORD 1
 * #include <simcoe/defer/defer.hpp>
 * @endcode
 * @{
 */

/**
 * @brief Defers execution of the given code block until the surrounding scope exits.
 * @code{.cpp}
 * {
 *     defer { printf("World\n"); };
 *     printf("Hello "); // "Hello " is printed here
 * } // "World" is printed here
 * @endcode
 */
#define SM_DEFER const auto SM_DETAIL_CONCAT(defer, __COUNTER__) = sm::detail::DeferTag::eTag + [&] ()

#if defined(SM_DEFER_AS_KEYWORD)
#   define defer SM_DEFER
#endif // defined(SM_DEFER_AS_KEYWORD)

#if __cpp_exceptions >= 199711L
/**
 * @brief Defers execution of the given code block until the surrounding scope exits due to an exception.
 * @code{.cpp}
 * try {
 *     errdefer { printf("Error occurred\n"); };
 *     // some code that may throw
 * } catch (...) {
 *     // "Error occurred" is printed here if an exception was thrown
 * }
 * @endcode
 * Only executes if the scope is exited due to an exception.
 * @code{.cpp}
 * {
 *     errdefer { printf("Error occurred\n"); };
 *     // some code that does not throw
 * } // nothing is printed here
 * @endcode
 */
#   define SM_ERRDEFER const auto SM_DETAIL_CONCAT(errdefer, __COUNTER__) = sm::detail::ErrDeferTag::eTag + [&] ()
#   if defined(SM_DEFER_AS_KEYWORD)
#       define errdefer SM_ERRDEFER
#   endif // defined(SM_DEFER_AS_KEYWORD)
#endif // __cpp_exceptions >= 199711L

/** @} */
