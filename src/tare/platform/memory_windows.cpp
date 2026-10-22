// SPDX-License-Identifier: BSL-1.0

#include <tare/platform/memory.hpp>

#include <malloc.h>

namespace tare::platform {

[[nodiscard]] u8* aligned_alloc(const usize size, const usize align) {
  void* ptr = _aligned_malloc(size, align);

  return static_cast<u8*>(ptr);
}

void aligned_free(u8* ptr) {
  _aligned_free(ptr);
}

}
