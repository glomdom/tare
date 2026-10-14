// SPDX-License-Identifier: BSL-1.0

#pragma once

#include <cstdio>
#include <cstdlib>
#include <source_location>
#include <string_view>

namespace tare {

[[noreturn]] inline void panic(std::string_view message, std::source_location location = std::source_location::current()) noexcept {
  std::fprintf(
    stderr,
    "tare panic: %.*s\n"
    "  at %s:%u\n"
    "  in %s\n",
    static_cast<int>(message.size()),
    message.data(),
    location.file_name(),
    location.line(),
    location.function_name()
  );

  std::fflush(stderr);
  std::abort();
}

}

#define TARE_PANIC(message) \
  ::tare::panic((message))

#define TARE_ASSERT(condition) \
  do {                                                                  \
    if (!(condition)) { TARE_PANIC("assertion failed: " #condition); }  \
  } while (false)