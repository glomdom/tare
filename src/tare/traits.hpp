// SPDX-License-Identifier: BSL-1.0

#pragma once

namespace tare {

/// @brief Strips a reference off `T`. Use the `RemoveReference` alias instead of
///        this directly.
///
/// @tparam T The type to strip.
template <typename T>
struct RemoveReferenceT {
  /// `T` without its reference.
  using Type = T;
};

/// @copydoc RemoveReferenceT
template <typename T>
struct RemoveReferenceT<T&> {
  /// `T` without its reference.
  using Type = T;
};

/// @copydoc RemoveReferenceT
template <typename T>
struct RemoveReferenceT<T&&> {
  /// `T` without its reference.
  using Type = T;
};

/// @brief `T` with any `&` or `&&` stripped off.
template <typename T>
using RemoveReference = typename RemoveReferenceT<T>::Type;

/// @brief Strips `const` and `volatile` off `T`. Use the `RemoveCv` alias instead
///        of this directly.
///
/// @tparam T The type to strip.
template <typename T>
struct RemoveCvT {
  /// `T` without `const` or `volatile`.
  using Type = T;
};

/// @copydoc RemoveCvT
template <typename T>
struct RemoveCvT<const T> {
  /// `T` without `const` or `volatile`.
  using Type = T;
};

/// @copydoc RemoveCvT
template <typename T>
struct RemoveCvT<volatile T> {
  /// `T` without `const` or `volatile`.
  using Type = T;
};

/// @copydoc RemoveCvT
template <typename T>
struct RemoveCvT<const volatile T> {
  /// `T` without `const` or `volatile`.
  using Type = T;
};

/// @brief `T` with top-level `const` and `volatile` stripped off.
template <typename T>
using RemoveCv = typename RemoveCvT<T>::Type;

/// @brief `T` with its reference stripped, then `const` and `volatile`, meaning
///        `const int&` becomes `int`.
template <typename T>
using RemoveCvRef = RemoveCv<RemoveReference<T>>;

/// @brief Turns `value` into an rvalue. 
///
/// @param value The object.
/// @return `value`, as an rvalue reference.
template <typename T>
[[nodiscard]] constexpr RemoveReference<T>&& move(T&& value) noexcept {
  return static_cast<RemoveReference<T>&&>(value);
}

/// @brief Passes an argument on exactly as it was received: as an lvalue if the
///        caller passed an lvalue, as an rvalue if they passed an rvalue.
///
/// @param value The argument to pass on.
/// @return `value`, as whatever kind of reference it is.
template <typename T>
[[nodiscard]] constexpr T&& forward(RemoveReference<T>& value) noexcept {
  return static_cast<T&&>(value);
}

/// @copydoc forward(RemoveReference<T>&)
template <typename T>
[[nodiscard]] constexpr T&& forward(RemoveReference<T>&& value) noexcept {
  return static_cast<T&&>(value);
}

/// @brief Names a value of type `T` without constructing one.
///
/// @warning Never defined, so it only works where nothing gets evaluated.
///
/// @return A `T&&`, in the type system only.
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

/// @brief `T` and `U` are exactly the same type.
///
/// @note Checked in both directions.
template <typename T, typename U>
concept SameAs = detail::SameAsImpl<T, U> && detail::SameAsImpl<U, T>;

/// @brief A `T` can be built from `Args`, with the same rules as `T(args...)`.
template <typename T, typename... Args>
concept ConstructibleFrom = __is_constructible(T, Args...);

/// @brief A `T` can be built by moving from another `T`.
template <typename T>
concept Movable = __is_constructible(T, T&&);

/// @brief A `T` can be built by copying another `T`.
///
/// @note Most tare types usually aren't, even when they could be: see
///       `Duplicable` for what it means by "can be duplicated".
template <typename T>
concept Copyable = __is_constructible(T, const T&);

/// @brief Copying a `T` is just copying its bytes, like `memcpy`.
template <typename T>
concept TriviallyCopyable = __is_trivially_copyable(T);

/// @brief Destroying a `T` does nothing, so no destructor needs to run.
template <typename T>
concept TriviallyDestructible = __is_trivially_destructible(T);

/// @brief Moving a `T` into a new object is just copying its bytes.
template <typename T>
concept TriviallyMoveConstructible = __is_trivially_constructible(T, T&&);

/// @brief Move-assigning a `T` is just copying its bytes.
template <typename T>
concept TriviallyMoveAssignable = __is_trivially_assignable(T&, T&&);

/// @brief Move-assigning a `T` is just copying its bytes, with no destructor to run
///        on the old value first.
template <typename T>
concept TriviallyMoveAssignableAndDestructible = TriviallyMoveAssignable<T> && TriviallyDestructible<T>;

/// @brief Every type in `Ts` satisfies `Concept`.
///
/// ```
/// AppliesForAll<TriviallyCopyable, T, E>
/// // means TriviallyCopyable<T> && TriviallyCopyable<E>
/// ```
template <template <typename> concept Concept, typename... Ts>
concept AppliesForAll = (Concept<Ts> && ...);

/// @brief Copying a `T` into a new object is just copying its bytes.
template <typename T>
concept TriviallyCopyConstructible = __is_trivially_constructible(T, const T&);

/// @brief Copy-assigning a `T` is just copying its bytes.
template <typename T>
concept TriviallyCopyAssignable = __is_trivially_assignable(T&, const T&);

/// @brief A `T` opts into being duplicated by providing a `clone()` that returns a
///        new `T`.
///
/// @attention This is a tare convention, not a C++ one. Nothing in tare will
///            deep-copy implicitly. A type is duplicable when it says how.
template <typename T>
concept Cloneable = requires(const T& t) {
  { t.clone() } -> SameAs<T>;
};

/// @brief A `T` can be duplicated, either because copying it is free or because it
///        provides `clone()`.
template <typename T>
concept Duplicable = TriviallyCopyable<T> || Cloneable<T>;

}