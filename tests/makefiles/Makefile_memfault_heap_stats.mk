SRC_FILES = \
  $(MFLT_COMPONENTS_DIR)/core/src/memfault_heap_stats.c

MOCK_AND_FAKE_SRC_FILES += \
  $(MFLT_TEST_FAKE_DIR)/fake_memfault_platform_locking.c

TEST_SRC_FILES = \
  $(MFLT_TEST_SRC_DIR)/test_memfault_heap_stats.cpp \
  $(MOCK_AND_FAKE_SRC_FILES)

include $(CPPUTEST_MAKFILE_INFRA)
