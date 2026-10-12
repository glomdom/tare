// SPDX-License-Identifier: BSL-1.0

#pragma once

#include <compare> // IWYU pragma: keep
#include <new> // IWYU pragma: keep

#include <tare/traits.hpp>

#ifndef TARE_ASSERT
    #define TARE_ASSERT(cond)  \
        do {                   \
            if (!(cond)) {     \
                ::abort();     \
            }                  \
        } while (false)
#endif

namespace tare {

enum class NoneT {
    None
};

inline constexpr NoneT None = NoneT::None;

struct InPlaceT {
    explicit constexpr InPlaceT() = default;
};

inline constexpr InPlaceT InPlace{};

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

/// A value that may or may not be present.
///
/// Prefer this to a sentinel value, or a `T*` that might be null.
///
/// Copying is the following: free for trivially copyable `T`, otherwise
/// `clone()`. Moving leaves the source *engaged* with a moved-from `T` inside,
/// matching the a non-destructive move; use `take()` when you want the source
/// emptied instead.
///
/// Every special member is declared as a disjoint pair: a defaulted one when
/// the operation is trivial for `T`, and a hand-written one otherwise. The
/// constraints must not overlap, or both are eligible and the class loses
/// triviality.
template <typename T>
class Optional {
public:
    using ValueType = T;

    constexpr Optional() noexcept : _empty{}, _has{false} {}
    constexpr Optional(NoneT) noexcept : _empty{}, _has{false} {}
    constexpr Optional(T value) requires Movable<T>
        : _value{tare::move(value)}, _has{true} {}

    template <typename... Args>
        requires ConstructibleFrom<T, Args...>
    constexpr explicit Optional(InPlaceT, Args&&... args)
        : _value{tare::forward<Args>(args)...}, _has{true} {}

    constexpr Optional(const Optional&) requires TriviallyCopyable<T> = default;
    constexpr Optional& operator=(const Optional&) requires TriviallyCopyable<T> = default;

    Optional(const Optional&) = delete;
    Optional& operator=(const Optional&) = delete;

    [[nodiscard]] constexpr Optional clone() const requires Cloneable<T> {
        if (!_has) {
            return Optional{};
        }

        return Optional{InPlace, _value.clone()};
    }

    constexpr Optional(Optional&&) requires TriviallyMoveConstructible<T> = default;
    constexpr Optional(Optional&& other) noexcept requires Movable<T> && (!TriviallyMoveConstructible<T>) : _empty{}, _has{false} {
        if (other._has) {
            detail::construct_at(__builtin_addressof(_value), tare::move(other._value));
            _has = true;
        }
    }

