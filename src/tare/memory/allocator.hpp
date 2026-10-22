// SPDX-License-Identifier: BSL-1.0

#pragma once

#include <tare/span.hpp>
#include <tare/types.hpp>
#include <tare/memory/layout.hpp>

namespace tare::memory {

/// @sa HeapAllocator
class Allocator {
public:
  /// @brief Hands out a block of memory for `layout`.
  ///
  /// The block may be larger than requested.
  ///
  /// @pre `layout.size > 0` and `layout.align` is a power of 2.
  /// @param layout The size and alignment needed.
  ///
  /// @return The block, at least `layout.size` bytes long and aligned
  ///         to `layout.align`. Never null, as null results in a panic.
  [[nodiscard]] virtual Span<u8> allocate(Layout layout) = 0;

  /// @brief Gives a block back.
  ///
  /// @pre `ptr` came from `allocate()` on an equivalent allocator, with the
  ///      same `layout`, and hasn't been freed yet.
  ///
  /// @param ptr The start of the block.
  /// @param layout The layout it was allocated with.
  virtual void deallocate(u8* ptr, Layout layout) = 0;

protected:
  ~Allocator() = default;
};

}
