SRC_FILES = \
  $(MFLT_COMPONENTS_DIR)/core/src/memfault_data_export.c

MOCK_AND_FAKE_SRC_FILES += \
  $(MFLT_COMPONENTS_DIR)/util/src/memfault_base64.c \
  $(MFLT_TEST_FAKE_DIR)/fake_memfault_platform_debug_log.c \
  $(MFLT_TEST_FAKE_DIR)/fake_memfault_sdk_assert.c \
  $(MFLT_TEST_STUB_DIR)/stub_memfault_log_save.c \

TEST_SRC_FILES = \
  $(MOCK_AND_FAKE_SRC_FILES) \
  $(MFLT_TEST_SRC_DIR)/test_memfault_data_export.cpp \

include $(CPPUTEST_MAKFILE_INFRA)
