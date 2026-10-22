// SPDX-License-Identifier: BSL-1.0

#include <tare/memory/allocator.hpp>
#include <tare/memory/heap_allocator.hpp>
#include <tare/memory/layout.hpp>

#include <gtest/gtest.h>

#include <type_traits>

using namespace tare::memory;
using namespace tare;

namespace {

struct alignas(64) CacheLine {
  u8 bytes[64];
};

bool is_aligned(const u8* ptr, const usize align) {
  return reinterpret_cast<usize>(ptr) % align == 0;
}

}

TEST(Layout, DescribesOneValueByDefault) {
  constexpr Layout layout = Layout::of<int>();

  EXPECT_EQ(layout.size, sizeof(int));
  EXPECT_EQ(layout.align, alignof(int));
}

TEST(Layout, ScalesSizeButNotAlignmentWithCount) {
  constexpr Layout layout = Layout::of<int>(16);

  EXPECT_EQ(layout.size, sizeof(int) * 16);
  EXPECT_EQ(layout.align, alignof(int));
}

TEST(Layout, CarriesOverAlignment) {
  constexpr Layout layout = Layout::of<CacheLine>(2);

  EXPECT_EQ(layout.size, usize{128});
  EXPECT_EQ(layout.align, usize{64});
}

TEST(HeapAllocator, CannotBeDestroyedThroughTheBase) {
  EXPECT_FALSE(std::is_destructible_v<Allocator>);
  EXPECT_TRUE(std::is_destructible_v<HeapAllocator>);
}

TEST(HeapAllocator, IsFinal) {
  EXPECT_TRUE(std::is_final_v<HeapAllocator>);
}

TEST(HeapAllocator, ReturnsAtLeastTheRequestedSize) {
  HeapAllocator heap;

  constexpr Layout layout = Layout::of<int>(10);
  const Span<u8> block = heap.allocate(layout);

  EXPECT_NE(block.data(), nullptr);
  EXPECT_GE(block.len(), layout.size);

  heap.deallocate(block.data(), layout);
}

namespace {

class HeapAllocatorAlignment : public testing::TestWithParam<usize> {};

}

TEST_P(HeapAllocatorAlignment, HonoursTheRequestedAlignment) {
  HeapAllocator heap;
  const Layout layout{100, GetParam()};

  Span<u8> block = heap.allocate(layout);

  EXPECT_TRUE(is_aligned(block.data(), layout.align)) << "align " << layout.align;

  heap.deallocate(block.data(), layout);
}

INSTANTIATE_TEST_SUITE_P(
  PowersOfTwo,
  HeapAllocatorAlignment,
  ::testing::Values(usize{1}, usize{2}, usize{4}, usize{8}, usize{16}, usize{32}, usize{64}, usize{128}, usize{4096})
);

TEST(HeapAllocator, GivesMemoryThatCanBeWrittenAndReadBack) {
  HeapAllocator heap;

  constexpr Layout layout{
    .size = 256,
    .align = 16,
  };

  const Span<u8> block = heap.allocate(layout);

  for (usize i = 0; i < layout.size; i += 1) {
    block[i] = static_cast<u8>(i);
  }

  for (usize i = 0; i < layout.size; i += 1) {
    EXPECT_EQ(block[i], static_cast<u8>(i));
  }

  heap.deallocate(block.data(), layout);
}

TEST(HeapAllocator, KeepsLiveAllocationsApart) {
  HeapAllocator heap;

  constexpr Layout layout{
    .size = 64,
    .align = 16,
  };

  const Span<u8> a = heap.allocate(layout);
  const Span<u8> b = heap.allocate(layout);

  const bool a_before_b = a.data() + layout.size <= b.data();
  const bool b_before_a = b.data() + layout.size <= a.data();

  EXPECT_TRUE(a_before_b || b_before_a);

  heap.deallocate(a.data(), layout);
  heap.deallocate(b.data(), layout);
}

TEST(HeapAllocator, SurvivesManyAllocateDeallocateCycles) {
  HeapAllocator heap;

  for (usize i = 1; i <= 10'000; i += 1) {
    const Layout layout{
      .size = i % 512 + 1,
      .align = i % 2 == 0 ? usize{64} : usize{8}
    };

    Span<u8> block = heap.allocate(layout);

    block[0] = 1;
    block[layout.size - 1] = 1;

    heap.deallocate(block.data(), layout);
  }

  SUCCEED();
}

TEST(HeapAllocator, HandlesALargeAllocation) {
  HeapAllocator heap;

  constexpr Layout layout{
    .size = usize{64} * 1024 * 1024,
    .align = 64
  };

  const Span<u8> block = heap.allocate(layout);

  block[0] = 1;
  block[layout.size - 1] = 1;

  EXPECT_TRUE(is_aligned(block.data(), 64));

  heap.deallocate(block.data(), layout);
}

TEST(HeapAllocator, CanFreeThroughAnotherInstance) {
  HeapAllocator first;
  HeapAllocator second;

  constexpr Layout layout{
    .size = 32,
    .align = 16,
  };

  const Span<u8> block = first.allocate(layout);

  second.deallocate(block.data(), layout);

  SUCCEED();
}

TEST(DefaultAllocator, IsTheSameObjectEveryTime) {
  EXPECT_EQ(&default_allocator(), &default_allocator());
}

TEST(DefaultAllocator, WorksThroughTheBaseInterface) {
  Allocator& alloc = default_allocator();

  constexpr Layout layout = Layout::of<CacheLine>(4);
  const Span<u8> block = alloc.allocate(layout);

  EXPECT_GE(block.len(), layout.size);
  EXPECT_TRUE(is_aligned(block.data(), 64));

  alloc.deallocate(block.data(), layout);
}

TEST(HeapAllocatorDeathTest, PanicsOnAZeroSizeAllocation) {
  HeapAllocator heap;

  EXPECT_DEATH({
    (void)heap.allocate(Layout{
      .size = 0,
      .align = 8,
    });
  }, "");
}

TEST(HeapAllocatorDeathTest, PanicsOnAZeroSizeDeallocation) {
  HeapAllocator heap;

  constexpr Layout layout{
    .size = 16,
    .align = 8,
  };

  const Span<u8> block = heap.allocate(layout);

  EXPECT_DEATH({
    heap.deallocate(block.data(), Layout{
      .size = 0,
      .align = 8,
    });
  }, "");

  heap.deallocate(block.data(), layout);
}

TEST(HeapAllocatorDeathTest, PanicsWhenTheHeapSaysNo) {
  HeapAllocator heap;

  EXPECT_DEATH({
    (void)heap.allocate(Layout{
      .size = ~usize{0} / 2,
      .align = 16,
    });
  }, "");
}