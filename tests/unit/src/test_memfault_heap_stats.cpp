//! @file
//!
//! @brief

#include <inttypes.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include <random>

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
    fake_memfault_metrics_platform_locking_reboot();

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
  bool match = (expected->lr == actual->lr) &&                //
               (expected->ptr == actual->ptr) &&              //
               (expected->info.size == actual->info.size) &&  //
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
      .info = {
        .size = 1234,
        .in_use = 1,
      },
    },
  };

  bool empty = memfault_heap_stats_empty();
  CHECK(empty);

  // Check initial list head points to empty
  LONGS_EQUAL(MEMFAULT_HEAP_STATS_LIST_END, g_memfault_heap_stats.stats_pool_head);

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
        .info = {
          .size = 1234,
          .in_use = 1,
        },
      },
      {
        .lr = lr,
        .ptr = (void *)0x1234567A,
        .info = {
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

  // Test correct state after freeing last entry
  MEMFAULT_HEAP_STATS_FREE(expected_heap_stats[0].ptr);
  LONGS_EQUAL(MEMFAULT_HEAP_STATS_LIST_END, g_memfault_heap_stats.stats_pool_head);
}

TEST(MemfaultHeapStats, Test_MaxEntriesRollover) {
  void *lr;
  MEMFAULT_GET_LR(lr);
  const sMfltHeapStatEntry
    expected_heap_stats[MEMFAULT_ARRAY_SIZE(g_memfault_heap_stats_pool)] = {
      {
        .lr = lr,
        .ptr = (void *)0x12345679,
        .info = {
          .size = 1234,
          .in_use = 1,
        },
      },
      {
        .lr = lr,
        // this entry should not appear when checking at the end of this test,
        // so set the data to something exceptional
        .ptr = (void *)0xabcdef,
        .info = {
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

  // now allocate enough entries to fill the stats completely
  for (size_t i = 0; i < MEMFAULT_ARRAY_SIZE(g_memfault_heap_stats_pool); i++) {
    MEMFAULT_HEAP_STATS_MALLOC((void *)((uintptr_t)expected_heap_stats[0].ptr + i),
                               expected_heap_stats[0].info.size + i);
  }
  LONGS_EQUAL(MEMFAULT_HEAP_STATS_MAX_COUNT, g_memfault_heap_stats.in_use_block_count);
  LONGS_EQUAL(MEMFAULT_HEAP_STATS_MAX_COUNT, g_memfault_heap_stats.max_in_use_block_count);

  // work over the list, FIFO, confirming that everything matches expected
  size_t list_count = 0;
  for (size_t i = 0; i < MEMFAULT_ARRAY_SIZE(g_memfault_heap_stats_pool); i++) {
    sMfltHeapStatEntry *pthis = &g_memfault_heap_stats_pool[i];
    size_t offset;
    if (i == 0) {
      // the first entry is the most recently added, since we wrapped around
      // ex:
      // [ 9, 0, 1, 2, 3, 4, 5, 6, 7, 8 ]
      offset = MEMFAULT_ARRAY_SIZE(g_memfault_heap_stats_pool) - 1;
    } else {
      // the start of the allocations is at the second position in the array
      offset = i - 1;
    }
    sMfltHeapStatEntry expected = {
      .lr = lr,
      .ptr = (void *)(0x12345679 + offset),
      .info = {
        .size = 1234 + (uint32_t)offset,
        .in_use = 1,
      },
    };

    if (pthis->info.size != 0) {
      list_count++;
      bool match = prv_heap_stat_equality(&expected, pthis);
      CHECK(match);
    }
  }
  LONGS_EQUAL(MEMFAULT_HEAP_STATS_MAX_COUNT, list_count);
}

//! Verify that an allocation that reuses a previously freed address is properly
//! cleared from the stats list
TEST(MemfaultHeapStats, Test_AddressReuse) {
  void *lr;
  MEMFAULT_GET_LR(lr);
  const sMfltHeapStatEntry expected_heap_stats[] = {
    {
      .lr = lr,
      .ptr = (void *)0x12345679,
      .info = {
        .size = 1234,
        .in_use = 0,
      },
    },
    {
      .lr = lr,
      .ptr = (void *)0x12345679,
      .info = {
        .size = 1234,
        .in_use = 0,
      },
    },
  };

  bool empty = memfault_heap_stats_empty();
  CHECK(empty);
  MEMFAULT_HEAP_STATS_MALLOC(expected_heap_stats[0].ptr, expected_heap_stats[0].info.size);
  MEMFAULT_HEAP_STATS_FREE(expected_heap_stats[0].ptr);
  MEMFAULT_HEAP_STATS_MALLOC(expected_heap_stats[1].ptr, expected_heap_stats[1].info.size);
  MEMFAULT_HEAP_STATS_FREE(expected_heap_stats[1].ptr);
  empty = memfault_heap_stats_empty();
  CHECK(!empty);

  size_t list_count = 0;
  for (size_t i = 0; i < MEMFAULT_ARRAY_SIZE(g_memfault_heap_stats_pool); i++) {
    sMfltHeapStatEntry *pthis = &g_memfault_heap_stats_pool[i];
    if (pthis->info.size != 0) {
      list_count++;
      bool match = prv_heap_stat_equality(&expected_heap_stats[i], pthis);
      CHECK(match);
    }
  }
  // 2 entries will be populated (though in_use==0 for each), since "never-used"
  // entries are always used before "unused" (freed) entries
  LONGS_EQUAL(2, list_count);
}

//! Verifies logic for reusing heap stat entries.
//!
//! Heap stats should first look for free entries, then overwrite the oldest entry
//! if none are free
TEST(MemfaultHeapStats, Test_Reuse) {
  void *lr;
  MEMFAULT_GET_LR(lr);
  const sMfltHeapStatEntry expected_heap_stats = {
    .lr = lr,
    .ptr = (void *)0x12345679,
    .info = {
      .size = 1234,
      .in_use = 1,
    },
  };

  const sMfltHeapStatEntry middle_entry = {
    .lr = lr,
    .ptr = (void *)0x87654321,
    .info = {
      .size = 4321,
      .in_use = 1,
    },
  };

  const sMfltHeapStatEntry final_entry = {
    .lr = lr,
    .ptr = (void *)0x111111111,
    .info = {
      .size = 1111,
      .in_use = 1,
    },
  };

  // Offset to unique entry in middle of heap stats
  size_t middle_offset = MEMFAULT_ARRAY_SIZE(g_memfault_heap_stats_pool) >> 1;

  // Fill up the heap stats pool
  for (size_t i = 0; i < MEMFAULT_ARRAY_SIZE(g_memfault_heap_stats_pool); i++) {
    MEMFAULT_HEAP_STATS_MALLOC((void *)((uintptr_t)expected_heap_stats.ptr + i),
                               expected_heap_stats.info.size + i);
  }

  // Free entry in middle of array, then malloc again
  MEMFAULT_HEAP_STATS_FREE((void *)((uintptr_t)expected_heap_stats.ptr + middle_offset));
  MEMFAULT_HEAP_STATS_MALLOC(middle_entry.ptr, middle_entry.info.size);

  // Check latest entry is list head
  LONGS_EQUAL(middle_offset, g_memfault_heap_stats.stats_pool_head);
  prv_heap_stat_equality(&middle_entry,
                         &g_memfault_heap_stats_pool[g_memfault_heap_stats.stats_pool_head]);

  // Check next recent is end of heap stats pool
  sMfltHeapStatEntry *entry = &g_memfault_heap_stats_pool[g_memfault_heap_stats.stats_pool_head];
  prv_heap_stat_equality(&g_memfault_heap_stats_pool[MEMFAULT_HEAP_STATS_MAX_COUNT - 1],
                         &g_memfault_heap_stats_pool[entry->info.next_entry_index]);

  // Allocate another, should overwrite first entry (current oldest)
  MEMFAULT_HEAP_STATS_MALLOC(final_entry.ptr, final_entry.info.size);
  LONGS_EQUAL(0, g_memfault_heap_stats.stats_pool_head);
  prv_heap_stat_equality(g_memfault_heap_stats_pool,
                         &g_memfault_heap_stats_pool[g_memfault_heap_stats.stats_pool_head]);

  // walk the other entries and confirm correct values
  size_t list_count = 0;
  for (size_t i = 1; i < MEMFAULT_ARRAY_SIZE(g_memfault_heap_stats_pool); i++) {
    if (i == middle_offset) {
      continue;
    }

    sMfltHeapStatEntry *pthis = &g_memfault_heap_stats_pool[i];
    sMfltHeapStatEntry expected = {
      .lr = lr,
      .ptr = (void *)(0x12345679 + i),
      .info = {
        .size = 1234 + (uint32_t)i,
        .in_use = 1,
      },
    };

    if (pthis->info.size != 0) {
      list_count++;
      bool match = prv_heap_stat_equality(&expected, pthis);
      if (!match) {
        fprintf(stderr, "Mismatch at index %zu\n", i);
      }
      CHECK(match);
    }
  }

  // We skipped two entries from the pool
  LONGS_EQUAL(MEMFAULT_HEAP_STATS_MAX_COUNT - 2, list_count);
}

//! Tests handling freeing the most recent allocation (list head)
TEST(MemfaultHeapStats, Test_FreeMostRecent) {
  void *lr;
  MEMFAULT_GET_LR(lr);

  const sMfltHeapStatEntry expected_heap_stats = {
    .lr = lr,
    .ptr = (void *)0x12345679,
    .info = {
      .size = 1234,
      .in_use = 1,
    },
  };

  size_t end_offset = MEMFAULT_ARRAY_SIZE(g_memfault_heap_stats_pool) - 1;

  // Fill up the heap stats pool
  for (size_t i = 0; i < MEMFAULT_ARRAY_SIZE(g_memfault_heap_stats_pool); i++) {
    MEMFAULT_HEAP_STATS_MALLOC((void *)((uintptr_t)expected_heap_stats.ptr + i),
                               expected_heap_stats.info.size + i);
  }

  // Remove most recent entry (last in pool)
  MEMFAULT_HEAP_STATS_FREE((void *)((uintptr_t)expected_heap_stats.ptr + end_offset));

  // Check that most recent entry is second to last, check entry contents
  LONGS_EQUAL(end_offset - 1, g_memfault_heap_stats.stats_pool_head);
  prv_heap_stat_equality(&expected_heap_stats,
                         &g_memfault_heap_stats_pool[g_memfault_heap_stats.stats_pool_head]);
}

//! Tests that "never-used" entries are used before "unused" (freed) entries
TEST(MemfaultHeapStats, Test_NeverUsedVsUnused) {
  void *lr;
  MEMFAULT_GET_LR(lr);

  const sMfltHeapStatEntry expected_heap_stats = {
    .lr = lr,
    .ptr = (void *)0x12345678,
    .info = {
      .size = 1234,
      .in_use = 1,
    },
  };
  // reference pool. fill this up with matching entries as the heap stats pool
  // during the allocation calls below. this is breaking the abstraction a bit,
  // but we need to check what the heap stats pool should look like at the end.
  sMfltHeapStatEntry reference_pool[MEMFAULT_ARRAY_SIZE(g_memfault_heap_stats_pool)];

  // Fill up the heap stats pool, but leave the last entry unused
  for (size_t i = 0; i < MEMFAULT_ARRAY_SIZE(g_memfault_heap_stats_pool) - 1; i++) {
    MEMFAULT_HEAP_STATS_MALLOC((void *)((uintptr_t)expected_heap_stats.ptr + i),
                               expected_heap_stats.info.size + i);
    reference_pool[i] = g_memfault_heap_stats_pool[i];
  }

  // Remove the first allocation
  MEMFAULT_HEAP_STATS_FREE(expected_heap_stats.ptr);
  // Unfortunately this peeks into the implementation. The data in
  // 'g_memfault_heap_stats_pool' is actually part of its public contract, since
  // we parse it in the backend, so we need to keep it consistent.
  reference_pool[0].info.in_use = 0;

  // Allocate again, should use the never-used entry
  size_t end_offset = MEMFAULT_ARRAY_SIZE(g_memfault_heap_stats_pool) - 1;
  MEMFAULT_HEAP_STATS_MALLOC((void *)((uintptr_t)expected_heap_stats.ptr + end_offset),
                             expected_heap_stats.info.size + end_offset);
  reference_pool[end_offset] = g_memfault_heap_stats_pool[end_offset];

  // walk the allocated entries and confirm correct values
  for (size_t i = 1; i < MEMFAULT_ARRAY_SIZE(g_memfault_heap_stats_pool); i++) {
    bool match = prv_heap_stat_equality(&reference_pool[i], &g_memfault_heap_stats_pool[i]);
    if (!match) {
      fprintf(stderr, "Mismatch at index %zu\n", i);
    }
    CHECK(match);
  }
  // Now allocate again, and confirm the "unused" entry is used now.
  MEMFAULT_HEAP_STATS_MALLOC((void *)((uintptr_t)expected_heap_stats.ptr + end_offset + 1),
                             expected_heap_stats.info.size + end_offset + 1);
  const sMfltHeapStatEntry expected = {
    .lr = lr,
    .ptr = (void *)(0x12345678 + end_offset + 1),
    .info = {
      .size = 1234 + (uint32_t)end_offset + 1,
      .in_use = 1,
    },
  };
  CHECK(prv_heap_stat_equality(&expected, &g_memfault_heap_stats_pool[0]));
}

static void prv_check_list_end(void) {
  bool list_end_found = false;
  if (g_memfault_heap_stats.stats_pool_head == MEMFAULT_HEAP_STATS_LIST_END) {
    printf("Found end at head\n");
    list_end_found = true;
  } else {
    uint16_t current_index = g_memfault_heap_stats.stats_pool_head;
    for (size_t i = 0; i < MEMFAULT_HEAP_STATS_MAX_COUNT; i++) {
      uint16_t next_index = g_memfault_heap_stats_pool[current_index].info.next_entry_index;
      if (next_index == MEMFAULT_HEAP_STATS_LIST_END) {
        printf("Found end at %u\n", current_index);
        list_end_found = true;
        break;
      }
      current_index = next_index;
    }
  }
  CHECK(list_end_found);
}

static void prv_print_list(void) {
  uint16_t current_index = g_memfault_heap_stats.stats_pool_head;
  printf("List: ");
  for (size_t i = 0; i < MEMFAULT_HEAP_STATS_MAX_COUNT; i++) {
    printf("%hu", current_index);
    if (current_index == MEMFAULT_HEAP_STATS_LIST_END) {
      break;
    }
    printf("[%s] ", g_memfault_heap_stats_pool[current_index].info.in_use ? "used" : "free");
    current_index = g_memfault_heap_stats_pool[current_index].info.next_entry_index;
  }
  printf("\n");
}

static void prv_print_heap_stats(void) {
  printf("Heap Stats: ");
  for (size_t i = 0; i < MEMFAULT_HEAP_STATS_MAX_COUNT; i++) {
    sMfltHeapStatEntry *entry = &g_memfault_heap_stats_pool[i];
    printf("Entry[%lu]{lr[%p], ptr[%p], size[%d], used:[%d], next:[%hu]} ", i, entry->lr,
           entry->ptr, entry->info.size, entry->info.in_use, entry->info.next_entry_index);
  }
  printf("\n");
}

static void prv_check_for_end(void) {
  bool result = false;
  uint16_t current_index = g_memfault_heap_stats.stats_pool_head;
  for (size_t i = 0; i < MEMFAULT_HEAP_STATS_MAX_COUNT; i++) {
    if (current_index == MEMFAULT_HEAP_STATS_LIST_END) {
      result = true;
      break;
    }
    current_index = g_memfault_heap_stats_pool[current_index].info.next_entry_index;
  }
  if (current_index == MEMFAULT_HEAP_STATS_LIST_END) {
    result = true;
  }
  CHECK(result);
}

static void prv_run_list_checks(void) {
  prv_print_list();
  prv_print_heap_stats();
  prv_check_list_end();
  prv_check_for_end();
}

TEST(MemfaultHeapStats, Test_NoCycle) {
  // Setup the random generator
  std::random_device rd;
  std::mt19937 gen(rd());

  // Create a distribution of addresses between 1 (0 would be NULL which is invalid) and the max
  // number of entries + 1. This gives a range of MEMFAULT_HEAP_STATS_MAX_COUNT
  std::uniform_int_distribution<unsigned long> heap_stats_entry_address_generator(
    1, MEMFAULT_HEAP_STATS_MAX_COUNT + 1);
  std::uniform_int_distribution<> malloc_operation_generator(0, 1);

  // Test pathological case where entries are freed in reverse order (newest first)
  for (uintptr_t i = 0; i < MEMFAULT_HEAP_STATS_MAX_COUNT; i++) {
    MEMFAULT_HEAP_STATS_MALLOC((void *)(i + 1), i + 2);
  }
  prv_run_list_checks();

  // Free every entry at the head of the list until empty
  for (uintptr_t i = 0; i < MEMFAULT_HEAP_STATS_MAX_COUNT; i++) {
    MEMFAULT_HEAP_STATS_FREE((void *)(MEMFAULT_HEAP_STATS_MAX_COUNT - i));
  }
  prv_run_list_checks();
  CHECK(g_memfault_heap_stats.stats_pool_head == MEMFAULT_HEAP_STATS_LIST_END);

  // Fill entries again, and then randomly malloc and free many times
  for (uintptr_t i = 0; i < MEMFAULT_HEAP_STATS_MAX_COUNT * 10; i++) {
    MEMFAULT_HEAP_STATS_MALLOC((void *)(i + 1 + MEMFAULT_HEAP_STATS_MAX_COUNT), i + 2);
  }

  prv_run_list_checks();

  for (size_t i = 0; i < 10 * MEMFAULT_HEAP_STATS_MAX_COUNT; i++) {
    unsigned long entry_addr = heap_stats_entry_address_generator(gen);
    if (malloc_operation_generator(gen)) {
      MEMFAULT_HEAP_STATS_MALLOC((void *)entry_addr, entry_addr + 2);
    } else {
      MEMFAULT_HEAP_STATS_FREE((void *)entry_addr);
    }
    prv_run_list_checks();
  }
}
