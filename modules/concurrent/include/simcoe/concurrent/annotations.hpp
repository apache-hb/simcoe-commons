// SPDX-License-Identifier: Apache-2.0

#pragma once

#if defined(__has_attribute)
#    if __has_attribute(nonblocking)
#        define SM_CLANG_NONBLOCKING [[clang::nonblocking]]
#    endif
#    if __has_attribute(blocking)
#        define SM_CLANG_BLOCKING [[clang::blocking]]
#    endif
#    if __has_attribute(nonnull) && __has_attribute(noescape)
#        define SM_OUTPARAM [[gnu::nonnull, clang::noescape]]
#    endif
#endif

#if !defined(SM_CLANG_NONBLOCKING)
#    define SM_CLANG_NONBLOCKING
#endif

#if !defined(SM_CLANG_BLOCKING)
#    define SM_CLANG_BLOCKING
#endif

#if !defined(SM_OUTPARAM)
#    define SM_OUTPARAM
#endif