    constexpr Optional& operator=(Optional&&) requires TriviallyMoveAssignable<T> && TriviallyDestructible<T> = default;
    constexpr Optional& operator=(Optional&& other) noexcept requires Movable<T> && (!(TriviallyMoveAssignable<T> && TriviallyDestructible<T>)) {
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

    constexpr Optional& operator=(NoneT) noexcept {
        reset();

        return *this;
    }

    constexpr ~Optional() requires TriviallyDestructible<T> = default;
    constexpr ~Optional() {
        reset();
    }

    [[nodiscard]] constexpr bool has_value() const noexcept {
        return _has;
    }

    [[nodiscard]] constexpr explicit operator bool() const noexcept {
        return _has;
    }

    [[nodiscard]] constexpr T& operator*() & {
        TARE_ASSERT(_has);

        return _value;
    }

    [[nodiscard]] constexpr const T& operator*() const& {
        TARE_ASSERT(_has);

        return _value;
    }

    [[nodiscard]] constexpr T&& operator*() && {
        TARE_ASSERT(_has);

        return tare::move(_value);
    }

    [[nodiscard]] constexpr T* operator->() {
        TARE_ASSERT(_has);

        return __builtin_addressof(_value);
    }

    [[nodiscard]] constexpr const T* operator->() const {
        TARE_ASSERT(_has);

        return __builtin_addressof(_value);
    }

    [[nodiscard]] constexpr T unwrap() && {
        TARE_ASSERT(_has);

        return tare::move(_value);
    }

    [[nodiscard]] constexpr T unwrap_or(T fallback) && {
        if (!_has) {
            return tare::move(fallback);
        }

        return tare::move(_value);
    }

    template <typename TFunction>
    [[nodiscard]] constexpr T unwrap_or_else(TFunction&& f) && {
        if (!_has) {
            return tare::forward<TFunction>(f)();
        }

        return tare::move(_value);
    }

    /// Applies `f` to the value and wraps the result.
    template <typename TFunction>
    [[nodiscard]] constexpr auto map(TFunction&& f) && {
        using U = RemoveCvRef<decltype(tare::forward<TFunction>(f)(tare::move(_value)))>;

        if (!_has) {
            return Optional<U>{};
        }

        return Optional<U>{tare::forward<TFunction>(f)(tare::move(_value))};
    }

    /// Like `map`, but `f` already returns an `Optional` — no double wrapping.
    template <typename TFunction>
    [[nodiscard]] constexpr auto and_then(TFunction&& f) && {
        using Result = RemoveCvRef<decltype(tare::forward<TFunction>(f)(tare::move(_value)))>;

        static_assert(detail::is_optional_v<Result>, "and_then callback must return an Optional");

        if (!_has) {
            return Result{};
        }

        return tare::forward<TFunction>(f)(tare::move(_value));
    }

    template <typename TFunction>
    [[nodiscard]] constexpr Optional or_else(TFunction&& f) && requires Movable<T> {
        if (_has) {
            return Optional{tare::move(_value)};
        }

        return tare::forward<TFunction>(f)();
    }

    template <typename TPredicate>
    [[nodiscard]] constexpr Optional filter(TPredicate&& p) && requires Movable<T> {
        if (_has && tare::forward<TPredicate>(p)(_value)) {
            return Optional{tare::move(_value)};
        }

        return Optional{};
    }

    template <typename... Args> requires ConstructibleFrom<T, Args...>
    constexpr T& emplace(Args&&... args) {
        reset();
        detail::construct_at(__builtin_addressof(_value), tare::forward<Args>(args)...);
        _has = true;

        return _value;
    }

    constexpr T& get_or_insert(T value) requires Movable<T> {
        if (!_has) {
            emplace(tare::move(value));
        }

        return _value;
    }

    [[nodiscard]] constexpr Optional take() noexcept requires Movable<T> {
        if (!_has) {
            return Optional{};
        }

        Optional result{tare::move(_value)};
        reset();

        return result;
    }

    constexpr Optional replace(T value) requires Movable<T> {
        Optional old = take();
        emplace(tare::move(value));

        return old;
    }

    constexpr void reset() noexcept {
        if (_has) {
            detail::destroy_at(__builtin_addressof(_value));
            _has = false;
        }
    }

    template <typename U = T>
    [[nodiscard]] friend constexpr bool operator==(const Optional& a, const Optional& b) requires requires(const U& x, const U& y) { x == y; } {
        if (a._has != b._has) {
            return false;
        }

        if (!a._has) {
            return true;
        }

        return a._value == b._value;
    }

    template <typename U = T>
    [[nodiscard]] friend constexpr auto operator<=>(const Optional& a, const Optional& b) requires requires(const U& x, const U& y) { x <=> y; } {
        using Ordering = decltype(declval<const U&>() <=> declval<const U&>());

        if (a._has && b._has) {
            return static_cast<Ordering>(a._value <=> b._value);
        }

        return static_cast<Ordering>(a._has <=> b._has);
    }

    [[nodiscard]] friend constexpr bool operator==(const Optional& a, NoneT) noexcept {
        return !a._has;
    }

    [[nodiscard]] friend constexpr auto operator<=>(const Optional& a, NoneT) noexcept {
        return a._has <=> false;
    }

private:
    struct Empty {};

    union {
        Empty _empty;
        T _value;
    };

    bool _has;
};

template <typename T>
Optional(T) -> Optional<T>;

}