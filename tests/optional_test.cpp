// SPDX-License-Identifier: BSL-1.0

#include <tare/optional.hpp>

#include "harness.hpp"

using namespace tare;
using harness::describe;
using harness::it;

namespace {

struct Immovable {
  int v;

  constexpr explicit Immovable(int x) : v{x} {}

  Immovable(const Immovable&) = delete;
  Immovable(Immovable&&) = delete;
  Immovable& operator=(const Immovable&) = delete;
  Immovable& operator=(Immovable&&) = delete;
};

struct Duplicate {
  int v;

  constexpr explicit Duplicate(int x) : v{x} {}

  constexpr Duplicate(Duplicate&&) = default;
  constexpr Duplicate& operator=(Duplicate&&) = default;

  Duplicate(const Duplicate&) = delete;
  Duplicate& operator=(const Duplicate&) = delete;

  [[nodiscard]] constexpr Duplicate clone() const {
    return Duplicate{v};
  }
};

struct Tracked {
  int* counter;

  constexpr explicit Tracked(int* c) : counter{c} {}

  constexpr Tracked(Tracked&& other) noexcept : counter{other.counter} {
    other.counter = nullptr;
  }

  constexpr Tracked& operator=(Tracked&& other) noexcept {
    counter = other.counter;
    other.counter = nullptr;

    return *this;
  }

  Tracked(const Tracked&) = delete;
  Tracked& operator=(const Tracked&) = delete;

  constexpr ~Tracked() {
    if (counter != nullptr) {
      *counter += 1;
    }
  }
};

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
    Optional<Tracked> t{InPlace, &destroyed};
    t.emplace(&destroyed);
  }

  return destroyed == 2;
}

static_assert(whole_api_in_constant_evaluation());
static_assert(TriviallyCopyable<Optional<int>>);
static_assert(TriviallyDestructible<Optional<int>>);
static_assert(!TriviallyDestructible<Optional<Tracked>>);
static_assert(!Movable<Optional<Immovable>>);

void layout() {
  describe("layout", [] {
    it("keeps Optional<int> trivially copyable", [] {
      CHECK(TriviallyCopyable<Optional<int>>);
      CHECK(TriviallyMoveConstructible<Optional<int>>);
    });

    it("keeps Optional<int> trivially destructible", [] {
      CHECK(TriviallyDestructible<Optional<int>>);
    });

    it("costs one word beyond T", [] {
      CHECK_EQ(sizeof(Optional<int>), sizeof(int) * 2);
      CHECK_EQ(sizeof(Optional<char>), std::size_t{2});
    });

    it("goes non-trivial exactly when T does", [] {
      CHECK(!TriviallyDestructible<Optional<Tracked>>);
      CHECK(Movable<Optional<Tracked>>);
    });

    it("refuses to move an Optional over an immovable type", [] {
      CHECK(!Movable<Optional<Immovable>>);
    });

    it("recognises clone() only where it exists", [] {
      CHECK(Cloneable<Duplicate>);
      CHECK(!Cloneable<int>);
    });
  });
}

void construction() {
  describe("construction", [] {
    it("is empty by default", [] {
      Optional<int> a;

      CHECK(!a.has_value());
      CHECK(!static_cast<bool>(a));
      CHECK(a == None);
    });

    it("is empty when built from None", [] {
      Optional<int> a = None;

      CHECK(!a.has_value());
    });

    it("holds a value built from one", [] {
      Optional<int> a = 5;

      CHECK(a.has_value());
      CHECK_EQ(*a, 5);
      CHECK(a != None);
    });

    it("builds in place from arguments", [] {
      Optional<Duplicate> a{InPlace, 7};

      CHECK(a.has_value());
      CHECK_EQ(a->v, 7);
    });

    it("builds an immovable type in place", [] {
      Optional<Immovable> a{InPlace, 9};

      CHECK(a.has_value());
      CHECK_EQ(a->v, 9);
    });

    it("still copies when copying is free", [] {
      Optional<int> a = 5;
      Optional<int> b = a;

      CHECK_EQ(*b, 5);
      CHECK_EQ(*a, 5);
    });

    it("duplicates through clone when it is not", [] {
      Optional<Duplicate> a{InPlace, 3};
      Optional<Duplicate> b = a.clone();

      b->v = 4;

      CHECK_EQ(a->v, 3);
      CHECK_EQ(b->v, 4);
    });

    it("clones an empty Optional to an empty one", [] {
      Optional<Duplicate> a = None;

      CHECK(!a.clone().has_value());
    });
  });
}

