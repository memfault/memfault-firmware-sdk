SRC_FILES = \
  $(MFLT_COMPONENTS_DIR)/panics/src/memfault_coredump.c

MOCK_AND_FAKE_SRC_FILES += \
  $(MFLT_TEST_FAKE_DIR)/fake_memfault_platform_coredump_storage.c

TEST_SRC_FILES = \
  $(MFLT_TEST_SRC_DIR)/test_memfault_coredump.cpp \
  $(MOCK_AND_FAKE_SRC_FILES)

include $(CPPUTEST_MAKFILE_INFRA)
