// SPDX-License-Identifier: BSL-1.0

#pragma once

#include <tare/assert.hpp>
#include <tare/traits.hpp>
#include <tare/types.hpp>

namespace tare {

template <typename T>
class Span {
public:
  using ValueType = T;

  constexpr Span() noexcept = default;

  template <usize N>
  constexpr Span(T (&array)[N]) noexcept : _data{array}, _len{N} {}

  template <typename U> requires SameAs<T, const U>
  constexpr Span(Span<U> other) noexcept : Span<T>{other.data(), other.len()} {}

  [[nodiscard]] static constexpr Span<T> from_raw(T* data, usize len) noexcept {
    return Span<T>{data, len};
  }

  [[nodiscard]] constexpr usize len() const noexcept {
    return _len;
  }

  [[nodiscard]] constexpr bool is_empty() const noexcept {
    return _len == 0;
  }

  [[nodiscard]] constexpr T* data() const noexcept {
    return _data;
  }

  [[nodiscard]] constexpr T& operator[](usize idx) const {
    TARE_ASSERT(idx < _len);

    return _data[idx];
  }

  [[nodiscard]] constexpr T* get(usize idx) const noexcept {
    if (idx >= _len) {
      return nullptr;
    }

    return &_data[idx];
  }

  [[nodiscard]] constexpr T* first() const noexcept {
    if (is_empty()) {
      return nullptr;
    }

    return &_data[0];
  }

  [[nodiscard]] constexpr T* last() const noexcept {
    if (is_empty()) {
      return nullptr;
    }

    return &_data[_len - 1];
  }

  [[nodiscard]] constexpr Span<T> sub(usize start, usize len) const {
    TARE_ASSERT(start <= _len);
    TARE_ASSERT(len <= _len - start);

    return Span<T>{_data + start, len};
  }

  [[nodiscard]] constexpr Span<T> take(usize n) const {
    TARE_ASSERT(n <= _len);

    return Span<T>{_data, n};
  }

  [[nodiscard]] constexpr Span<T> skip(usize n) const {
    TARE_ASSERT(n <= _len);

    return Span<T>{_data + n, _len - n};
  }

  [[nodiscard]] constexpr T* begin() const noexcept {
    return _data;
  }

  [[nodiscard]] constexpr T* end() const noexcept {
    return _data + _len;
  }

private:
  constexpr Span(T* data, usize len) noexcept : _data{data}, _len{len} {}

  T* _data{};
  usize _len{};
};

}