//! @file
//!
//! @brief

#include <inttypes.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "CppUTest/MemoryLeakDetectorMallocMacros.h"
#include "CppUTest/MemoryLeakDetectorNewMacros.h"
#include "CppUTest/TestHarness.h"
#include "CppUTestExt/MockSupport.h"
#include "fakes/fake_memfault_platform_metrics_locking.h"

#include "memfault/core/heap_stats.h"
#include "memfault/core/heap_stats_impl.h"
#include "memfault/core/math.h"

TEST_GROUP(MemfaultHeapStats) {
  void setup() {
    fake_memfault_metrics_platorm_locking_reboot();

    mock().disable();
  }
  void teardown() {
    CHECK(fake_memfault_platform_metrics_lock_calls_balanced());
    memfault_heap_stats_reset();
    mock().checkExpectations();
    mock().clear();
  }
};

static bool prv_heap_stat_equality(const sMfltHeapStatEntry *expected,
                                   const sMfltHeapStatEntry *actual) {
  bool match = (expected->lr == actual->lr) &&    //
               (expected->ptr == actual->ptr) &&  //
               (expected->info.size == actual->info.size) && //
               (expected->info.in_use == actual->info.in_use);
  if (!match) {
    fprintf(stderr,
            "sMfltHeapStatEntry:\n"
            " lr: %p : %p\n"
            " ptr: %p : %p\n"
            " size: %u : %u\n"
            " in_use: %u : %u\n",
            expected->lr, actual->lr, expected->ptr, actual->ptr, (unsigned int)expected->info.size,
            (unsigned int)actual->info.size, (unsigned int)expected->info.in_use,
            (unsigned int)actual->info.in_use);
  }
  return match;
}

TEST(MemfaultHeapStats, Test_Basic) {
  void *lr;
  MEMFAULT_GET_LR(lr);
  const sMfltHeapStatEntry expected_heap_stats[] = {
    {
      .lr = lr,
      .ptr = (void *)0x12345679,
      .info =
        {
          .size = 1234,
          .in_use = 1,
        },
    },
  };

  bool empty = memfault_heap_stats_empty();
  CHECK(empty);
  MEMFAULT_HEAP_STATS_MALLOC(expected_heap_stats[0].ptr, expected_heap_stats[0].info.size);
  empty = memfault_heap_stats_empty();
  CHECK(!empty);

  bool match = prv_heap_stat_equality(&expected_heap_stats[0], &g_memfault_heap_stats_pool[0]);

  CHECK(match);
}

TEST(MemfaultHeapStats, Test_Free) {
  void *lr;
  MEMFAULT_GET_LR(lr);
  const sMfltHeapStatEntry
    expected_heap_stats[MEMFAULT_ARRAY_SIZE(g_memfault_heap_stats_pool)] = {
      {
        .lr = lr,
        .ptr = (void *)0x12345679,
        .info =
          {
            .size = 1234,
            .in_use = 1,
          },
      },
      {
        .lr = lr,
        .ptr = (void *)0x1234567A,
        .info =
          {
            .size = 12345,
            .in_use = 0,
          },
      },
    };

  MEMFAULT_HEAP_STATS_MALLOC(expected_heap_stats[0].ptr, expected_heap_stats[0].info.size);
  LONGS_EQUAL(1, g_memfault_heap_stats.in_use_block_count);
  LONGS_EQUAL(1, g_memfault_heap_stats.max_in_use_block_count);

  MEMFAULT_HEAP_STATS_MALLOC(expected_heap_stats[1].ptr, expected_heap_stats[1].info.size);
  LONGS_EQUAL(2, g_memfault_heap_stats.in_use_block_count);
  LONGS_EQUAL(2, g_memfault_heap_stats.max_in_use_block_count);

  MEMFAULT_HEAP_STATS_FREE(expected_heap_stats[1].ptr);
  LONGS_EQUAL(1, g_memfault_heap_stats.in_use_block_count);
  LONGS_EQUAL(2, g_memfault_heap_stats.max_in_use_block_count);

  // test freeing a null pointer
  MEMFAULT_HEAP_STATS_FREE(NULL);
  LONGS_EQUAL(1, g_memfault_heap_stats.in_use_block_count);
  LONGS_EQUAL(2, g_memfault_heap_stats.max_in_use_block_count);

  // test freeing something that doesn't exist in the tracked list
  MEMFAULT_HEAP_STATS_FREE((void *)0xabc);
  // since we don't know that the untracked but non-null pointer being freed
  // above is invalid, we still decrement the in use count
  LONGS_EQUAL(0, g_memfault_heap_stats.in_use_block_count);
  LONGS_EQUAL(2, g_memfault_heap_stats.max_in_use_block_count);

  // test malloc stats with failed malloc (NULL pointer)
  MEMFAULT_HEAP_STATS_MALLOC(NULL, 123456);
  LONGS_EQUAL(0, g_memfault_heap_stats.in_use_block_count);
  LONGS_EQUAL(2, g_memfault_heap_stats.max_in_use_block_count);

  // work over the list, confirming that everything matches expected
  size_t list_count = 0;
  for (size_t i = 0; i < MEMFAULT_ARRAY_SIZE(g_memfault_heap_stats_pool); i++) {
    sMfltHeapStatEntry *pthis = &g_memfault_heap_stats_pool[i];
    if (pthis->info.size != 0) {
      list_count++;
      bool match = prv_heap_stat_equality(&expected_heap_stats[i], pthis);
      CHECK(match);
    }
  }
  LONGS_EQUAL(2, list_count);
}

