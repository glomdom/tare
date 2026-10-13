// SPDX-License-Identifier: BSL-1.0

#include <tare/optional.hpp>
#include <tare/result.hpp>

#include "harness.hpp"
#include "test_types.hpp"

using namespace tare;
using namespace test_types;
using harness::describe;
using harness::it;

namespace {

enum class Error {
  Empty,
  Invalid,
};

void wrappers() {
  describe("Ok and Err", [] {
    it("deduces the wrapped type", [] {
      CHECK(SameAs<decltype(Ok(5)), Ok<int>>);
      CHECK(SameAs<decltype(Err(Error::Empty)), Err<Error>>);
    });

    it("decays a string literal to a pointer", [] {
      CHECK(SameAs<decltype(Err("bad")), Err<const char*>>);
    });

    it("stores a copy, not a reference", [] {
      int x = 1;
      auto o = Ok(x);

      x = 2;

      CHECK(SameAs<decltype(o), Ok<int>>);
      CHECK_EQ(o.value, 1);
    });

    it("keeps Ok and Err distinct over the same type", [] {
      CHECK(!SameAs<Ok<int>, Err<int>>);
    });

    it("holds a move-only value", [] {
      auto o = Ok(Duplicate{3});

      CHECK_EQ(o.value.v, 3);
    });
  });
}

void construction() {
  describe("construction", [] {
    it("builds a success from Ok", [] {
      Result<int, Error> r = Ok(5);

      CHECK(r.is_ok());
      CHECK(!r.is_err());
      CHECK_EQ(*r, 5);
    });

    it("builds a failure from Err", [] {
      Result<int, Error> r = Err(Error::Empty);

      CHECK(r.is_err());
      CHECK(!r.is_ok());
      CHECK(r.error() == Error::Empty);
    });

    it("tells the sides apart when T and E match", [] {
      Result<int, int> a = Ok(1);
      Result<int, int> b = Err(1);

      CHECK(a.is_ok());
      CHECK(b.is_err());
    });

    it("converts the wrapped type on the way in", [] {
      Result<long long, Error> r = Ok(5);

      CHECK_EQ(*r, 5LL);
    });

    it("builds a success in place", [] {
      auto r = Result<Duplicate, Error>::ok(7);

      CHECK(r.is_ok());
      CHECK_EQ(r->v, 7);
    });

    it("builds an immovable success in place", [] {
      auto r = Result<Immovable, Error>::ok(9);

      CHECK(r.is_ok());
      CHECK_EQ(r->v, 9);
    });

    it("builds a failure in place", [] {
      auto r = Result<int, Duplicate>::err(4);

      CHECK(r.is_err());
      CHECK_EQ(r.error().v, 4);
    });

    it("refuses a bare value without Ok", [] {
      CHECK(!ConstructibleFrom<Result<int, Error>, int>);
    });

    it("has no empty state", [] {
      CHECK(!ConstructibleFrom<Result<int, Error>>);
    });

    it("still copies when both sides copy for free", [] {
      Result<int, int> a = Ok(5);
      Result<int, int> b = a;

      CHECK_EQ(*a, 5);
      CHECK_EQ(*b, 5);
    });

    it("duplicates a success through clone", [] {
      auto a = Result<Duplicate, Error>::ok(3);
      Result<Duplicate, Error> b = a.clone();

      b->v = 4;

      CHECK_EQ(a->v, 3);
      CHECK_EQ(b->v, 4);
    });

    it("duplicates a failure through clone", [] {
      auto a = Result<int, Duplicate>::err(2);
      Result<int, Duplicate> b = a.clone();

      CHECK(b.is_err());
      CHECK_EQ(b.error().v, 2);
    });
  });
}

void access() {
  describe("access", [] {
    it("moves the success out on unwrap", [] {
      Result<int, Error> r = Ok(42);

      CHECK_EQ(tare::move(r).unwrap(), 42);
    });

    it("moves the failure out on unwrap_err", [] {
      Result<int, Error> r = Err(Error::Invalid);

      CHECK(tare::move(r).unwrap_err() == Error::Invalid);
    });

    it("falls back on a failed unwrap_or", [] {
      Result<int, Error> bad = Err(Error::Empty);
      Result<int, Error> good = Ok(1);

      CHECK_EQ(tare::move(bad).unwrap_or(99), 99);
      CHECK_EQ(tare::move(good).unwrap_or(99), 1);
    });

    it("hands the error to unwrap_or_else", [] {
      Result<int, int> r = Err(3);

      CHECK_EQ(tare::move(r).unwrap_or_else([](int e) { return e * 10; }), 30);
    });

    it("only calls unwrap_or_else on failure", [] {
      int calls = 0;
      Result<int, int> r = Ok(1);

      auto fallback = [&calls](int e) {
        calls += 1;

        return e;
      };

      CHECK_EQ(tare::move(r).unwrap_or_else(fallback), 1);
      CHECK_EQ(calls, 0);
    });

    it("reaches members through the arrow", [] {
      auto r = Result<Duplicate, Error>::ok(11);

      CHECK_EQ(r->v, 11);
    });

    it("hands out the success as an Optional through ok", [] {
      Result<int, Error> good = Ok(5);
      Result<int, Error> bad = Err(Error::Empty);

      Optional<int> a = tare::move(good).ok();
      Optional<int> b = tare::move(bad).ok();

      CHECK_EQ(*a, 5);
      CHECK(!b.has_value());
    });

    it("hands out the failure as an Optional through err", [] {
      Result<int, Error> good = Ok(5);
      Result<int, Error> bad = Err(Error::Invalid);

      Optional<Error> a = tare::move(good).err();
      Optional<Error> b = tare::move(bad).err();

      CHECK(!a.has_value());
      CHECK(*b == Error::Invalid);
    });
  });
}

void transforms() {
  describe("transforms", [] {
    it("transforms the success through map", [] {
      Result<int, Error> r = Ok(4);
      auto m = tare::move(r).map([](int x) { return x * 2; });

      CHECK(SameAs<decltype(m), Result<int, Error>>);
      CHECK_EQ(*m, 8);
    });

    it("passes a failure through map untouched", [] {
      int calls = 0;
      Result<int, Error> r = Err(Error::Invalid);

      auto m = tare::move(r).map([&calls](int x) {
        calls += 1;

        return x;
      });

      CHECK(m.is_err());
      CHECK(m.error() == Error::Invalid);
      CHECK_EQ(calls, 0);
    });

    it("changes the success type through map", [] {
      Result<int, Error> r = Ok(3);
      auto m = tare::move(r).map([](int x) { return static_cast<char>('a' + x); });

      CHECK(SameAs<decltype(m), Result<char, Error>>);
      CHECK_EQ(*m, 'd');
    });

    it("transforms the failure through map_err", [] {
      Result<int, int> r = Err(2);
      auto m = tare::move(r).map_err([](int e) { return static_cast<char>('a' + e); });

      CHECK(SameAs<decltype(m), Result<int, char>>);
      CHECK_EQ(m.error(), 'c');
    });

    it("passes a success through map_err untouched", [] {
      Result<int, int> r = Ok(7);
      auto m = tare::move(r).map_err([](int e) { return e; });

      CHECK_EQ(*m, 7);
    });

    it("chains a fallible step through and_then", [] {
      Result<int, Error> r = Ok(4);

      auto m = tare::move(r).and_then([](int x) -> Result<int, Error> {
        return Ok(x * 10);
      });

      CHECK(SameAs<decltype(m), Result<int, Error>>);
      CHECK_EQ(*m, 40);
    });

    it("lets and_then turn a success into a failure", [] {
      Result<int, Error> r = Ok(1);

      auto m = tare::move(r).and_then([](int) -> Result<int, Error> {
        return Err(Error::Invalid);
      });

      CHECK(m.is_err());
      CHECK(m.error() == Error::Invalid);
    });

    it("short circuits and_then on failure", [] {
      int calls = 0;
      Result<int, Error> r = Err(Error::Empty);

      auto m = tare::move(r).and_then([&calls](int x) -> Result<int, Error> {
        calls += 1;

        return Ok(x);
      });

      CHECK(m.is_err());
      CHECK_EQ(calls, 0);
    });

    it("recovers from a failure through or_else", [] {
      Result<int, int> r = Err(1);

      auto m = tare::move(r).or_else([](int e) -> Result<int, Error> {
        return Ok(e * 100);
      });

      CHECK(SameAs<decltype(m), Result<int, Error>>);
      CHECK_EQ(*m, 100);
    });

    it("skips or_else on success", [] {
      int calls = 0;
      Result<int, int> r = Ok(5);

      auto m = tare::move(r).or_else([&calls](int e) -> Result<int, int> {
        calls += 1;

        return Ok(e);
      });

      CHECK_EQ(*m, 5);
      CHECK_EQ(calls, 0);
    });
  });
}

void lifetime() {
  describe("lifetime", [] {
    it("destroys a success at scope exit", [] {
      int destroyed = 0;

      {
        auto r = Result<Tracked, int>::ok(&destroyed);
      }

      CHECK_EQ(destroyed, 1);
    });

    it("destroys a failure at scope exit", [] {
      int destroyed = 0;

      {
        auto r = Result<int, Tracked>::err(&destroyed);
      }

      CHECK_EQ(destroyed, 1);
    });

    it("never destroys the side it does not hold", [] {
      int destroyed = 0;

      {
        Result<Tracked, int> r = Err(5);
      }

      CHECK_EQ(destroyed, 0);
    });

    it("moves ownership without destroying twice", [] {
      int destroyed = 0;

      {
        auto a = Result<Tracked, int>::ok(&destroyed);
        Result<Tracked, int> b = tare::move(a);

        CHECK_EQ(destroyed, 0);
      }

      CHECK_EQ(destroyed, 1);
    });

    it("destroys once after unwrap moves the value out", [] {
      int destroyed = 0;

      {
        auto r = Result<Tracked, int>::ok(&destroyed);
        Tracked t = tare::move(r).unwrap();

        CHECK_EQ(destroyed, 0);
      }

      CHECK_EQ(destroyed, 1);
    });

    it("switches sides on move assignment", [] {
      int destroyed = 0;

      {
        auto a = Result<Tracked, int>::ok(&destroyed);
        Result<Tracked, int> b = Err(3);

        a = tare::move(b);

        CHECK_EQ(destroyed, 1);
        CHECK(a.is_err());
        CHECK_EQ(a.error(), 3);
      }

      CHECK_EQ(destroyed, 1);
    });
  });
}

void comparison() {
  describe("comparison", [] {
    it("compares successes by value", [] {
      Result<int, int> a = Ok(1);
      Result<int, int> b = Ok(1);
      Result<int, int> c = Ok(2);

      CHECK(a == b);
      CHECK(a != c);
    });

    it("compares failures by value", [] {
      Result<int, int> a = Err(1);
      Result<int, int> b = Err(1);
      Result<int, int> c = Err(2);

      CHECK(a == b);
      CHECK(a != c);
    });

    it("never equates a success with a failure", [] {
      Result<int, int> a = Ok(1);
      Result<int, int> b = Err(1);

      CHECK(a != b);
    });
  });
}

void layout() {
  describe("layout", [] {
    it("names both sides", [] {
      CHECK(SameAs<Result<int, Error>::ValueType, int>);
      CHECK(SameAs<Result<int, Error>::ErrorType, Error>);
    });

    it("keeps Result<int, int> trivially copyable", [] {
      CHECK(TriviallyCopyable<Result<int, int>>);
      CHECK(TriviallyMoveConstructible<Result<int, int>>);
    });

    it("keeps Result<int, int> trivially destructible", [] {
      CHECK(TriviallyDestructible<Result<int, int>>);
    });

    it("costs one word beyond the larger side", [] {
      CHECK_EQ(sizeof(Result<int, int>), sizeof(int) * 2);
      CHECK_EQ(sizeof(Result<int, char>), sizeof(int) * 2);
    });

    it("goes non-trivial when the success side does", [] {
      CHECK(!TriviallyDestructible<Result<Tracked, int>>);
      CHECK(Movable<Result<Tracked, int>>);
    });

    it("goes non-trivial when the failure side does", [] {
      CHECK(!TriviallyDestructible<Result<int, Tracked>>);
      CHECK(Movable<Result<int, Tracked>>);
    });

    it("refuses to move a Result over an immovable type", [] {
      CHECK(!Movable<Result<Immovable, int>>);
      CHECK(!Movable<Result<int, Immovable>>);
    });
  });
}

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

static_assert(whole_api_in_constant_evaluation());

void constant_evaluation() {
  describe("constant evaluation", [] {
    it("runs the whole API at compile time", [] {
      CHECK(whole_api_in_constant_evaluation());
    });
  });
}

}

void run_result_tests() {
  describe("Result", [] {
    wrappers();
    construction();
    access();
    transforms();
    lifetime();
    comparison();
    layout();
    constant_evaluation();
  });
}