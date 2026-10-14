// SPDX-License-Identifier: BSL-1.0

#pragma once

#include <tare/panic.hpp>
#include <tare/traits.hpp>
#include <tare/types.hpp>

namespace tare {

/// @brief Represents a view over an array of `T`.
///
/// `Span` provides access to an existing array or contigous memory region
/// without taking ownership or managing lifetimes. It is a container of a
/// pointer, of the start, and the number of elements inside.
///
/// @tparam T The type of the elements in the array.
///
/// @note `Span` does not own the referenced memory, nor does it manage the
///       lifetime/validity of the elements.
template <typename T>
class Span {
public:
  /// @brief The type of elements contained in the `Span`.
  using ValueType = T;

  /// @brief Constructs an empty `Span`.
  ///
  /// @post `len() == 0` and `data() == nullptr`.
  constexpr Span() noexcept = default;

  /// @brief Constructs a `Span` from a C-style array.
  ///
  /// @tparam N The size of the array.
  /// @param array Reference to the raw array to view.
  template <usize N>
  constexpr Span(T (&array)[N]) noexcept : _data{array}, _len{N} {}

  /// @brief Constructs a `Span` from another convertible `Span`.
  ///
  /// @tparam U The element type of the source span.
  /// @param other The source span to convert from (e.g., non-const to const).
  template <typename U> requires SameAs<T, const U>
  constexpr Span(Span<U> other) noexcept : Span<T>{other.data(), other.len()} {}

  /// @brief Creates a `Span` directly from a raw pointer and length.
  ///
  /// @warning It must be ensured that `data` points to a valid contiguous
  ///          memory region containing at least `len` elements, and that the
  ///          memory remains valid and unchanged throughout the lifetime of the `Span`.
  ///
  /// @param data Pointer to the start of the memory region.
  /// @param len The number of elements in the region.
  ///
  /// @return A new `Span` covering the specified raw memory.
  [[nodiscard]] static constexpr Span<T> from_raw(T* data, usize len) noexcept {
    return Span<T>{data, len};
  }

  /// @brief Returns the number of elements in the `Span`.
  ///
  /// @return The element count.
  [[nodiscard]] constexpr usize len() const noexcept {
    return _len;
  }

  /// @brief Checks whether the `Span` is empty.
  ///
  /// @return `true` if the `Span` contains no elements, `false` otherwise.
  [[nodiscard]] constexpr bool is_empty() const noexcept {
    return _len == 0;
  }

  /// @brief Returns a pointer to the beginning of the memory region.
  ///
  /// @return Pointer to the first element, or `nullptr` if the `Span` is empty.
  [[nodiscard]] constexpr T* data() const noexcept {
    return _data;
  }

  /// @brief Accesses an element at the specified index with bounds checking.
  ///
  /// @pre `idx < len()` (enforced via assertion).
  ///
  /// @param idx The index of the element to access.
  /// @return Reference to the element at the given index.
  [[nodiscard]] constexpr T& operator[](usize idx) const {
    TARE_ASSERT(idx < _len);

    return _data[idx];
  }

  /// @brief Safely retrieves a pointer to the element at the specified index.
  ///
  /// @param idx The index of the element to retrieve.
  /// @return Pointer to the element, or `nullptr` if the index is out of bounds.
  [[nodiscard]] constexpr T* get(usize idx) const noexcept {
    if (idx >= _len) {
      return nullptr;
    }

    return &_data[idx];
  }

  /// @brief Returns a pointer to the first element in the `Span`.
  ///
  /// @return Pointer to the first element, or `nullptr` if the `Span` is empty.
  [[nodiscard]] constexpr T* first() const noexcept {
    if (is_empty()) {
      return nullptr;
    }

    return &_data[0];
  }

  /// @brief Returns a pointer to the last element in the `Span`.
  ///
  /// @return Pointer to the last element, or `nullptr` if the `Span` is empty.
  [[nodiscard]] constexpr T* last() const noexcept {
    if (is_empty()) {
      return nullptr;
    }

    return &_data[_len - 1];
  }

  /// @brief Creates a sub-span from a starting index and length.
  ///
  /// @pre `start <= len()`
  /// @pre `len <= len() - start` (both enforced via assertions).
  ///
  /// @param start The starting index of the sub-span.
  /// @param len The number of elements in the sub-span.
  /// @return A new `Span` representing the requested sub-range.
  [[nodiscard]] constexpr Span<T> sub(usize start, usize len) const {
    TARE_ASSERT(start <= _len);
    TARE_ASSERT(len <= _len - start);

    return Span<T>{_data + start, len};
  }

  /// @brief Takes the first `n` elements of the `Span`.
  ///
  /// @pre `n <= len()` (enforced via assertion).
  ///
  /// @param n The number of elements to take.
  /// @return A new `Span` containing the first `n` elements.
  [[nodiscard]] constexpr Span<T> take(usize n) const {
    TARE_ASSERT(n <= _len);

    return Span<T>{_data, n};
  }

  /// @brief Skips the first `n` elements and returns a `Span` over the rest.
  ///
  /// @pre `n <= len()` (enforced via assertion).
  ///
  /// @param n The number of elements to skip.
  /// @return A new `Span` containing the remaining elements.
  [[nodiscard]] constexpr Span<T> skip(usize n) const {
    TARE_ASSERT(n <= _len);

    return Span<T>{_data + n, _len - n};
  }

  /// @brief Returns a pointer to the beginning of the `Span` for iteration.
  ///
  /// @return Pointer to the first element.
  [[nodiscard]] constexpr T* begin() const noexcept {
    return _data;
  }

  /// @brief Returns a pointer to one-past-the-end of the `Span` for iteration.
  ///
  /// @return Pointer to the theoretical element after the last.
  [[nodiscard]] constexpr T* end() const noexcept {
    return _data + _len;
  }

private:
  constexpr Span(T* data, usize len) noexcept : _data{data}, _len{len} {}

  T* _data{};
  usize _len{};
};

}