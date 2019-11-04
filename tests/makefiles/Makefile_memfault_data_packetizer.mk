COMPONENT_NAME=memfault_data_packetizer

SRC_FILES = \
  $(MFLT_COMPONENTS_DIR)/core/src/memfault_data_packetizer.c

MOCK_AND_FAKE_SRC_FILES += \
  $(MFLT_TEST_FAKE_DIR)/fake_memfault_platform_debug_log.c

TEST_SRC_FILES = \
  $(MFLT_TEST_SRC_DIR)/test_memfault_data_packetizer.cpp \
  $(MOCK_AND_FAKE_SRC_FILES)

include $(CPPUTEST_MAKFILE_INFRA)
