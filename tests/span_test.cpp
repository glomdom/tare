// SPDX-License-Identifier: BSL-1.0

#include <tare/span.hpp>

#include <gtest/gtest.h>

using namespace tare;

namespace {

template <typename To>
[[maybe_unused]] void accept(To);

template <typename From, typename To>
concept ImplicitlyConvertibleTo = requires(From from) { accept<To>(from); };

constexpr const char* PANIC = "panic: assertion failed";

constexpr bool whole_api_in_constant_evaluation() {
  int a[5] = {1, 2, 3, 4, 5};
  Span<int> s{a};

  if (s.len() != 5 || s[4] != 5 || *s.last() != 5) {
    return false;
  }

  if (s.get(5) != nullptr) {
    return false;
  }

  Span<int> w = s.skip(1).take(3);
  int sum = 0;

  for (int x : w) {
    sum += x;
  }

  Span<const int> c = w;

  return sum == 9 && c.len() == 3 && c[0] == 2;
}

}

TEST(SpanConstruction, IsEmptyByDefault) {
  Span<int> s;

  EXPECT_TRUE(s.is_empty());
  EXPECT_EQ(s.len(), usize{0});
  EXPECT_EQ(s.data(), nullptr);
}

TEST(SpanConstruction, ViewsAWholeArray) {
  int a[3] = {10, 20, 30};
  Span<int> s{a};

  EXPECT_EQ(s.len(), usize{3});
  EXPECT_FALSE(s.is_empty());
  EXPECT_EQ(s.data(), a);
}

TEST(SpanConstruction, ViewsAConstArrayAsConst) {
  const int a[2] = {1, 2};
  Span<const int> s{a};

  EXPECT_EQ(s.len(), usize{2});
  EXPECT_EQ(s.data(), a);
}

TEST(SpanConstruction, BuildsFromARawPointerAndALength) {
  int a[4] = {1, 2, 3, 4};
  auto s = Span<int>::from_raw(a + 1, 2);

  EXPECT_EQ(s.len(), usize{2});
  EXPECT_EQ(s.data(), a + 1);
}

TEST(SpanConstruction, RefusesARawPointerWithoutFromRaw) {
  static_assert(!ConstructibleFrom<Span<int>, int*, usize>);
}

TEST(SpanConstruction, RefusesToViewAConstArrayAsMutable) {
  static_assert(!ConstructibleFrom<Span<int>, const int (&)[3]>);
}

TEST(SpanConstruction, ConvertsMutableToConstImplicitly) {
  int a[2] = {1, 2};
  Span<int> s{a};
  Span<const int> c = s;

  static_assert(ImplicitlyConvertibleTo<Span<int>, Span<const int>>);
  EXPECT_EQ(c.data(), s.data());
  EXPECT_EQ(c.len(), s.len());
}

TEST(SpanConstruction, NeverConvertsConstToMutable) {
  static_assert(!ConstructibleFrom<Span<int>, Span<const int>>);
}

TEST(SpanConstruction, CopiesAsAViewNotADeepCopy) {
  int a[2] = {1, 2};
  Span<int> s{a};
  Span<int> t = s;

  t[0] = 9;

  EXPECT_EQ(a[0], 9);
  EXPECT_EQ(t.data(), s.data());
}

TEST(SpanAccess, ReadsThroughTheIndexOperator) {
  int a[3] = {10, 20, 30};
  Span<int> s{a};

  EXPECT_EQ(s[0], 10);
  EXPECT_EQ(s[2], 30);
}

TEST(SpanAccess, WritesThroughTheIndexOperator) {
  int a[3] = {10, 20, 30};
  Span<int> s{a};

  s[1] = 99;

  EXPECT_EQ(a[1], 99);
}

TEST(SpanAccess, WritesThroughAConstSpan) {
  int a[1] = {1};
  const Span<int> s{a};

  s[0] = 5;

  EXPECT_EQ(a[0], 5);
}

TEST(SpanAccess, HandsOutAnElementThroughGet) {
  int a[3] = {10, 20, 30};
  Span<int> s{a};

  EXPECT_EQ(s.get(1), a + 1);
  EXPECT_EQ(*s.get(1), 20);
}

TEST(SpanAccess, ReturnsNullFromGetWhenOutOfRange) {
  int a[3] = {10, 20, 30};
  Span<int> s{a};

  EXPECT_EQ(s.get(3), nullptr);
  EXPECT_EQ(s.get(1000), nullptr);
}

TEST(SpanAccess, FindsTheFirstAndLastElements) {
  int a[3] = {10, 20, 30};
  Span<int> s{a};

  EXPECT_EQ(*s.first(), 10);
  EXPECT_EQ(*s.last(), 30);
}

TEST(SpanAccess, HasTheSameFirstAndLastWithOneElement) {
  int a[1] = {7};
  Span<int> s{a};

  EXPECT_EQ(s.first(), s.last());
}

TEST(SpanAccess, ReturnsNullFromFirstAndLastWhenEmpty) {
  Span<int> s;

  EXPECT_EQ(s.first(), nullptr);
  EXPECT_EQ(s.last(), nullptr);
}

TEST(SpanSlicing, TakesAWindowOutOfTheMiddleWithSub) {
  int a[5] = {1, 2, 3, 4, 5};
  Span<int> s{a};
  Span<int> w = s.sub(1, 3);

  EXPECT_EQ(w.len(), usize{3});
  EXPECT_EQ(w[0], 2);
  EXPECT_EQ(w[2], 4);
}