TEST(MemfaultHeapStats, Test_MaxEntriesRollover) {
  void *lr;
  MEMFAULT_GET_LR(lr);
  const sMfltHeapStatEntry
    expected_heap_stats[MEMFAULT_ARRAY_SIZE(g_memfault_heap_stats_pool)] = {
      {
        .lr = lr,
        .ptr = (void *)0x12345679,
        .info =
          {
            .size = 1234,
            .in_use = 1,
          },
      },
      {
        .lr = lr,
        // this entry should not appear when checking at the end of this test,
        // so set the data to something exceptional
        .ptr = (void *)0xabcdef,
        .info =
          {
            .size = 123456,
            .in_use = 0,
          },
      },
    };

  // allocate one entry, and then free it
  MEMFAULT_HEAP_STATS_MALLOC(expected_heap_stats[1].ptr, expected_heap_stats[1].info.size);
  MEMFAULT_HEAP_STATS_FREE(expected_heap_stats[1].ptr);
  LONGS_EQUAL(0, g_memfault_heap_stats.in_use_block_count);
  LONGS_EQUAL(1, g_memfault_heap_stats.max_in_use_block_count);

  // now allocate enough entries to fill the stats completely, overwriting the
  // existing entry
  for (size_t i = 0; i < MEMFAULT_ARRAY_SIZE(g_memfault_heap_stats_pool); i++) {
    MEMFAULT_HEAP_STATS_MALLOC((void *)((uintptr_t)expected_heap_stats[0].ptr + i),
                               expected_heap_stats[0].info.size + i);
  }
  LONGS_EQUAL(32, g_memfault_heap_stats.in_use_block_count);
  LONGS_EQUAL(32, g_memfault_heap_stats.max_in_use_block_count);

  // work over the list, FIFO, confirming that everything matches expected
  size_t list_count = 0;
  for (size_t i = 0; i < MEMFAULT_ARRAY_SIZE(g_memfault_heap_stats_pool); i++) {
    size_t current_idx = (g_memfault_heap_stats.stats_pool_head + i) %
                         MEMFAULT_ARRAY_SIZE(g_memfault_heap_stats_pool);
    sMfltHeapStatEntry *pthis = &g_memfault_heap_stats_pool[current_idx];
    sMfltHeapStatEntry expected = {
      .lr = lr,
      .ptr = (void *)(0x12345679 + i),
      .info =
        {
          .size = 1234 + (uint32_t)i,
          .in_use = 1,
        },
    };

    if (pthis->info.size != 0) {
      list_count++;
      bool match = prv_heap_stat_equality(&expected, pthis);
      CHECK(match);
    }
  }
  LONGS_EQUAL(32, list_count);
}
