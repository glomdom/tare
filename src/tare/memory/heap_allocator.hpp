// SPDX-License-Identifier: BSL-1.0

#pragma once

#include <tare/memory/allocator.hpp>

namespace tare::memory {

/// @brief A general-purpose allocator.
///
/// Handles any power-of-two alignment, including over-aligned types.
/// It holds no state, so every instance is interchangeable:
/// a block from one can be freed through another.
class HeapAllocator final : public Allocator {
public:
  /// @copydoc Allocator::allocate
  [[nodiscard]] Span<u8> allocate(Layout layout) override;

  /// @copydoc Allocator::deallocate
  void deallocate(u8* ptr, Layout layout) override;
};

/// @brief The default allocator, which is a `HeapAllocator`.
///
/// @return The shared heap allocator.
[[nodiscard]] Allocator& default_allocator();


}