TEST(SpanSlicing, AllowsAnEmptyWindowAtTheVeryEnd) {
  int a[3] = {1, 2, 3};
  Span<int> s{a};

  EXPECT_TRUE(s.sub(3, 0).is_empty());
}

TEST(SpanSlicing, WritesThroughAWindowIntoTheOriginal) {
  int a[3] = {1, 2, 3};
  Span<int> s{a};

  s.sub(1, 1)[0] = 42;

  EXPECT_EQ(a[1], 42);
}

TEST(SpanSlicing, KeepsTheFrontWithTake) {
  int a[4] = {1, 2, 3, 4};
  Span<int> s{a};
  Span<int> t = s.take(2);

  EXPECT_EQ(t.len(), usize{2});
  EXPECT_EQ(t.data(), a);
}

TEST(SpanSlicing, DropsTheFrontWithSkip) {
  int a[4] = {1, 2, 3, 4};
  Span<int> s{a};
  Span<int> t = s.skip(1);

  EXPECT_EQ(t.len(), usize{3});
  EXPECT_EQ(t[0], 2);
}

TEST(SpanSlicing, HandlesTakeAndSkipAtTheEdges) {
  int a[3] = {1, 2, 3};
  Span<int> s{a};

  EXPECT_TRUE(s.take(0).is_empty());
  EXPECT_EQ(s.take(3).len(), usize{3});
  EXPECT_TRUE(s.skip(3).is_empty());
  EXPECT_EQ(s.skip(0).len(), usize{3});
}

TEST(SpanSlicing, KeepsConstnessThroughSlicing) {
  const int a[3] = {1, 2, 3};
  Span<const int> s{a};

  static_assert(SameAs<decltype(s.sub(0, 1)), Span<const int>>);
  static_assert(SameAs<decltype(s.take(1)), Span<const int>>);
  static_assert(SameAs<decltype(s.skip(1)), Span<const int>>);
}

TEST(SpanIteration, WalksEveryElementWithRangeFor) {
  int a[4] = {1, 2, 3, 4};
  Span<int> s{a};
  int sum = 0;

  for (int x : s) {
    sum += x;
  }

  EXPECT_EQ(sum, 10);
}

TEST(SpanIteration, MutatesThroughRangeFor) {
  int a[3] = {1, 2, 3};
  Span<int> s{a};

  for (int& x : s) {
    x *= 10;
  }

  EXPECT_EQ(a[0], 10);
  EXPECT_EQ(a[2], 30);
}

TEST(SpanIteration, NeverEntersTheLoopWhenEmpty) {
  Span<int> s;
  int calls = 0;

  for (int _ : s) {
    calls += 1;
  }

  EXPECT_EQ(calls, 0);
}

TEST(SpanIteration, SpansExactlyLenElementsFromBeginToEnd) {
  int a[5] = {};
  Span<int> s{a};

  EXPECT_EQ(s.begin(), a);
  EXPECT_EQ(s.end(), a + 5);
  EXPECT_EQ(static_cast<usize>(s.end() - s.begin()), s.len());
}

TEST(SpanIteration, LinesFirstAndLastUpWithBeginAndEnd) {
  int a[3] = {1, 2, 3};
  Span<int> s{a};

  EXPECT_EQ(s.first(), s.begin());
  EXPECT_EQ(s.last(), s.end() - 1);
}

TEST(SpanBoundsDeathTest, PanicsOnAnIndexPastTheEnd) {
  int a[3] = {1, 2, 3};
  Span<int> s{a};

  EXPECT_DEATH((void)s[3], PANIC);
}

TEST(SpanBoundsDeathTest, PanicsOnAnyIndexIntoAnEmptySpan) {
  Span<int> s;

  EXPECT_DEATH((void)s[0], PANIC);
}

TEST(SpanBoundsDeathTest, PanicsOnAWindowThatRunsPastTheEnd) {
  int a[3] = {1, 2, 3};
  Span<int> s{a};

  EXPECT_DEATH((void)s.sub(2, 2), PANIC);
}

TEST(SpanBoundsDeathTest, PanicsOnAWindowThatStartsPastTheEnd) {
  int a[3] = {1, 2, 3};
  Span<int> s{a};

  EXPECT_DEATH((void)s.sub(4, 0), PANIC);
}

TEST(SpanBoundsDeathTest, PanicsInsteadOfClampingTakeAndSkip) {
  int a[3] = {1, 2, 3};
  Span<int> s{a};

  EXPECT_DEATH((void)s.take(4), PANIC);
  EXPECT_DEATH((void)s.skip(4), PANIC);
}

TEST(SpanBounds, DoesNotPanicOnTheLastValidIndex) {
  int a[3] = {1, 2, 3};
  Span<int> s{a};

  EXPECT_EQ(s[2], 3);
  EXPECT_TRUE(s.sub(3, 0).is_empty());
}

TEST(SpanLayout, NamesItsElementType) {
  static_assert(SameAs<Span<int>::ValueType, int>);
  static_assert(SameAs<Span<const int>::ValueType, const int>);
}

TEST(SpanLayout, IsTriviallyCopyable) {
  static_assert(TriviallyCopyable<Span<int>>);
  static_assert(TriviallyCopyable<Span<const int>>);
}

TEST(SpanLayout, IsAPointerAndALength) {
  static_assert(sizeof(Span<int>) == sizeof(void*) * 2);
}

TEST(SpanConstantEvaluation, RunsTheWholeApiAtCompileTime) {
  static_assert(whole_api_in_constant_evaluation());
  EXPECT_TRUE(whole_api_in_constant_evaluation());
}