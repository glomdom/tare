// SPDX-License-Identifier: BSL-1.0

#pragma once

#include <compare> // IWYU pragma: keep
#include <new> // IWYU pragma: keep

#include <tare/panic.hpp>
#include <tare/traits.hpp>

namespace tare {

/// The type of `None`.
enum class NoneT {
  None
};

/// The empty value for any `Optional`.
///
/// Assign or compare against it to mean "nothing here":
///
/// ```
/// Optional<int> x = None;
/// x = None;
///
/// if (x == None) { ... }
/// ```
inline constexpr NoneT None = NoneT::None;

template <typename T>
class Optional;

namespace detail {

template <typename T>
inline constexpr bool is_optional_v = false;

template <typename T>
inline constexpr bool is_optional_v<Optional<T>> = true;

template <typename T, typename... Args>
constexpr T* construct_at(T* p, Args&&... args) {
  return ::new (static_cast<void*>(p)) T(tare::forward<Args>(args)...);
}

template <typename T>
constexpr void destroy_at(T* p) {
  p->~T();
}

}

/// Either holds a `T` or holds nothing.
///
/// Reach for this instead of a magic sentinel value or a `T*` that might be
/// null.
///
/// To build the value directly from its constructor arguments, use `some()`.
/// That's also the only way to get an immovable type in.
///
/// Copies are free when `T` is trivially copyable. Otherwise, you have to ask
/// for one explicitly with `clone()`. Moving out of an %Optional doesn't
/// empty it: the source still holds a `T`, just a moved-from one, the same as
/// any other non-destructive move. If you want the source left empty, use
/// `take()`.
///
/// ```
/// Optional<int> x = 5;
/// Optional<int> y = None;
///
/// tare::move(x).unwrap_or(0); // 5
/// tare::move(y).unwrap_or(0); // 0
/// ```
///
/// @tparam T The type of the value it may hold.
///
/// @sa Result
template <typename T>
class Optional {
public:
  /// The type of the value it may hold.
  using ValueType = T;

  /// Constructs an empty %Optional.
  constexpr Optional() noexcept : _empty{}, _has{false} {}

  /// Constructs an empty %Optional, from `None`.
  constexpr Optional(NoneT) noexcept : _empty{}, _has{false} {}

  /// Constructs an %Optional holding `value`, moving it in.
  ///
  /// Implicit on purpose, so `return 5;` works in a function returning
  /// `Optional<int>`.
  ///
  /// @param value The value to move inside.
  constexpr Optional(T value) requires Movable<T> : _value(tare::move(value)), _has{true} {}

  /// Builds an %Optional holding a value made directly from the
  /// constructor arguments of `T`.
  ///
  /// Nothing is moved, so this works for immovable types too:
  ///
  /// ```
  /// auto h = Optional<Handle>::some(path);
  /// ```
  ///
  /// @param args The arguments to build the `T` from.
  /// @return An %Optional holding the new value.
  template <typename... Args> requires ConstructibleFrom<T, Args...>
  [[nodiscard]] static constexpr Optional<T> some(Args&&... args) {
    return Optional<T>(SomeTag{}, tare::forward<Args>(args)...);
  }

  /// Copies the %Optional. Only exists when `T` is trivially copyable;
  /// otherwise use `clone()`.
  constexpr Optional(const Optional<T>&) requires TriviallyCopyable<T> = default;

  /// Copy-assigns the %Optional. Only exists when `T` is trivially copyable;
  /// otherwise use `clone()`.
  ///
  /// @return This %Optional.
  constexpr Optional<T>& operator=(const Optional<T>&) requires TriviallyCopyable<T> = default;

  /// Deleted: copying a non-trivial `T` has to be asked for with `clone()`.
  Optional(const Optional<T>&) = delete;

  /// Deleted: copying a non-trivial `T` has to be asked for with `clone()`.
  Optional<T>& operator=(const Optional<T>&) = delete;

  /// Makes an independent copy, cloning the value if there is one.
  ///
  /// @return A new %Optional holding a clone of the value, or an empty one.
  [[nodiscard]] constexpr Optional<T> clone() const requires Cloneable<T> {
    if (!_has) {
      return Optional<T>{};
    }

    return Optional<T>::some(_value.clone());
  }

  /// Moves the %Optional. The source keeps holding a moved-from value; use
  /// `take()` if you want it emptied.
  ///
  /// @param other The %Optional to move from.
  constexpr Optional(Optional<T>&& other) requires TriviallyMoveConstructible<T> = default;

  /// @copydoc Optional(Optional<T>&&)
  constexpr Optional(Optional<T>&& other) noexcept requires Movable<T> && (!TriviallyMoveConstructible<T>) : _empty{}, _has{false} {
    if (other._has) {
      detail::construct_at(__builtin_addressof(_value), tare::move(other._value));
      _has = true;
    }
  }

