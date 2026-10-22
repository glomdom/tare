// SPDX-License-Identifier: BSL-1.0

#include <tare/platform/memory.hpp>

#include <cstddef>
#include <stdlib.h>

namespace tare::platform {

[[nodiscard]] u8* aligned_alloc(const usize size, const usize align) {
  if (align <= alignof(std::max_align_t)) {
    return static_cast<u8*>(::malloc(size));
  }

  void* ptr = nullptr;

  if (::posix_memalign(&ptr, align, size) != 0) {
    return nullptr;
  }

  return static_cast<u8*>(ptr);
}

void aligned_free(u8* ptr) {
  ::free(ptr);
}

}