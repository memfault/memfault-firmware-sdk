SRC_FILES = \
  $(MFLT_COMPONENTS_DIR)/core/src/memfault_self_test.c \

MOCK_AND_FAKE_SRC_FILES += \
  $(MFLT_TEST_FAKE_DIR)/fake_memfault_platform_debug_log.cpp \
  $(MFLT_TEST_STUB_DIR)/stub_memfault_log_save.c \

TEST_SRC_FILES = \
  $(MFLT_TEST_SRC_DIR)/test_memfault_self_test.cpp \
  $(MOCK_AND_FAKE_SRC_FILES)

# Add flag to remove subtest definitions
CPPUTEST_CPPFLAGS += -DMEMFAULT_UNITTEST_SELF_TEST \
  -DMEMFAULT_NORETURN="" \
  -DMEMFAULT_DEMO_CLI_SELF_TEST_COREDUMP_STORAGE=1 \

include $(CPPUTEST_MAKFILE_INFRA)
