SRC_FILES = \
  $(MFLT_COMPONENTS_DIR)/core/src/memfault_build_id.c \
  $(MFLT_COMPONENTS_DIR)/core/src/memfault_core_utils.c

MOCK_AND_FAKE_SRC_FILES += \
  $(MFLT_TEST_MOCK_DIR)/mock_memfault_platform_debug_log.cpp \
  $(MFLT_TEST_STUB_DIR)/stub_memfault_log_save.c \

TEST_SRC_FILES = \
  $(MFLT_TEST_SRC_DIR)/test_memfault_build_id.cpp \
  $(MOCK_AND_FAKE_SRC_FILES)

CPPUTEST_CPPFLAGS += -DMEMFAULT_USE_GNU_BUILD_ID=1

include $(CPPUTEST_MAKFILE_INFRA)