void access() {
  describe("access", [] {
    it("moves the value out on unwrap", [] {
      Optional<int> a = 42;

      CHECK_EQ(tare::move(a).unwrap(), 42);
    });

    it("falls back on an empty unwrap_or", [] {
      Optional<int> empty = None;
      Optional<int> full = 1;

      CHECK_EQ(tare::move(empty).unwrap_or(99), 99);
      CHECK_EQ(tare::move(full).unwrap_or(99), 1);
    });

    it("only calls the fallback when empty", [] {
      int calls = 0;

      auto make = [&calls] {
        calls += 1;

        return 7;
      };

      Optional<int> empty = None;
      Optional<int> full = 1;

      CHECK_EQ(tare::move(empty).unwrap_or_else(make), 7);
      CHECK_EQ(tare::move(full).unwrap_or_else(make), 1);
      CHECK_EQ(calls, 1);
    });

    it("reaches members through the arrow", [] {
      Optional<Duplicate> a{InPlace, 11};

      CHECK_EQ(a->v, 11);
    });
  });
}

void transforms() {
  describe("transforms", [] {
    it("wraps what map returns", [] {
      Optional<int> a = 4;
      auto doubled = tare::move(a).map([](int x) { return x * 2; });

      CHECK(doubled.has_value());
      CHECK_EQ(*doubled, 8);
    });

    it("skips map on an empty Optional", [] {
      Optional<int> empty = None;
      int calls = 0;

      auto r = tare::move(empty).map([&calls](int x) {
        calls += 1;

        return x;
      });

      CHECK(!r.has_value());
      CHECK_EQ(calls, 0);
    });

    it("changes the wrapped type through map", [] {
      Optional<int> a = 3;
      auto as_char = tare::move(a).map([](int x) { return static_cast<char>('a' + x); });

      CHECK(SameAs<decltype(as_char), Optional<char>>);
      CHECK_EQ(*as_char, 'd');
    });

    it("does not double wrap in and_then", [] {
      Optional<int> a = 4;

      auto r = tare::move(a).and_then([](int x) {
        return Optional<int>{x * 10};
      });

      CHECK(SameAs<decltype(r), Optional<int>>);
      CHECK_EQ(*r, 40);
    });

    it("short circuits and_then on empty", [] {
      Optional<int> empty = None;
      int calls = 0;

      auto r = tare::move(empty).and_then([&calls](int x) {
        calls += 1;

        return Optional<int>{x};
      });

      CHECK(!r.has_value());
      CHECK_EQ(calls, 0);
    });

    it("supplies a default through or_else", [] {
      Optional<int> empty = None;
      Optional<int> full = 1;

      CHECK_EQ(*tare::move(empty).or_else([] { return Optional<int>{5}; }), 5);
      CHECK_EQ(*tare::move(full).or_else([] { return Optional<int>{5}; }), 1);
    });

    it("drops values that fail the filter", [] {
      auto is_even = [](int x) { return x % 2 == 0; };

      Optional<int> even = 4;
      Optional<int> odd = 3;

      auto kept = tare::move(even).filter(is_even);
      auto dropped = tare::move(odd).filter(is_even);

      CHECK_EQ(*kept, 4);
      CHECK(!dropped.has_value());
    });
  });
}

