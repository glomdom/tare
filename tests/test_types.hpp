// SPDX-License-Identifier: BSL-1.0

#pragma once

namespace test_types {

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

  friend constexpr bool operator==(const Duplicate& a, const Duplicate& b) {
    return a.v == b.v;
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

}