  /// Move-assigns the %Optional, destroying whatever this one held first.
  ///
  /// @param other The %Optional to move from.
  /// @return This %Optional.
  constexpr Optional<T>& operator=(Optional<T>&& other) requires TriviallyMoveAssignable<T> && TriviallyDestructible<T> = default;

  /// @copydoc operator=(Optional<T>&&)
  constexpr Optional<T>& operator=(Optional<T>&& other) noexcept requires Movable<T> && (!(TriviallyMoveAssignable<T> && TriviallyDestructible<T>)) {
    if (this == __builtin_addressof(other)) {
      return *this;
    }

    reset();

    if (other._has) {
      detail::construct_at(__builtin_addressof(_value), tare::move(other._value));
      _has = true;
    }

    return *this;
  }

  /// Empties the %Optional, destroying the value if there is one.
  ///
  /// @return This %Optional.
  constexpr Optional<T>& operator=(NoneT) noexcept {
    reset();

    return *this;
  }

  /// Destroys the value if there is one.
  constexpr ~Optional() requires TriviallyDestructible<T> = default;

  /// @copydoc ~Optional()
  constexpr ~Optional() {
    reset();
  }

  /// @return `true` if there is a value.
  [[nodiscard]] constexpr bool has_value() const noexcept {
    return _has;
  }

  /// @return `true` if there is a value. Explicit, so it works in an `if`
  /// but never converts to `bool` by accident.
  [[nodiscard]] constexpr explicit operator bool() const noexcept {
    return _has;
  }

  /// Reaches the value.
  ///
  /// @pre `has_value()`. Traps otherwise.
  /// @return The value.
  [[nodiscard]] constexpr T& operator*() & {
    TARE_ASSERT(_has);

    return _value;
  }

  /// @copydoc operator*() &
  [[nodiscard]] constexpr const T& operator*() const& {
    TARE_ASSERT(_has);

    return _value;
  }

  /// Reaches the value of a temporary %Optional, so it can be moved from.
  ///
  /// @pre `has_value()`. Traps otherwise.
  /// @return The value, ready to be moved from.
  [[nodiscard]] constexpr T&& operator*() && {
    TARE_ASSERT(_has);

    return tare::move(_value);
  }

  /// Reaches a member of the value.
  ///
  /// @pre `has_value()`. Traps otherwise.
  /// @return A pointer to the value.
  [[nodiscard]] constexpr T* operator->() {
    TARE_ASSERT(_has);

    return __builtin_addressof(_value);
  }

  /// @copydoc operator->()
  [[nodiscard]] constexpr const T* operator->() const {
    TARE_ASSERT(_has);

    return __builtin_addressof(_value);
  }

  /// Moves the value out.
  ///
  /// Only callable on an rvalue, so the consumption shows at the call site:
  /// `tare::move(x).unwrap()`.
  ///
  /// @pre `has_value()`. Traps otherwise.
  /// @return The value.
  [[nodiscard]] constexpr T unwrap() && {
    TARE_ASSERT(_has);

    return tare::move(_value);
  }

  /// Moves the value out, or hands back `fallback` if there isn't one.
  ///
  /// @param fallback What to return when empty.
  /// @return The value, or `fallback`.
  [[nodiscard]] constexpr T unwrap_or(T fallback) && {
    if (!_has) {
      return tare::move(fallback);
    }

    return tare::move(_value);
  }

  /// Moves the value out, or calls `f` to make one if there isn't one.
  ///
  /// Unlike `unwrap_or()`, the fallback is only built when it's needed.
  ///
  /// @param f Called with no arguments when empty; returns a `T`.
  /// @return The value, or whatever `f` returned.
  template <typename TFunction>
  [[nodiscard]] constexpr T unwrap_or_else(TFunction&& f) && {
    if (!_has) {
      return tare::forward<TFunction>(f)();
    }

    return tare::move(_value);
  }

  /// Applies `f` to the value and wraps the result.
  ///
  /// ```
  /// Optional<int> x = 4;
  /// tare::move(x).map([](int v) { return v * 2; }); // Optional<int>{8}
  /// ```
  ///
  /// @param f Called with the value when there is one.
  /// @return An %Optional of whatever `f` returns, or an empty one. `f` is
  /// never called when empty.
  template <typename TFunction>
  [[nodiscard]] constexpr auto map(TFunction&& f) && {
    using U = RemoveCvRef<decltype(tare::forward<TFunction>(f)(tare::move(_value)))>;

    if (!_has) {
      return Optional<U>{};
    }

    return Optional<U>{tare::forward<TFunction>(f)(tare::move(_value))};
  }

