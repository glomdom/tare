// SPDX-License-Identifier: BSL-1.0

#pragma once

#include <tare/optional.hpp>
#include <tare/panic.hpp>
#include <tare/traits.hpp>

namespace tare {

/// @brief Marks a value as the success side of a `Result`.
///
/// It only lives for the length of a `return`, then gets unpacked into the
/// `Result`:
///
/// ```
/// return Ok(42);
/// ```
///
/// @tparam T The type of the success value.
template <typename T>
struct Ok {
  /// The success value being carried.
  T value;
};

/// Deduction guide for `Ok`.
template <typename T>
Ok(T) -> Ok<T>;

/// @brief Marks a value as the failure side of a `Result`.
///
/// The counterpart to `Ok`:
///
/// ```
/// return Err(Error::Invalid);
/// ```
///
/// @tparam T The type of the error.
template <typename T>
struct Err {
  /// The error being carried.
  T value;
};

/// Deduction guide for `Err`.
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

/// @brief Either holds a success value `T` or a failure `E`. Always exactly one of
///        the two, never both and never neither.
///
/// Return this from anything that can fail.
///
/// ```
/// Result<int, Error> parse_digit(char c) {
///   if (c < '0' || c > '9') {
///     return Err(Error::Invalid);
///   }
///
///   return Ok(c - '0');
/// }
/// ```
///
/// Build one with `Ok(x)` or `Err(e)`. A bare value is rejected on purpose,
/// so `Result<int, int>` is never ambiguous. To build a side directly from
/// its constructor arguments, use `ok()` or `err()`. That's also the only
/// way an immovable value can be inside.
///
/// Copying works the same as `Optional`: free when both sides are trivially
/// copyable, otherwise ask for one with `clone()`.
///
/// @tparam T The type of the success value.
/// @tparam E The type of the failure.
///
/// @sa Optional
template <typename T, typename E>
class Result {
public:
  /// The type of the success value.
  using ValueType = T;

  /// The type of the failure.
  using ErrorType = E;

  /// Constructs a success from an `Ok`, converting the value if needed.
  ///
  /// @param ok The success value, moved in.
  template <typename U> requires ConstructibleFrom<T, U>
  constexpr Result(Ok<U>&& ok) : _value(tare::move(ok.value)), _ok{true} {}

  /// Constructs a failure from an `Err`, converting the error if needed.
  ///
  /// @param err The error, moved in.
  template <typename U> requires ConstructibleFrom<E, U>
  constexpr Result(Err<U>&& err) : _err(tare::move(err.value)), _ok{false} {}

  /// Deleted: copying a non-trivial side has to be asked for with `clone()`.
  ///
  /// @param other The %Result that would have been copied.
  Result(const Result<T, E>& other) = delete;

  /// Deleted: copying a non-trivial side has to be asked for with `clone()`.
  ///
  /// @param other The %Result that would have been copied.
  /// @return Nothing; this overload can't be called.
  Result<T, E>& operator=(const Result<T, E>& other) = delete;

  /// Copies the %Result. Only exists when both sides are trivially copyable;
  /// otherwise use `clone()`.
  ///
  /// @param other The %Result to copy.
  constexpr Result(const Result<T, E>& other) requires AppliesForAll<TriviallyCopyable, T, E> = default;

  /// Copy-assigns the %Result. Only exists when both sides are trivially
  /// copyable; otherwise use `clone()`.
  ///
  /// @param other The %Result to copy.
  /// @return This %Result.
  constexpr Result<T, E>& operator=(const Result<T, E>& other) requires AppliesForAll<TriviallyCopyable, T, E> = default;

  /// Moves the %Result. The source keeps holding its side, moved-from.
  ///
  /// @param other The %Result to move from.
  constexpr Result(Result<T, E>&& other) requires AppliesForAll<TriviallyMoveConstructible, T, E> = default;

  /// @copydoc Result(Result<T, E>&&)
  constexpr Result(Result<T, E>&& other) noexcept requires AppliesForAll<Movable, T, E> && (!AppliesForAll<TriviallyMoveConstructible, T, E>) : _ok{other._ok} {
    if (other._ok) {
      detail::construct_at(__builtin_addressof(_value), tare::move(other._value));
    } else {
      detail::construct_at(__builtin_addressof(_err), tare::move(other._err));
    }
  }

  /// Move-assigns the %Result, destroying whichever side this one held
  /// first. It can switch sides, from success to failure or back.
  ///
  /// @param other The %Result to move from.
  /// @return This %Result.
  constexpr Result<T, E>& operator=(Result<T, E>&& other) requires AppliesForAll<TriviallyMoveAssignableAndDestructible, T, E> = default;

