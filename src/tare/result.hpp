#pragma once

#include <tare/assert.hpp>
#include <tare/traits.hpp>

namespace tare {

template <typename T>
struct Ok {
  T value;
};

template <typename T>
Ok(T) -> Ok<T>;

template <typename T>
struct Err {
  T value;
};

template <typename T>
Err(T) -> Err<T>;

struct InPlaceOkT {
  explicit constexpr InPlaceOkT() = default;
};

struct InPlaceErrT {
  explicit constexpr InPlaceErrT() = default;
};

inline constexpr InPlaceOkT InPlaceOk{};
inline constexpr InPlaceErrT InPlaceErr{};

namespace detail {

template <typename X>
constexpr X duplicate(const X& x) {
  if constexpr (Cloneable<X>) {
    return x.clone();
  } else {
    return x;
  }
}

}

/// Either holds a success value `T` or a failure `E`. Always exactly one of
/// the two, never both and never neither.
///
/// Return this from anything that can fail, instead of an error code with an
/// out-parameter or an exception. The failure is part of the signature, so
/// the caller can see it, and has to get past it to reach the value.
///
/// Build one with `Ok(x)` or `Err(e)`. A bare value is rejected on purpose,
/// so `Result<int, int>` is never ambiguous about which side it's on. To
/// construct a side directly, use `InPlaceOk` or `InPlaceErr`. That's also
/// the only way to get an immovable type in.
///
/// Copying is the same `Optional`: free when both sides are
/// trivially copyable, otherwise ask for one with `clone()`.
template <typename T, typename E>
class Result {
public:
  template <typename U> requires ConstructibleFrom<T, U>
  constexpr Result(Ok<U>&& ok) : _value(tare::move(ok.value)), _ok{true} {}
  
  template <typename U> requires ConstructibleFrom<E, U>
  constexpr Result(Err<U>&& err) : _err(tare::move(err.value)), _ok{false} {}
  
  template <typename... Args> requires ConstructibleFrom<T, Args...>
  constexpr explicit Result(InPlaceOkT, Args&&... args) : _value(tare::forward<Args>(args)...), _ok{true} {}
  
  template <typename... Args> requires ConstructibleFrom<E, Args...>
  constexpr explicit Result(InPlaceErrT, Args&&... args) : _err(tare::forward<Args>(args)...), _ok{false} {}

  [[nodiscard]] constexpr Result clone() const requires Duplicable<T> && Duplicable<E> {
    if (!_ok) {
      return Result{InPlaceErr, detail::duplicate(_err)};
    }

    return Result{InPlaceOk, detail::duplicate(_value)};
  }

  [[nodiscard]] constexpr bool is_ok() const noexcept {
    return _ok;
  }

  [[nodiscard]] constexpr bool is_err() const noexcept {
    return !_ok;
  }

  [[nodiscard]] constexpr E& error() & {
    TARE_ASSERT(!_ok);

    return _err;
  }

  [[nodiscard]] constexpr const E& error() const& {
    TARE_ASSERT(!_ok);

    return _err;
  }

  [[nodiscard]] constexpr T& operator*() & {
    TARE_ASSERT(_ok);

    return _value;
  }

  [[nodiscard]] constexpr const T& operator*() const& {
    TARE_ASSERT(_ok);

    return _value;
  }

  [[nodiscard]] constexpr T&& operator*() && {
    TARE_ASSERT(_ok);

    return tare::move(_value);
  } 

  [[nodiscard]] constexpr T* operator->() {
    TARE_ASSERT(_ok);

    return __builtin_addressof(_value);
  }

  [[nodiscard]] constexpr const T* operator->() const {
    TARE_ASSERT(_ok);

    return __builtin_addressof(_value);
  }

private:
  union {
    T _value;
    E _err;
  };

  bool _ok;
};

}