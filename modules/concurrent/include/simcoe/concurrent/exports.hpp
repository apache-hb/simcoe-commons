// SPDX-License-Identifier: Apache-2.0

#pragma once

#if defined(SM_CONCURRENT_API_EXPORT)
#   if defined(_WIN32) || defined(_MSC_VER)
#        define SM_CONCURRENT_API __declspec(dllexport)
#    else
#        define SM_CONCURRENT_API __attribute__((visibility("default")))
#    endif // defined(_WIN32) || defined(_MSC_VER)
#else
#   if defined(_WIN32) || defined(_MSC_VER)
#        define SM_CONCURRENT_API __declspec(dllimport)
#    else
#        define SM_CONCURRENT_API
#    endif // defined(_WIN32) || defined(_MSC_VER)
#endif // SM_CONCURRENT_API_EXPORT
