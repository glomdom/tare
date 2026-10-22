// SPDX-License-Identifier: BSL-1.0

#include <tare/optional.hpp>

#include <gtest/gtest.h>

#include "test_types.hpp"

using namespace tare;
using namespace test_types;

namespace {

constexpr bool whole_api_in_constant_evaluation() {
  Optional<int> a = 4;

  auto doubled = tare::move(a).map([](int x) { return x * 2; });

  if (*doubled != 8) {
    return false;
  }

  Optional<int> b = 1;
  Optional<int> taken = b.take();

  if (b.has_value() || *taken != 1) {
    return false;
  }

  int destroyed = 0;

  {
    auto t = Optional<Tracked>::some(&destroyed);
    t.emplace(&destroyed);
  }

  return destroyed == 2;
}

}

TEST(OptionalLayout, KeepsIntTriviallyCopyable) {
  static_assert(TriviallyCopyable<Optional<int>>);
  static_assert(TriviallyMoveConstructible<Optional<int>>);
}

TEST(OptionalLayout, KeepsIntTriviallyDestructible) {
  static_assert(TriviallyDestructible<Optional<int>>);
}

TEST(OptionalLayout, CostsOneWordBeyondT) {
  static_assert(sizeof(Optional<int>) == sizeof(int) * 2);
  static_assert(sizeof(Optional<char>) == std::size_t{2});
}

TEST(OptionalLayout, GoesNonTrivialExactlyWhenTDoes) {
  static_assert(!TriviallyDestructible<Optional<Tracked>>);
  static_assert(Movable<Optional<Tracked>>);
}

TEST(OptionalLayout, RefusesToMoveOverAnImmovableType) {
  static_assert(!Movable<Optional<Immovable>>);
}

TEST(OptionalLayout, RecognisesCloneOnlyWhereItExists) {
  static_assert(Cloneable<Duplicate>);
  static_assert(!Cloneable<int>);
}

TEST(OptionalConstruction, IsEmptyByDefault) {
  Optional<int> a;

  EXPECT_FALSE(a.has_value());
  EXPECT_FALSE(static_cast<bool>(a));
  EXPECT_TRUE(a == None);
}

TEST(OptionalConstruction, IsEmptyWhenBuiltFromNone) {
  Optional<int> a = None;

  EXPECT_FALSE(a.has_value());
}

TEST(OptionalConstruction, HoldsAValueBuiltFromOne) {
  Optional<int> a = 5;

  EXPECT_TRUE(a.has_value());
  EXPECT_EQ(*a, 5);
  EXPECT_TRUE(a != None);
}

TEST(OptionalConstruction, BuildsThroughSomeFromArguments) {
  auto a = Optional<Duplicate>::some(7);

  EXPECT_TRUE(a.has_value());
  EXPECT_EQ(a->v, 7);
}

TEST(OptionalConstruction, BuildsAnImmovableTypeThroughSome) {
  auto a = Optional<Immovable>::some(9);

  EXPECT_TRUE(a.has_value());
  EXPECT_EQ(a->v, 9);
}

TEST(OptionalConstruction, StillCopiesWhenCopyingIsFree) {
  Optional<int> a = 5;
  Optional<int> b = a;

  EXPECT_EQ(*b, 5);
  EXPECT_EQ(*a, 5);
}

TEST(OptionalConstruction, DuplicatesThroughCloneWhenItIsNot) {
  auto a = Optional<Duplicate>::some(3);
  Optional<Duplicate> b = a.clone();

  b->v = 4;

  EXPECT_EQ(a->v, 3);
  EXPECT_EQ(b->v, 4);
}

TEST(OptionalConstruction, ClonesAnEmptyOptionalToAnEmptyOne) {
  Optional<Duplicate> a = None;

  EXPECT_FALSE(a.clone().has_value());
}

