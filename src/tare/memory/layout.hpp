// SPDX-License-Identifier: BSL-1.0

#pragma once

#include <tare/types.hpp>

namespace tare::memory {

/// @brief The size and alignment of a block of memory.
///
/// Every allocation is described by one. The same `Layout` goes back to the
/// allocator when the block is freed, so the allocator doesn't have to remember
/// what it allocated.
///
/// ```
/// auto layout = Layout::of<u8>(1024);
/// ```
struct Layout {
  /// @brief Size in bytes.
  usize size;

  /// @brief Alignment in bytes. Always a power of 2.
  usize align;

  /// The layout of `count` values of `T` laid out back to back.
  ///
  /// @tparam T The element type.
  /// @param count How many elements.
  ///
  /// @return A layout of `sizeof(T) * count` bytes, aligned for `T`.
  template <typename T>
  [[nodiscard]] static constexpr Layout of(const usize count = 1) {
    return Layout{
      .size = sizeof(T) * count,
      .align = alignof(T),
    };
  }
};

}
