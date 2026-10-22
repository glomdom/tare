// SPDX-License-Identifier: BSL-1.0

#include <tare/optional.hpp>
#include <tare/result.hpp>

#include <gtest/gtest.h>

#include "test_types.hpp"

using namespace tare;
using namespace test_types;

namespace {

enum class Error {
  Empty,
  Invalid,
};

constexpr bool whole_api_in_constant_evaluation() {
  Result<int, Error> a = Ok(4);

  auto chained = tare::move(a)
    .map([](int x) { return x * 2; })
    .and_then([](int x) -> Result<int, Error> { return Ok(x + 1); });

  if (*chained != 9) {
    return false;
  }

  Result<int, Error> b = Err(Error::Empty);

  if (tare::move(b).unwrap_or(7) != 7) {
    return false;
  }

  int destroyed = 0;

  {
    auto t = Result<Tracked, int>::ok(&destroyed);
    auto u = Result<int, Tracked>::err(&destroyed);
  }

  return destroyed == 2;
}

}

TEST(OkAndErr, DeducesTheWrappedType) {
  static_assert(SameAs<decltype(Ok(5)), Ok<int>>);
  static_assert(SameAs<decltype(Err(Error::Empty)), Err<Error>>);
}

TEST(OkAndErr, DecaysAStringLiteralToAPointer) {
  static_assert(SameAs<decltype(Err("bad")), Err<const char*>>);
}

TEST(OkAndErr, StoresACopyNotAReference) {
  int x = 1;
  auto o = Ok(x);

  x = 2;

  static_assert(SameAs<decltype(o), Ok<int>>);
  EXPECT_EQ(o.value, 1);
}

TEST(OkAndErr, KeepsOkAndErrDistinctOverTheSameType) {
  static_assert(!SameAs<Ok<int>, Err<int>>);
}

TEST(OkAndErr, HoldsAMoveOnlyValue) {
  auto o = Ok(Duplicate{3});

  EXPECT_EQ(o.value.v, 3);
}

TEST(ResultConstruction, BuildsASuccessFromOk) {
  Result<int, Error> r = Ok(5);

  EXPECT_TRUE(r.is_ok());
  EXPECT_FALSE(r.is_err());
  EXPECT_EQ(*r, 5);
}

TEST(ResultConstruction, BuildsAFailureFromErr) {
  Result<int, Error> r = Err(Error::Empty);

  EXPECT_TRUE(r.is_err());
  EXPECT_FALSE(r.is_ok());
  EXPECT_EQ(r.error(), Error::Empty);
}

TEST(ResultConstruction, TellsTheSidesApartWhenTAndEMatch) {
  Result<int, int> a = Ok(1);
  Result<int, int> b = Err(1);

  EXPECT_TRUE(a.is_ok());
  EXPECT_TRUE(b.is_err());
}

TEST(ResultConstruction, ConvertsTheWrappedTypeOnTheWayIn) {
  Result<long long, Error> r = Ok(5);

  EXPECT_EQ(*r, 5LL);
}

TEST(ResultConstruction, BuildsASuccessInPlace) {
  auto r = Result<Duplicate, Error>::ok(7);

  EXPECT_TRUE(r.is_ok());
  EXPECT_EQ(r->v, 7);
}

TEST(ResultConstruction, BuildsAnImmovableSuccessInPlace) {
  auto r = Result<Immovable, Error>::ok(9);

  EXPECT_TRUE(r.is_ok());
  EXPECT_EQ(r->v, 9);
}

TEST(ResultConstruction, BuildsAFailureInPlace) {
  auto r = Result<int, Duplicate>::err(4);

  EXPECT_TRUE(r.is_err());
  EXPECT_EQ(r.error().v, 4);
}

TEST(ResultConstruction, RefusesABareValueWithoutOk) {
  static_assert(!ConstructibleFrom<Result<int, Error>, int>);
}

TEST(ResultConstruction, HasNoEmptyState) {
  static_assert(!ConstructibleFrom<Result<int, Error>>);
}

TEST(ResultConstruction, StillCopiesWhenBothSidesCopyForFree) {
  Result<int, int> a = Ok(5);
  Result<int, int> b = a;

  EXPECT_EQ(*a, 5);
  EXPECT_EQ(*b, 5);
}

TEST(ResultConstruction, DuplicatesASuccessThroughClone) {
  auto a = Result<Duplicate, Error>::ok(3);
  Result<Duplicate, Error> b = a.clone();

  b->v = 4;

  EXPECT_EQ(a->v, 3);
  EXPECT_EQ(b->v, 4);
}

TEST(ResultConstruction, DuplicatesAFailureThroughClone) {
  auto a = Result<int, Duplicate>::err(2);
  Result<int, Duplicate> b = a.clone();

  EXPECT_TRUE(b.is_err());
  EXPECT_EQ(b.error().v, 2);
}

TEST(ResultAccess, MovesTheSuccessOutOnUnwrap) {
  Result<int, Error> r = Ok(42);

  EXPECT_EQ(tare::move(r).unwrap(), 42);
}