void mutation() {
  describe("mutation", [] {
    it("replaces the contents on emplace", [] {
      Optional<Duplicate> a = None;

      a.emplace(2);
      CHECK_EQ(a->v, 2);

      a.emplace(8);
      CHECK_EQ(a->v, 8);
    });

    it("only fills an empty Optional on get_or_insert", [] {
      Optional<int> a = None;
      Optional<int> b = 1;

      CHECK_EQ(a.get_or_insert(5), 5);
      CHECK_EQ(b.get_or_insert(5), 1);
      CHECK_EQ(*a, 5);
    });

    it("empties the source on take", [] {
      Optional<int> a = 7;
      Optional<int> b = a.take();

      CHECK(!a.has_value());
      CHECK_EQ(*b, 7);
    });

    it("takes nothing from an empty Optional", [] {
      Optional<int> a = None;
      Optional<int> b = a.take();

      CHECK(!a.has_value());
      CHECK(!b.has_value());
    });

    it("hands back the old value on replace", [] {
      Optional<int> a = 1;
      Optional<int> old = a.replace(2);

      CHECK_EQ(*a, 2);
      CHECK_EQ(*old, 1);
    });
  });
}

void lifetime() {
  describe("lifetime", [] {
    it("destroys the value on reset", [] {
      int destroyed = 0;

      {
        Optional<Tracked> a{InPlace, &destroyed};
        a.reset();

        CHECK_EQ(destroyed, 1);
        CHECK(!a.has_value());
      }

      CHECK_EQ(destroyed, 1);
    });

    it("destroys the value at scope exit", [] {
      int destroyed = 0;

      {
        Optional<Tracked> a{InPlace, &destroyed};
      }

      CHECK_EQ(destroyed, 1);
    });

    it("destroys nothing when empty", [] {
      int destroyed = 0;

      {
        Optional<Tracked> a = None;
      }

      CHECK_EQ(destroyed, 0);
    });

    it("destroys the previous value on emplace", [] {
      int destroyed = 0;

      {
        Optional<Tracked> a{InPlace, &destroyed};
        a.emplace(&destroyed);

        CHECK_EQ(destroyed, 1);
      }

      CHECK_EQ(destroyed, 2);
    });

    it("destroys the value on assignment from None", [] {
      int destroyed = 0;

      {
        Optional<Tracked> a{InPlace, &destroyed};
        a = None;

        CHECK_EQ(destroyed, 1);
        CHECK(!a.has_value());
      }

      CHECK_EQ(destroyed, 1);
    });

    it("moves ownership without destroying twice", [] {
      int destroyed = 0;

      {
        Optional<Tracked> a{InPlace, &destroyed};
        Optional<Tracked> b = tare::move(a);

        CHECK_EQ(destroyed, 0);
      }

      CHECK_EQ(destroyed, 1);
    });
  });
}

void comparison() {
  describe("comparison", [] {
    it("compares contents for equality", [] {
      Optional<int> a = 1;
      Optional<int> b = 1;
      Optional<int> c = 2;

      CHECK(a == b);
      CHECK(a != c);
    });

    it("treats empty as unequal to any value", [] {
      Optional<int> a = 1;
      Optional<int> e = None;

      CHECK(a != e);
      CHECK(e == Optional<int>{None});
    });

    it("sorts empty before any value", [] {
      Optional<int> e = None;
      Optional<int> a = 1;
      Optional<int> b = 2;

      CHECK(e < a);
      CHECK(a < b);
      CHECK(!(a < e));
    });
  });
}

void constant_evaluation() {
  describe("constant evaluation", [] {
    it("runs the whole API at compile time", [] {
      CHECK(whole_api_in_constant_evaluation());
    });
  });
}

}

int main() {
  harness::begin("tests/optional_test.cpp");

  describe("Optional", [] {
    layout();
    construction();
    access();
    transforms();
    mutation();
    lifetime();
    comparison();
    constant_evaluation();
  });

  return harness::finish("tests/optional_test.cpp");
}