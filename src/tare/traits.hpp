// SPDX-License-Identifier: BSL-1.0

#pragma once

namespace tare {

template <typename T>
struct RemoveReferenceT {
    using Type = T;
};

template <typename T>
struct RemoveReferenceT<T&> {
    using Type = T;
};

template <typename T>
struct RemoveReferenceT<T&&> {
    using Type = T;
};

template <typename T>
using RemoveReference = typename RemoveReferenceT<T>::Type;

template <typename T>
struct RemoveCvT {
    using Type = T;
};

template <typename T>
struct RemoveCvT<const T> {
    using Type = T;
};

template <typename T>
struct RemoveCvT<volatile T> {
    using Type = T;
};

template <typename T>
struct RemoveCvT<const volatile T> {
    using Type = T;
};

template <typename T>
using RemoveCv = typename RemoveCvT<T>::Type;

template <typename T>
using RemoveCvRef = RemoveCv<RemoveReference<T>>;

template <typename T>
[[nodiscard]] constexpr RemoveReference<T>&& move(T&& value) noexcept {
    return static_cast<RemoveReference<T>&&>(value);
}

template <typename T>
[[nodiscard]] constexpr T&& forward(RemoveReference<T>& value) noexcept {
    return static_cast<T&&>(value);
}

template <typename T>
[[nodiscard]] constexpr T&& forward(RemoveReference<T>&& value) noexcept {
    return static_cast<T&&>(value);
}

template <typename T>
RemoveReference<T>&& declval() noexcept;

namespace detail {

template <typename T, typename U>
inline constexpr bool is_same_v = false;

template <typename T>
inline constexpr bool is_same_v<T, T> = true;

template <typename T, typename U>
concept SameAsImpl = is_same_v<T, U>;

}

template <typename T, typename U>
concept SameAs = detail::SameAsImpl<T, U> && detail::SameAsImpl<U, T>;

template <typename T, typename... Args>
concept ConstructibleFrom = __is_constructible(T, Args...);

template <typename T>
concept Movable = __is_constructible(T, T&&);

template <typename T>
concept Copyable = __is_constructible(T, const T&);

template <typename T>
concept TriviallyCopyable = __is_trivially_copyable(T);

template <typename T>
concept TriviallyDestructible = __is_trivially_destructible(T);

template <typename T>
concept TriviallyMoveConstructible = __is_trivially_constructible(T, T&&);

template <typename T>
concept TriviallyMoveAssignable = __is_trivially_assignable(T&, T&&);

template <typename T>
concept TriviallyCopyConstructible = __is_trivially_constructible(T, const T&);

template <typename T>
concept TriviallyCopyAssignable = __is_trivially_assignable(T&, const T&);

template <typename T>
concept Cloneable = requires(const T& t) {
    { t.clone() } -> SameAs<T>;
};

template <typename T>
concept Duplicable = TriviallyCopyable<T> || Cloneable<T>;

}