TEST(ResultAccess, MovesTheFailureOutOnUnwrapErr) {
  Result<int, Error> r = Err(Error::Invalid);

  EXPECT_EQ(tare::move(r).unwrap_err(), Error::Invalid);
}

TEST(ResultAccess, FallsBackOnAFailedUnwrapOr) {
  Result<int, Error> bad = Err(Error::Empty);
  Result<int, Error> good = Ok(1);

  EXPECT_EQ(tare::move(bad).unwrap_or(99), 99);
  EXPECT_EQ(tare::move(good).unwrap_or(99), 1);
}

TEST(ResultAccess, HandsTheErrorToUnwrapOrElse) {
  Result<int, int> r = Err(3);

  EXPECT_EQ(tare::move(r).unwrap_or_else([](int e) { return e * 10; }), 30);
}

TEST(ResultAccess, OnlyCallsUnwrapOrElseOnFailure) {
  int calls = 0;
  Result<int, int> r = Ok(1);

  auto fallback = [&calls](int e) {
    calls += 1;

    return e;
  };

  EXPECT_EQ(tare::move(r).unwrap_or_else(fallback), 1);
  EXPECT_EQ(calls, 0);
}

TEST(ResultAccess, ReachesMembersThroughTheArrow) {
  auto r = Result<Duplicate, Error>::ok(11);

  EXPECT_EQ(r->v, 11);
}

TEST(ResultAccess, HandsOutTheSuccessAsAnOptionalThroughOk) {
  Result<int, Error> good = Ok(5);
  Result<int, Error> bad = Err(Error::Empty);

  Optional<int> a = tare::move(good).ok();
  Optional<int> b = tare::move(bad).ok();

  EXPECT_EQ(*a, 5);
  EXPECT_FALSE(b.has_value());
}

TEST(ResultAccess, HandsOutTheFailureAsAnOptionalThroughErr) {
  Result<int, Error> good = Ok(5);
  Result<int, Error> bad = Err(Error::Invalid);

  Optional<Error> a = tare::move(good).err();
  Optional<Error> b = tare::move(bad).err();

  EXPECT_FALSE(a.has_value());
  EXPECT_EQ(*b, Error::Invalid);
}

TEST(ResultTransforms, TransformsTheSuccessThroughMap) {
  Result<int, Error> r = Ok(4);
  auto m = tare::move(r).map([](int x) { return x * 2; });

  static_assert(SameAs<decltype(m), Result<int, Error>>);
  EXPECT_EQ(*m, 8);
}

TEST(ResultTransforms, PassesAFailureThroughMapUntouched) {
  int calls = 0;
  Result<int, Error> r = Err(Error::Invalid);

  auto m = tare::move(r).map([&calls](int x) {
    calls += 1;

    return x;
  });

  EXPECT_TRUE(m.is_err());
  EXPECT_EQ(m.error(), Error::Invalid);
  EXPECT_EQ(calls, 0);
}

TEST(ResultTransforms, ChangesTheSuccessTypeThroughMap) {
  Result<int, Error> r = Ok(3);
  auto m = tare::move(r).map([](int x) { return static_cast<char>('a' + x); });

  static_assert(SameAs<decltype(m), Result<char, Error>>);
  EXPECT_EQ(*m, 'd');
}

TEST(ResultTransforms, TransformsTheFailureThroughMapErr) {
  Result<int, int> r = Err(2);
  auto m = tare::move(r).map_err([](int e) { return static_cast<char>('a' + e); });

  static_assert(SameAs<decltype(m), Result<int, char>>);
  EXPECT_EQ(m.error(), 'c');
}

TEST(ResultTransforms, PassesASuccessThroughMapErrUntouched) {
  Result<int, int> r = Ok(7);
  auto m = tare::move(r).map_err([](int e) { return e; });

  EXPECT_EQ(*m, 7);
}

TEST(ResultTransforms, ChainsAFallibleStepThroughAndThen) {
  Result<int, Error> r = Ok(4);

  auto m = tare::move(r).and_then([](int x) -> Result<int, Error> {
    return Ok(x * 10);
  });

  static_assert(SameAs<decltype(m), Result<int, Error>>);
  EXPECT_EQ(*m, 40);
}

TEST(ResultTransforms, LetsAndThenTurnASuccessIntoAFailure) {
  Result<int, Error> r = Ok(1);

  auto m = tare::move(r).and_then([](int) -> Result<int, Error> {
    return Err(Error::Invalid);
  });

  EXPECT_TRUE(m.is_err());
  EXPECT_EQ(m.error(), Error::Invalid);
}

TEST(ResultTransforms, ShortCircuitsAndThenOnFailure) {
  int calls = 0;
  Result<int, Error> r = Err(Error::Empty);

  auto m = tare::move(r).and_then([&calls](int x) -> Result<int, Error> {
    calls += 1;

    return Ok(x);
  });

  EXPECT_TRUE(m.is_err());
  EXPECT_EQ(calls, 0);
}