TEST(OptionalAccess, MovesTheValueOutOnUnwrap) {
  Optional<int> a = 42;

  EXPECT_EQ(tare::move(a).unwrap(), 42);
}

TEST(OptionalAccess, FallsBackOnAnEmptyUnwrapOr) {
  Optional<int> empty = None;
  Optional<int> full = 1;

  EXPECT_EQ(tare::move(empty).unwrap_or(99), 99);
  EXPECT_EQ(tare::move(full).unwrap_or(99), 1);
}

TEST(OptionalAccess, OnlyCallsTheFallbackWhenEmpty) {
  int calls = 0;

  auto make = [&calls] {
    calls += 1;

    return 7;
  };

  Optional<int> empty = None;
  Optional<int> full = 1;

  EXPECT_EQ(tare::move(empty).unwrap_or_else(make), 7);
  EXPECT_EQ(tare::move(full).unwrap_or_else(make), 1);
  EXPECT_EQ(calls, 1);
}

TEST(OptionalAccess, ReachesMembersThroughTheArrow) {
  auto a = Optional<Duplicate>::some(11);

  EXPECT_EQ(a->v, 11);
}

TEST(OptionalTransforms, WrapsWhatMapReturns) {
  Optional<int> a = 4;
  auto doubled = tare::move(a).map([](int x) { return x * 2; });

  EXPECT_TRUE(doubled.has_value());
  EXPECT_EQ(*doubled, 8);
}

TEST(OptionalTransforms, SkipsMapOnAnEmptyOptional) {
  Optional<int> empty = None;
  int calls = 0;

  auto r = tare::move(empty).map([&calls](int x) {
    calls += 1;

    return x;
  });

  EXPECT_FALSE(r.has_value());
  EXPECT_EQ(calls, 0);
}

TEST(OptionalTransforms, ChangesTheWrappedTypeThroughMap) {
  Optional<int> a = 3;
  auto as_char = tare::move(a).map([](int x) { return static_cast<char>('a' + x); });

  static_assert(SameAs<decltype(as_char), Optional<char>>);
  EXPECT_EQ(*as_char, 'd');
}

TEST(OptionalTransforms, DoesNotDoubleWrapInAndThen) {
  Optional<int> a = 4;

  auto r = tare::move(a).and_then([](int x) {
    return Optional<int>{x * 10};
  });

  static_assert(SameAs<decltype(r), Optional<int>>);
  EXPECT_EQ(*r, 40);
}

TEST(OptionalTransforms, ShortCircuitsAndThenOnEmpty) {
  Optional<int> empty = None;
  int calls = 0;

  auto r = tare::move(empty).and_then([&calls](int x) {
    calls += 1;

    return Optional<int>{x};
  });

  EXPECT_FALSE(r.has_value());
  EXPECT_EQ(calls, 0);
}

TEST(OptionalTransforms, SuppliesADefaultThroughOrElse) {
  Optional<int> empty = None;
  Optional<int> full = 1;

  EXPECT_EQ(*tare::move(empty).or_else([] { return Optional<int>{5}; }), 5);
  EXPECT_EQ(*tare::move(full).or_else([] { return Optional<int>{5}; }), 1);
}

TEST(OptionalTransforms, DropsValuesThatFailTheFilter) {
  auto is_even = [](int x) { return x % 2 == 0; };

  Optional<int> even = 4;
  Optional<int> odd = 3;

  auto kept = tare::move(even).filter(is_even);
  auto dropped = tare::move(odd).filter(is_even);

  EXPECT_EQ(*kept, 4);
  EXPECT_FALSE(dropped.has_value());
}

TEST(OptionalMutation, ReplacesTheContentsOnEmplace) {
  Optional<Duplicate> a = None;

  a.emplace(2);
  EXPECT_EQ(a->v, 2);

  a.emplace(8);
  EXPECT_EQ(a->v, 8);
}

