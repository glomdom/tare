// SPDX-License-Identifier: BSL-1.0

#pragma once

#include <tare/types.hpp>

namespace tare::platform {

/// Allocates `size` bytes aligned to `align`.
///
/// Doesn't check nor panic.
///
/// @pre `size > 0`, and `align` is a power of two.
/// @param size Size in bytes.
/// @param align Alignment in bytes.
///
/// @return The memory, or null if the heap is out.
[[nodiscard]] u8* aligned_alloc(usize size, usize align);

/// Frees memory from `aligned_alloc()`.
///
/// @pre `ptr` came from `aligned_alloc()` and hasn't been freed yet.
/// @param ptr The memory to free.
void aligned_free(u8* ptr);

}