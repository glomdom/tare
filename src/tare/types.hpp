// SPDX-License-Identifier: BSL-1.0

#pragma once

namespace tare {

#if defined(__INT8_TYPE__)
  using i8 = __INT8_TYPE__;
  using i16 = __INT16_TYPE__;
  using i32 = __INT32_TYPE__;
  using i64 = __INT64_TYPE__;

  using u8 = __UINT8_TYPE__;
  using u16 = __UINT16_TYPE__;
  using u32 = __UINT32_TYPE__;
  using u64 = __UINT64_TYPE__;
#else
  using i8 = signed char;
  using i16 = short;
  using i32 = int;
  using i64 = long long;

  using u8 = unsigned char;
  using u16 = unsigned short;
  using u32 = unsigned int;
  using u64 = unsigned long long;
#endif

using usize = decltype(sizeof(0));
using isize = decltype(static_cast<int*>(nullptr) - static_cast<int*>(nullptr));

using f32 = float;
using f64 = double;

static_assert(sizeof(i8) == 1 && sizeof(u8) == 1);
static_assert(sizeof(i16) == 2 && sizeof(u16) == 2);
static_assert(sizeof(i32) == 4 && sizeof(u32) == 4);
static_assert(sizeof(i64) == 8 && sizeof(u64) == 8);
static_assert(sizeof(usize) == sizeof(void*));
static_assert(sizeof(isize) == sizeof(void*));
static_assert(sizeof(f32) == 4 && sizeof(f64) == 8);

}