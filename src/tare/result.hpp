#pragma once

#include "tare/optional.hpp"
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

template <typename T, typename E>
class Result;

namespace detail {

template <typename T>
inline constexpr bool is_result_v = false;

template <typename T, typename E>
inline constexpr bool is_result_v<Result<T, E>> = true;

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
  using ValueType = T;
  using ErrorType = E;

  template <typename U> requires ConstructibleFrom<T, U>
  constexpr Result(Ok<U>&& ok) : _value(tare::move(ok.value)), _ok{true} {}
  
  template <typename U> requires ConstructibleFrom<E, U>
  constexpr Result(Err<U>&& err) : _err(tare::move(err.value)), _ok{false} {}

  Result(const Result&) = delete;
  Result& operator=(const Result&) = delete;

  constexpr Result(const Result&) requires AppliesForAll<TriviallyCopyable, T, E> = default;
  constexpr Result& operator=(const Result&) requires AppliesForAll<TriviallyCopyable, T, E> = default;

  constexpr Result(Result&&) requires AppliesForAll<TriviallyMoveConstructible, T, E> = default;
  constexpr Result(Result&& other) noexcept requires AppliesForAll<Movable, T, E> && (!AppliesForAll<TriviallyMoveConstructible, T, E>) : _ok{other._ok} {
    if (other._ok) {
      detail::construct_at(__builtin_addressof(_value), tare::move(other._value));
    } else {
      detail::construct_at(__builtin_addressof(_err), tare::move(other._err));
    }
  }

  constexpr Result& operator=(Result&&) requires AppliesForAll<TriviallyMoveAssignableAndDestructible, T, E> = default;
  constexpr Result& operator=(Result&& other) noexcept requires AppliesForAll<Movable, T, E> && (!(AppliesForAll<TriviallyMoveAssignableAndDestructible, T, E>)) {
    if (this == __builtin_addressof(other)) {
      return *this;
    }

    destroy();

    _ok = other._ok;
    if (other._ok) {
      detail::construct_at(__builtin_addressof(_value), tare::move(other._value));
    } else {
      detail::construct_at(__builtin_addressof(_err), tare::move(other._err));
    }

    return *this;
  }

  constexpr ~Result() requires TriviallyDestructible<T> && TriviallyDestructible<E> = default;
  constexpr ~Result() {
    destroy();
  }

  template <typename... Args>
  [[nodiscard]] static constexpr Result ok(Args&&... args) {
    return Result(OkTag{}, tare::forward<Args>(args)...);
  }

  template <typename... Args>
  [[nodiscard]] static constexpr Result err(Args&&... args) {
    return Result(ErrTag{}, tare::forward<Args>(args)...);
  }

  [[nodiscard]] constexpr Result clone() const requires Duplicable<T> && Duplicable<E> {
    if (!_ok) {
      return Result::err(detail::duplicate(_err));
    }

    return Result::ok(detail::duplicate(_value));
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

  [[nodiscard]] constexpr T unwrap() && {
    TARE_ASSERT(_ok);

    return tare::move(_value);
  } 

  [[nodiscard]] constexpr E unwrap_err() && {
    TARE_ASSERT(!_ok);

    return tare::move(_err);
  }

  [[nodiscard]] constexpr T unwrap_or(T value) && {
    return _ok ? _value : value;
  }

  template <typename TFunction>
  [[nodiscard]] constexpr T unwrap_or_else(TFunction&& f) && {
    if (_ok) {
      return tare::move(_value);
    }

    return tare::forward<TFunction>(f)(tare::move(_err));
  }
  
  [[nodiscard]] constexpr Optional<T> ok() && requires Movable<T> {
    if (!_ok) {
      return None;
    }

    return Optional<T>{tare::move(_value)};
  }

  [[nodiscard]] constexpr Optional<E> err() && requires Movable<E> {
    if (_ok) {
      return None;
    }

    return Optional<E>{tare::move(_err)};
  }

  template <typename TFunction>
  [[nodiscard]] constexpr auto map(TFunction&& f) && {
    using U = RemoveCvRef<decltype(tare::forward<TFunction>(f)(tare::move(_value)))>;

    if (!_ok) {
      return Result<U, E>::err(tare::move(_err));
    }

    return Result<U, E>::ok(tare::forward<TFunction>(f)(tare::move(_value)));
  }

  template <typename TFunction>
  [[nodiscard]] constexpr auto map_err(TFunction&& f) && {
    using U = RemoveCvRef<decltype(tare::forward<TFunction>(f)(tare::move(_err)))>;

    if (_ok) {
      return Result<T, U>::ok(tare::move(_value));
    }

    return Result<T, U>::err(tare::forward<TFunction>(f)(tare::move(_err)));
  }

  template <typename TFunction>
  [[nodiscard]] constexpr auto and_then(TFunction&& f) && {
    using Output = RemoveCvRef<decltype(tare::forward<TFunction>(f)(tare::move(_value)))>;

    static_assert(detail::is_result_v<Output>, "and_then callback must return a Result");

    if (!_ok) {
      return Output::err(tare::move(_err));
    }

    return tare::forward<TFunction>(f)(tare::move(_value));
  }

  template <typename TFunction>
  [[nodiscard]] constexpr auto or_else(TFunction&& f) && {
    using Output = RemoveCvRef<decltype(tare::forward<TFunction>(f)(tare::move(_err)))>;

    static_assert(detail::is_result_v<Output>, "or_else callback must return a Result");

    if (_ok) {
      return Output::ok(tare::move(_value));
    }

    return tare::forward<TFunction>(f)(tare::move(_err));
  }

  template <typename U = T, typename V = E>
  [[nodiscard]] friend constexpr bool operator==(const Result& a, const Result& b) requires requires(const U& x, const V& y) { x == x; y == y; } {
    if (a._ok != b._ok) {
      return false;
    }

    if (a._ok) {
      return a._value == b._value;
    }
    
    return a._err == b._err;
  }

private:
  struct OkTag {};
  struct ErrTag {};

  template <typename... Args>
  constexpr Result(OkTag, Args&&...args) : _value(tare::forward<Args>(args)...), _ok{true} {}
  
  template <typename... Args>
  constexpr Result(ErrTag, Args&&...args) : _err(tare::forward<Args>(args)...), _ok{false} {}

  constexpr void destroy() {
    if (_ok) {
      detail::destroy_at(__builtin_addressof(_value));
    } else { 
      detail::destroy_at(__builtin_addressof(_err));
    }
  }

  union {
    T _value;
    E _err;
  };

  bool _ok{};
};

}