TEST(OptionalMutation, OnlyFillsAnEmptyOptionalOnGetOrInsert) {
  Optional<int> a = None;
  Optional<int> b = 1;

  EXPECT_EQ(a.get_or_insert(5), 5);
  EXPECT_EQ(b.get_or_insert(5), 1);
  EXPECT_EQ(*a, 5);
}

TEST(OptionalMutation, EmptiesTheSourceOnTake) {
  Optional<int> a = 7;
  Optional<int> b = a.take();

  EXPECT_FALSE(a.has_value());
  EXPECT_EQ(*b, 7);
}

TEST(OptionalMutation, TakesNothingFromAnEmptyOptional) {
  Optional<int> a = None;
  Optional<int> b = a.take();

  EXPECT_FALSE(a.has_value());
  EXPECT_FALSE(b.has_value());
}

TEST(OptionalMutation, HandsBackTheOldValueOnReplace) {
  Optional<int> a = 1;
  Optional<int> old = a.replace(2);

  EXPECT_EQ(*a, 2);
  EXPECT_EQ(*old, 1);
}

TEST(OptionalLifetime, DestroysTheValueOnReset) {
  int destroyed = 0;

  {
    auto a = Optional<Tracked>::some(&destroyed);
    a.reset();

    EXPECT_EQ(destroyed, 1);
    EXPECT_FALSE(a.has_value());
  }

  EXPECT_EQ(destroyed, 1);
}

TEST(OptionalLifetime, DestroysTheValueAtScopeExit) {
  int destroyed = 0;

  {
    auto a = Optional<Tracked>::some(&destroyed);
  }

  EXPECT_EQ(destroyed, 1);
}

TEST(OptionalLifetime, DestroysNothingWhenEmpty) {
  int destroyed = 0;

  {
    Optional<Tracked> a = None;
  }

  EXPECT_EQ(destroyed, 0);
}

TEST(OptionalLifetime, DestroysThePreviousValueOnEmplace) {
  int destroyed = 0;

  {
    auto a = Optional<Tracked>::some(&destroyed);
    a.emplace(&destroyed);

    EXPECT_EQ(destroyed, 1);
  }

  EXPECT_EQ(destroyed, 2);
}

TEST(OptionalLifetime, DestroysTheValueOnAssignmentFromNone) {
  int destroyed = 0;

  {
    auto a = Optional<Tracked>::some(&destroyed);
    a = None;

    EXPECT_EQ(destroyed, 1);
    EXPECT_FALSE(a.has_value());
  }

  EXPECT_EQ(destroyed, 1);
}

TEST(OptionalLifetime, MovesOwnershipWithoutDestroyingTwice) {
  int destroyed = 0;

  {
    auto a = Optional<Tracked>::some(&destroyed);
    Optional<Tracked> b = tare::move(a);

    EXPECT_EQ(destroyed, 0);
  }

  EXPECT_EQ(destroyed, 1);
}

TEST(OptionalComparison, ComparesContentsForEquality) {
  Optional<int> a = 1;
  Optional<int> b = 1;
  Optional<int> c = 2;

  EXPECT_TRUE(a == b);
  EXPECT_TRUE(a != c);
}

TEST(OptionalComparison, TreatsEmptyAsUnequalToAnyValue) {
  Optional<int> a = 1;
  Optional<int> e = None;

  EXPECT_TRUE(a != e);
  EXPECT_TRUE(e == Optional<int>{None});
}

TEST(OptionalComparison, SortsEmptyBeforeAnyValue) {
  Optional<int> e = None;
  Optional<int> a = 1;
  Optional<int> b = 2;

  EXPECT_TRUE(e < a);
  EXPECT_TRUE(a < b);
  EXPECT_FALSE(a < e);
}

TEST(OptionalConstantEvaluation, RunsTheWholeApiAtCompileTime) {
  static_assert(whole_api_in_constant_evaluation());
  EXPECT_TRUE(whole_api_in_constant_evaluation());
}