  /// Like `map()`, but `f` already returns an %Optional, so nothing gets
  /// double-wrapped.
  ///
  /// Use it to chain steps that might each come up empty.
  ///
  /// @param f Called with the value when there is one; returns an %Optional.
  /// @return Whatever `f` returned, or an empty %Optional. `f` is never
  /// called when empty.
  template <typename TFunction>
  [[nodiscard]] constexpr auto and_then(TFunction&& f) && {
    using Result = RemoveCvRef<decltype(tare::forward<TFunction>(f)(tare::move(_value)))>;

    static_assert(detail::is_optional_v<Result>, "and_then callback must return an Optional");

    if (!_has) {
      return Result{};
    }

    return tare::forward<TFunction>(f)(tare::move(_value));
  }

  /// Keeps the value, or calls `f` for a replacement %Optional if empty.
  ///
  /// @param f Called with no arguments when empty; returns an `Optional<T>`.
  /// @return This %Optional if it has a value, otherwise whatever `f`
  /// returned.
  template <typename TFunction>
  [[nodiscard]] constexpr Optional<T> or_else(TFunction&& f) && requires Movable<T> {
    if (_has) {
      return Optional<T>{tare::move(_value)};
    }

    return tare::forward<TFunction>(f)();
  }

  /// Keeps the value only if it passes `p`.
  ///
  /// @param p Called with the value; returns whether to keep it.
  /// @return This %Optional if `p` says yes, otherwise an empty one.
  template <typename TPredicate>
  [[nodiscard]] constexpr Optional<T> filter(TPredicate&& p) && requires Movable<T> {
    if (_has && tare::forward<TPredicate>(p)(_value)) {
      return Optional<T>{tare::move(_value)};
    }

    return Optional<T>{};
  }

  /// Builds a new value in place from `args`, destroying the old one first.
  ///
  /// @param args The arguments to build the `T` from.
  /// @return The new value.
  template <typename... Args> requires ConstructibleFrom<T, Args...>
  constexpr T& emplace(Args&&... args) {
    reset();
    detail::construct_at(__builtin_addressof(_value), tare::forward<Args>(args)...);

    _has = true;

    return _value;
  }

  /// Fills the %Optional with `value` if it's empty.
  ///
  /// @param value Moved in only when empty; dropped otherwise.
  /// @return The value now inside, old or new.
  constexpr T& get_or_insert(T value) requires Movable<T> {
    if (!_has) {
      emplace(tare::move(value));
    }

    return _value;
  }

  /// Moves the value out and leaves this %Optional empty.
  ///
  /// The way to move a value out of a field without leaving a moved-from
  /// husk behind.
  ///
  /// @return What was inside, or an empty %Optional.
  [[nodiscard]] constexpr Optional<T> take() noexcept requires Movable<T> {
    if (!_has) {
      return Optional<T>{};
    }

    Optional<T> result{tare::move(_value)};
    reset();

    return result;
  }

  /// Puts `value` in and hands back what was there before.
  ///
  /// @param value The new value.
  /// @return The old value, or an empty %Optional.
  constexpr Optional<T> replace(T value) requires Movable<T> {
    Optional<T> old = take();
    emplace(tare::move(value));

    return old;
  }

  /// Empties the %Optional, destroying the value if there is one.
  constexpr void reset() noexcept {
    if (_has) {
      detail::destroy_at(__builtin_addressof(_value));
      _has = false;
    }
  }

  /// Equal when both are empty, or both hold equal values.
  ///
  /// Only exists when `T` has `==`.
  template <typename U = T>
  [[nodiscard]] friend constexpr bool operator==(const Optional<T>& a, const Optional<T>& b) requires requires(const U& x, const U& y) { x == y; } {
    if (a._has != b._has) {
      return false;
    }

    if (!a._has) {
      return true;
    }

    return a._value == b._value;
  }

  /// Orders by value, with an empty %Optional before any value.
  ///
  /// Only exists when `T` has `<=>`.
  template <typename U = T>
  [[nodiscard]] friend constexpr auto operator<=>(const Optional<T>& a, const Optional<T>& b) requires requires(const U& x, const U& y) { x <=> y; } {
    using Ordering = decltype(declval<const U&>() <=> declval<const U&>());

    if (a._has && b._has) {
      return static_cast<Ordering>(a._value <=> b._value);
    }

    return static_cast<Ordering>(a._has <=> b._has);
  }

  /// `true` when the %Optional is empty.
  [[nodiscard]] friend constexpr bool operator==(const Optional<T>& a, NoneT) noexcept {
    return !a._has;
  }

  /// Orders against `None`: equal when empty, greater when holding a value.
  [[nodiscard]] friend constexpr auto operator<=>(const Optional<T>& a, NoneT) noexcept {
    return a._has <=> false;
  }

private:
  struct Empty {};
  struct SomeTag {};

  template <typename... Args>
  constexpr Optional(SomeTag, Args&&... args) : _value(tare::forward<Args>(args)...), _has{true} {}

  union {
    Empty _empty;
    T _value;
  };

  bool _has;
};

template <typename T>
Optional(T) -> Optional<T>;

}