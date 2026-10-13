// SPDX-License-Identifier: BSL-1.0

#include <tare/span.hpp>

#include "harness.hpp"

using namespace tare;
using harness::describe;
using harness::it;

namespace {

template <typename To>
void accept(To);

template <typename From, typename To>
concept ImplicitlyConvertibleTo = requires(From from) { accept<To>(from); };

template <int>
struct Probe {};

// i would link the stackoverflow page i found this from but i forgot it and firefox history is ass so
template <auto F>
concept Traps = !requires { typename Probe<(F(), 0)>; };

void construction() {
  describe("construction", [] {
    it("is empty by default", [] {
      Span<int> s;

      CHECK(s.is_empty());
      CHECK_EQ(s.len(), usize{0});
      CHECK(s.data() == nullptr);
    });

    it("views a whole array", [] {
      int a[3] = {10, 20, 30};
      Span<int> s{a};

      CHECK_EQ(s.len(), usize{3});
      CHECK(!s.is_empty());
      CHECK(s.data() == a);
    });

    it("views a const array as const", [] {
      const int a[2] = {1, 2};
      Span<const int> s{a};

      CHECK_EQ(s.len(), usize{2});
      CHECK(s.data() == a);
    });

    it("builds from a raw pointer and a length", [] {
      int a[4] = {1, 2, 3, 4};
      auto s = Span<int>::from_raw(a + 1, 2);

      CHECK_EQ(s.len(), usize{2});
      CHECK(s.data() == a + 1);
    });

    it("refuses a raw pointer without from_raw", [] {
      CHECK(!ConstructibleFrom<Span<int>, int*, usize>);
    });

    it("refuses to view a const array as mutable", [] {
      CHECK(!ConstructibleFrom<Span<int>, const int (&)[3]>);
    });

    it("converts mutable to const implicitly", [] {
      int a[2] = {1, 2};
      Span<int> s{a};
      Span<const int> c = s;

      CHECK(ImplicitlyConvertibleTo<Span<int>, Span<const int>>);
      CHECK(c.data() == s.data());
      CHECK_EQ(c.len(), s.len());
    });

    it("never converts const to mutable", [] {
      CHECK(!ConstructibleFrom<Span<int>, Span<const int>>);
    });

    it("copies as a view, not a deep copy", [] {
      int a[2] = {1, 2};
      Span<int> s{a};
      Span<int> t = s;

      t[0] = 9;

      CHECK_EQ(a[0], 9);
      CHECK(t.data() == s.data());
    });
  });
}

void access() {
  describe("access", [] {
    it("reads through the index operator", [] {
      int a[3] = {10, 20, 30};
      Span<int> s{a};

      CHECK_EQ(s[0], 10);
      CHECK_EQ(s[2], 30);
    });

    it("writes through the index operator", [] {
      int a[3] = {10, 20, 30};
      Span<int> s{a};

      s[1] = 99;

      CHECK_EQ(a[1], 99);
    });

    it("writes through a const Span", [] {
      int a[1] = {1};
      const Span<int> s{a};

      s[0] = 5;

      CHECK_EQ(a[0], 5);
    });

    it("hands out an element through get", [] {
      int a[3] = {10, 20, 30};
      Span<int> s{a};

      CHECK(s.get(1) == a + 1);
      CHECK_EQ(*s.get(1), 20);
    });

    it("returns null from get when out of range", [] {
      int a[3] = {10, 20, 30};
      Span<int> s{a};

      CHECK(s.get(3) == nullptr);
      CHECK(s.get(1000) == nullptr);
    });

    it("finds the first and last elements", [] {
      int a[3] = {10, 20, 30};
      Span<int> s{a};

      CHECK_EQ(*s.first(), 10);
      CHECK_EQ(*s.last(), 30);
    });

    it("has the same first and last with one element", [] {
      int a[1] = {7};
      Span<int> s{a};

      CHECK(s.first() == s.last());
    });

    it("returns null from first and last when empty", [] {
      Span<int> s;

      CHECK(s.first() == nullptr);
      CHECK(s.last() == nullptr);
    });
  });
}

void slicing() {
  describe("slicing", [] {
    it("takes a window out of the middle with sub", [] {
      int a[5] = {1, 2, 3, 4, 5};
      Span<int> s{a};
      Span<int> w = s.sub(1, 3);

      CHECK_EQ(w.len(), usize{3});
      CHECK_EQ(w[0], 2);
      CHECK_EQ(w[2], 4);
    });

    it("allows an empty window at the very end", [] {
      int a[3] = {1, 2, 3};
      Span<int> s{a};

      CHECK(s.sub(3, 0).is_empty());
    });

    it("writes through a window into the original", [] {
      int a[3] = {1, 2, 3};
      Span<int> s{a};

      s.sub(1, 1)[0] = 42;

      CHECK_EQ(a[1], 42);
    });

    it("keeps the front with take", [] {
      int a[4] = {1, 2, 3, 4};
      Span<int> s{a};
      Span<int> t = s.take(2);

      CHECK_EQ(t.len(), usize{2});
      CHECK(t.data() == a);
    });

    it("drops the front with skip", [] {
      int a[4] = {1, 2, 3, 4};
      Span<int> s{a};
      Span<int> t = s.skip(1);

      CHECK_EQ(t.len(), usize{3});
      CHECK_EQ(t[0], 2);
    });

    it("handles take and skip at the edges", [] {
      int a[3] = {1, 2, 3};
      Span<int> s{a};

      CHECK(s.take(0).is_empty());
      CHECK_EQ(s.take(3).len(), usize{3});
      CHECK(s.skip(3).is_empty());
      CHECK_EQ(s.skip(0).len(), usize{3});
    });

    it("keeps constness through slicing", [] {
      const int a[3] = {1, 2, 3};
      Span<const int> s{a};

      CHECK(SameAs<decltype(s.sub(0, 1)), Span<const int>>);
      CHECK(SameAs<decltype(s.take(1)), Span<const int>>);
      CHECK(SameAs<decltype(s.skip(1)), Span<const int>>);
    });
  });
}

void iteration() {
  describe("iteration", [] {
    it("walks every element with range-for", [] {
      int a[4] = {1, 2, 3, 4};
      Span<int> s{a};
      int sum = 0;

      for (int x : s) {
        sum += x;
      }

      CHECK_EQ(sum, 10);
    });

    it("mutates through range-for", [] {
      int a[3] = {1, 2, 3};
      Span<int> s{a};

      for (int& x : s) {
        x *= 10;
      }

      CHECK_EQ(a[0], 10);
      CHECK_EQ(a[2], 30);
    });

    it("never enters the loop when empty", [] {
      Span<int> s;
      int calls = 0;

      for (int _ : s) {
        calls += 1;
      }

      CHECK_EQ(calls, 0);
    });

    it("spans exactly len elements from begin to end", [] {
      int a[5] = {};
      Span<int> s{a};

      CHECK(s.begin() == a);
      CHECK(s.end() == a + 5);
      CHECK_EQ(static_cast<usize>(s.end() - s.begin()), s.len());
    });

    it("lines first and last up with begin and end", [] {
      int a[3] = {1, 2, 3};
      Span<int> s{a};

      CHECK(s.first() == s.begin());
      CHECK(s.last() == s.end() - 1);
    });
  });
}

static_assert(Traps<[] {
  int a[3] = {1, 2, 3};
  Span<int> s{a};

  return s[3];
}>);

static_assert(Traps<[] {
  Span<int> s;

  return s[0];
}>);

static_assert(Traps<[] {
  int a[3] = {1, 2, 3};
  Span<int> s{a};

  return s.sub(2, 2).len();
}>);

static_assert(Traps<[] {
  int a[3] = {1, 2, 3};
  Span<int> s{a};

  return s.sub(4, 0).len();
}>);

static_assert(Traps<[] {
  int a[3] = {1, 2, 3};
  Span<int> s{a};

  return s.take(4).len();
}>);

static_assert(Traps<[] {
  int a[3] = {1, 2, 3};
  Span<int> s{a};

  return s.skip(4).len();
}>);

static_assert(!Traps<[] {
  int a[3] = {1, 2, 3};
  Span<int> s{a};

  return s[2] + static_cast<int>(s.sub(3, 0).len());
}>);

void bounds() {
  describe("bounds", [] {
    it("traps on an index past the end", [] {
      CHECK(Traps<[] {
        int a[3] = {1, 2, 3};
        Span<int> s{a};

        return s[3];
      }>);
    });

    it("traps on any index into an empty Span", [] {
      CHECK(Traps<[] {
        Span<int> s;

        return s[0];
      }>);
    });

    it("traps on a window that runs past the end", [] {
      CHECK(Traps<[] {
        int a[3] = {1, 2, 3};
        Span<int> s{a};

        return s.sub(2, 2).len();
      }>);
    });

    it("traps on a window that starts past the end", [] {
      CHECK(Traps<[] {
        int a[3] = {1, 2, 3};
        Span<int> s{a};

        return s.sub(4, 0).len();
      }>);
    });

    it("traps instead of clamping take and skip", [] {
      CHECK(Traps<[] {
        int a[3] = {1, 2, 3};
        Span<int> s{a};

        return s.take(4).len();
      }>);

      CHECK(Traps<[] {
        int a[3] = {1, 2, 3};
        Span<int> s{a};

        return s.skip(4).len();
      }>);
    });

    it("does not trap on the last valid index", [] {
      CHECK(!Traps<[] {
        int a[3] = {1, 2, 3};
        Span<int> s{a};

        return s[2];
      }>);
    });
  });
}

void layout() {
  describe("layout", [] {
    it("names its element type", [] {
      CHECK(SameAs<Span<int>::ValueType, int>);
      CHECK(SameAs<Span<const int>::ValueType, const int>);
    });

    it("is trivially copyable", [] {
      CHECK(TriviallyCopyable<Span<int>>);
      CHECK(TriviallyCopyable<Span<const int>>);
    });

    it("is a pointer and a length", [] {
      CHECK_EQ(sizeof(Span<int>), sizeof(void*) * 2);
    });
  });
}

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

static_assert(whole_api_in_constant_evaluation());

void constant_evaluation() {
  describe("constant evaluation", [] {
    it("runs the whole API at compile time", [] {
      CHECK(whole_api_in_constant_evaluation());
    });
  });
}

}

void run_span_tests() {
  describe("Span", [] {
    construction();
    access();
    slicing();
    iteration();
    bounds();
    layout();
    constant_evaluation();
  });
}