  /// @copydoc operator=(Result<T, E>&&)
  constexpr Result<T, E>& operator=(Result<T, E>&& other) noexcept requires AppliesForAll<Movable, T, E> && (!(AppliesForAll<TriviallyMoveAssignableAndDestructible, T, E>)) {
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

  /// Destroys whichever side is held.
  constexpr ~Result() requires TriviallyDestructible<T> && TriviallyDestructible<E> = default;

  /// @copydoc ~Result()
  constexpr ~Result() {
    destroy();
  }

  /// Builds a success directly from the constructor arguments of `T`.
  ///
  /// Nothing is moved, so this works for immovable types too:
  ///
  /// ```
  /// auto r = Result<Handle, Error>::ok(path);
  /// ```
  ///
  /// @param args The arguments to build the `T` from.
  /// @return A %Result holding the new success value.
  template <typename... Args>
  [[nodiscard]] static constexpr Result<T, E> ok(Args&&... args) {
    return Result<T, E>(OkTag{}, tare::forward<Args>(args)...);
  }

  /// Builds a failure directly from the constructor arguments of `E`.
  ///
  /// @param args The arguments to build the `E` from.
  /// @return A %Result holding the new error.
  template <typename... Args>
  [[nodiscard]] static constexpr Result<T, E> err(Args&&... args) {
    return Result<T, E>(ErrTag{}, tare::forward<Args>(args)...);
  }

  /// Makes an independent copy of whichever side is held. Each side is
  /// copied if it's trivially copyable, and cloned otherwise.
  ///
  /// @return A new %Result holding a copy of this one's side.
  [[nodiscard]] constexpr Result<T, E> clone() const requires Duplicable<T> && Duplicable<E> {
    if (!_ok) {
      return Result<T, E>::err(detail::duplicate(_err));
    }

    return Result<T, E>::ok(detail::duplicate(_value));
  }

  /// @return `true` if this holds a success value.
  [[nodiscard]] constexpr bool is_ok() const noexcept {
    return _ok;
  }

  /// @return `true` if this holds a failure.
  [[nodiscard]] constexpr bool is_err() const noexcept {
    return !_ok;
  }

  /// Reaches the error.
  ///
  /// @pre `is_err()`. Traps otherwise.
  /// @return The error.
  [[nodiscard]] constexpr E& error() & {
    TARE_ASSERT(!_ok);

    return _err;
  }

  /// @copydoc error() &
  [[nodiscard]] constexpr const E& error() const& {
    TARE_ASSERT(!_ok);

    return _err;
  }

  /// Reaches the success value.
  ///
  /// @pre `is_ok()`. Traps otherwise.
  /// @return The success value.
  [[nodiscard]] constexpr T& operator*() & {
    TARE_ASSERT(_ok);

    return _value;
  }

  /// @copydoc operator*() &
  [[nodiscard]] constexpr const T& operator*() const& {
    TARE_ASSERT(_ok);

    return _value;
  }

  /// Reaches the success value of a temporary %Result, so it can be moved
  /// from.
  ///
  /// @pre `is_ok()`. Traps otherwise.
  /// @return The success value, ready to be moved from.
  [[nodiscard]] constexpr T&& operator*() && {
    TARE_ASSERT(_ok);

    return tare::move(_value);
  }

  /// Reaches a member of the success value.
  ///
  /// @pre `is_ok()`. Traps otherwise.
  /// @return A pointer to the success value.
  [[nodiscard]] constexpr T* operator->() {
    TARE_ASSERT(_ok);

    return __builtin_addressof(_value);
  }

  /// @copydoc operator->()
  [[nodiscard]] constexpr const T* operator->() const {
    TARE_ASSERT(_ok);

    return __builtin_addressof(_value);
  }

  /// Moves the success value out.
  ///
  /// Only callable on an rvalue, so the consumption shows at the call site:
  /// `tare::move(r).unwrap()`.
  ///
  /// @pre `is_ok()`. Traps otherwise.
  /// @return The success value.
  [[nodiscard]] constexpr T unwrap() && {
    TARE_ASSERT(_ok);

    return tare::move(_value);
  }

  /// Moves the error out.
  ///
  /// @pre `is_err()`. Traps otherwise.
  /// @return The error.
  [[nodiscard]] constexpr E unwrap_err() && {
    TARE_ASSERT(!_ok);

    return tare::move(_err);
  }

  /// Moves the success value out, or hands back `value` on failure.
  ///
  /// @param value What to return on failure.
  /// @return The success value, or `value`.
  [[nodiscard]] constexpr T unwrap_or(T value) && {
    if (!_ok) {
      return tare::move(value);
    }

    return tare::move(_value);
  }

  /// Moves the success value out, or hands the error to `f` to make one.
  ///
  /// ```
  /// Result<int, int> r = Err(3);
  /// tare::move(r).unwrap_or_else([](int e) { return e * 10; }); // 30
  /// ```
  ///
  /// @param f Called with the error on failure; returns a `T`.
  /// @return The success value, or whatever `f` returned.
  template <typename TFunction>
  [[nodiscard]] constexpr T unwrap_or_else(TFunction&& f) && {
    if (_ok) {
      return tare::move(_value);
    }

    return tare::forward<TFunction>(f)(tare::move(_err));
  }

  /// Moves the success value into an `Optional`, dropping the error.
  ///
  /// @return The success value, or `None` on failure.
  [[nodiscard]] constexpr Optional<T> ok() && requires Movable<T> {
    if (!_ok) {
      return None;
    }

    return Optional<T>{tare::move(_value)};
  }

  /// Moves the error into an `Optional`, dropping the success value.
  ///
  /// @return The error, or `None` on success.
  [[nodiscard]] constexpr Optional<E> err() && requires Movable<E> {
    if (_ok) {
      return None;
    }

    return Optional<E>{tare::move(_err)};
  }

  /// Transforms the success value with `f`, and passes a failure through
  /// untouched.
  ///
  /// ```
  /// Result<int, Error> r = Ok(4);
  /// tare::move(r).map([](int x) { return x * 2; }); // Ok(8)
  /// ```
  ///
  /// @param f Called with the success value; never called on failure.
  /// @return A %Result of whatever `f` returns, with the same error type.
  template <typename TFunction>
  [[nodiscard]] constexpr auto map(TFunction&& f) && {
    using U = RemoveCvRef<decltype(tare::forward<TFunction>(f)(tare::move(_value)))>;

    if (!_ok) {
      return Result<U, E>::err(tare::move(_err));
    }

    return Result<U, E>::ok(tare::forward<TFunction>(f)(tare::move(_value)));
  }

  /// Transforms the error with `f`, and passes a success through untouched.
  ///
  /// Use it to convert one error type into another, usually before an
  /// `and_then()` that expects a different one.
  ///
  /// @param f Called with the error; never called on success.
  /// @return A %Result with the same success type and whatever error type
  /// `f` returns.
  template <typename TFunction>
  [[nodiscard]] constexpr auto map_err(TFunction&& f) && {
    using U = RemoveCvRef<decltype(tare::forward<TFunction>(f)(tare::move(_err)))>;

    if (_ok) {
      return Result<T, U>::ok(tare::move(_value));
    }

    return Result<T, U>::err(tare::forward<TFunction>(f)(tare::move(_err)));
  }

  /// Chains a step that can itself fail.
  ///
  /// On success, `f` gets the value and returns a new %Result. On failure,
  /// `f` is skipped and the error carries across, so the error type of
  /// `f`'s %Result has to be buildable from `E`.
  ///
  /// @param f Called with the success value; returns a %Result.
  /// @return Whatever `f` returned, or the carried-over error.
  template <typename TFunction>
  [[nodiscard]] constexpr auto and_then(TFunction&& f) && {
    using Output = RemoveCvRef<decltype(tare::forward<TFunction>(f)(tare::move(_value)))>;

    static_assert(detail::is_result_v<Output>, "and_then callback must return a Result");

    if (!_ok) {
      return Output::err(tare::move(_err));
    }

    return tare::forward<TFunction>(f)(tare::move(_value));
  }

  /// Gives a failure a chance to recover.
  ///
  /// On failure, `f` gets the error and returns a new %Result, which may
  /// use a different error type. On success, `f` is skipped and the value
  /// carries across, so the success type of `f`'s %Result has to be
  /// buildable from `T`.
  ///
  /// @param f Called with the error; returns a %Result.
  /// @return Whatever `f` returned, or the carried-over success value.
  template <typename TFunction>
  [[nodiscard]] constexpr auto or_else(TFunction&& f) && {
    using Output = RemoveCvRef<decltype(tare::forward<TFunction>(f)(tare::move(_err)))>;

    static_assert(detail::is_result_v<Output>, "or_else callback must return a Result");

    if (_ok) {
      return Output::ok(tare::move(_value));
    }

    return tare::forward<TFunction>(f)(tare::move(_err));
  }

  /// Equal when both are on the same side with equal contents. A success
  /// never equals a failure, even when `T` and `E` are the same type.
  ///
  /// Only exists when both `T` and `E` have `==`.
  template <typename U = T, typename V = E>
  [[nodiscard]] friend constexpr bool operator==(const Result<T, E>& a, const Result<T, E>& b) requires requires(const U& x, const V& y) { x == x; y == y; } {
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