TEST(ResultTransforms, RecoversFromAFailureThroughOrElse) {
  Result<int, int> r = Err(1);

  auto m = tare::move(r).or_else([](int e) -> Result<int, Error> {
    return Ok(e * 100);
  });

  static_assert(SameAs<decltype(m), Result<int, Error>>);
  EXPECT_EQ(*m, 100);
}

TEST(ResultTransforms, SkipsOrElseOnSuccess) {
  int calls = 0;
  Result<int, int> r = Ok(5);

  auto m = tare::move(r).or_else([&calls](int e) -> Result<int, int> {
    calls += 1;

    return Ok(e);
  });

  EXPECT_EQ(*m, 5);
  EXPECT_EQ(calls, 0);
}

TEST(ResultLifetime, DestroysASuccessAtScopeExit) {
  int destroyed = 0;

  {
    auto r = Result<Tracked, int>::ok(&destroyed);
  }

  EXPECT_EQ(destroyed, 1);
}

TEST(ResultLifetime, DestroysAFailureAtScopeExit) {
  int destroyed = 0;

  {
    auto r = Result<int, Tracked>::err(&destroyed);
  }

  EXPECT_EQ(destroyed, 1);
}

TEST(ResultLifetime, NeverDestroysTheSideItDoesNotHold) {
  int destroyed = 0;

  {
    Result<Tracked, int> r = Err(5);
  }

  EXPECT_EQ(destroyed, 0);
}

TEST(ResultLifetime, MovesOwnershipWithoutDestroyingTwice) {
  int destroyed = 0;

  {
    auto a = Result<Tracked, int>::ok(&destroyed);
    Result<Tracked, int> b = tare::move(a);

    EXPECT_EQ(destroyed, 0);
  }

  EXPECT_EQ(destroyed, 1);
}

TEST(ResultLifetime, DestroysOnceAfterUnwrapMovesTheValueOut) {
  int destroyed = 0;

  {
    auto r = Result<Tracked, int>::ok(&destroyed);
    Tracked t = tare::move(r).unwrap();

    EXPECT_EQ(destroyed, 0);
  }

  EXPECT_EQ(destroyed, 1);
}

TEST(ResultLifetime, SwitchesSidesOnMoveAssignment) {
  int destroyed = 0;

  {
    auto a = Result<Tracked, int>::ok(&destroyed);
    Result<Tracked, int> b = Err(3);

    a = tare::move(b);

    EXPECT_EQ(destroyed, 1);
    EXPECT_TRUE(a.is_err());
    EXPECT_EQ(a.error(), 3);
  }

  EXPECT_EQ(destroyed, 1);
}

TEST(ResultComparison, ComparesSuccessesByValue) {
  Result<int, int> a = Ok(1);
  Result<int, int> b = Ok(1);
  Result<int, int> c = Ok(2);

  EXPECT_TRUE(a == b);
  EXPECT_TRUE(a != c);
}

TEST(ResultComparison, ComparesFailuresByValue) {
  Result<int, int> a = Err(1);
  Result<int, int> b = Err(1);
  Result<int, int> c = Err(2);

  EXPECT_TRUE(a == b);
  EXPECT_TRUE(a != c);
}

TEST(ResultComparison, NeverEquatesASuccessWithAFailure) {
  Result<int, int> a = Ok(1);
  Result<int, int> b = Err(1);

  EXPECT_TRUE(a != b);
}

TEST(ResultLayout, NamesBothSides) {
  static_assert(SameAs<Result<int, Error>::ValueType, int>);
  static_assert(SameAs<Result<int, Error>::ErrorType, Error>);
}

TEST(ResultLayout, KeepsIntIntTriviallyCopyable) {
  static_assert(TriviallyCopyable<Result<int, int>>);
  static_assert(TriviallyMoveConstructible<Result<int, int>>);
}

TEST(ResultLayout, KeepsIntIntTriviallyDestructible) {
  static_assert(TriviallyDestructible<Result<int, int>>);
}

TEST(ResultLayout, CostsOneWordBeyondTheLargerSide) {
  static_assert(sizeof(Result<int, int>) == sizeof(int) * 2);
  static_assert(sizeof(Result<int, char>) == sizeof(int) * 2);
}

TEST(ResultLayout, GoesNonTrivialWhenTheSuccessSideDoes) {
  static_assert(!TriviallyDestructible<Result<Tracked, int>>);
  static_assert(Movable<Result<Tracked, int>>);
}

TEST(ResultLayout, GoesNonTrivialWhenTheFailureSideDoes) {
  static_assert(!TriviallyDestructible<Result<int, Tracked>>);
  static_assert(Movable<Result<int, Tracked>>);
}

TEST(ResultLayout, RefusesToMoveOverAnImmovableType) {
  static_assert(!Movable<Result<Immovable, int>>);
  static_assert(!Movable<Result<int, Immovable>>);
}

TEST(ResultConstantEvaluation, RunsTheWholeApiAtCompileTime) {
  static_assert(whole_api_in_constant_evaluation());
  EXPECT_TRUE(whole_api_in_constant_evaluation());
}