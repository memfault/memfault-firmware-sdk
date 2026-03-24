SRC_FILES = \
  $(MFLT_COMPONENTS_DIR)/core/src/memfault_task_watchdog.c

MOCK_AND_FAKE_SRC_FILES += \
  $(MFLT_TEST_FAKE_DIR)/fake_memfault_platform_locking.c \
  $(MFLT_TEST_MOCK_DIR)/mock_memfault_fault_handling.cpp

TEST_SRC_FILES = \
  $(MFLT_TEST_SRC_DIR)/test_memfault_task_watchdog.cpp \
  $(MOCK_AND_FAKE_SRC_FILES)

CPPUTEST_CPPFLAGS += -DMEMFAULT_TASK_WATCHDOG_ENABLE=1

# required for mock memfault_fault_handling_assert_extra
CPPUTEST_CPPFLAGS += -DMEMFAULT_NORETURN=""

include $(CPPUTEST_MAKFILE_INFRA)
