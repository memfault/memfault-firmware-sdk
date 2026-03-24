SRC_FILES = \
  $(MFLT_COMPONENTS_DIR)/core/src/memfault_data_packetizer.c

MOCK_AND_FAKE_SRC_FILES += \
  $(MFLT_TEST_FAKE_DIR)/fake_memfault_platform_debug_log.c \
  $(MFLT_TEST_STUB_DIR)/stub_memfault_log_save.c \

TEST_SRC_FILES = \
  $(MFLT_TEST_SRC_DIR)/test_memfault_data_packetizer.cpp \
  $(MOCK_AND_FAKE_SRC_FILES)

CPPUTEST_CPPFLAGS += \
  -DMEMFAULT_PROJECT_KEY="\"1234567890abcdef1234567890abcdef\"" \
  -DMEMFAULT_MESSAGE_HEADER_CONTAINS_PROJECT_KEY=1 \


include $(CPPUTEST_MAKFILE_INFRA)
