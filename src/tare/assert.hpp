// SPDX-License-Identifier: BSL-1.0

#pragma once

// why UB when you can abort :gnutroll:
#ifndef TARE_ASSERT
  #if defined(_MSC_VER) && !defined(__clang__)
    #define TARE_TRAP() __fastfail(7)
  #else
    #define TARE_TRAP() __builtin_trap()
  #endif

  #define TARE_ASSERT(cond) \
    do {                    \
      if (!(cond)) {        \
        TARE_TRAP();        \
      }                     \
    } while (false)
#endif