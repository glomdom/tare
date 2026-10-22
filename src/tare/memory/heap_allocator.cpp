// SPDX-License-Identifier: BSL-1.0

#include <tare/memory/heap_allocator.hpp>

#include <tare/platform/memory.hpp>

namespace tare::memory {

Span<u8> HeapAllocator::allocate(const Layout layout) {
  TARE_ASSERT(layout.size != 0);

  if (layout.align > 16) {
    void* mem = platform::aligned_alloc(layout.size, layout.align);
    TARE_ASSERT(mem != nullptr);

    return Span<u8>::from_raw(static_cast<u8*>(mem), layout.size);
  }

  void* mem = malloc(layout.size);
  TARE_ASSERT(mem != nullptr);

  return Span<u8>::from_raw(static_cast<u8*>(mem), layout.size);
}

void HeapAllocator::deallocate(u8* ptr, const Layout layout) {
  TARE_ASSERT(layout.size != 0);

  if (layout.align > 16) {
    platform::aligned_free(ptr);

    return;
  }

  free(ptr);
}

[[nodiscard]] Allocator& default_allocator() {
  static HeapAllocator allocator;

  return allocator;
}

}
