COMPONENT_NAME=memfault_metrics_serializer

SRC_FILES = \
  $(MFLT_COMPONENTS_DIR)/_metrics/src/memfault_metrics_serializer.c

MOCK_AND_FAKE_SRC_FILES += \
  $(MFLT_COMPONENTS_DIR)/util/src/memfault_minimal_cbor.c \
  $(MFLT_TEST_FAKE_DIR)/fake_memfault_platform_debug_log.c \

TEST_SRC_FILES = \
  $(MFLT_TEST_SRC_DIR)/test_memfault_metrics_serializer.cpp \
  $(MFLT_TEST_FAKE_DIR)/fake_memfault_platform_get_device_info.c \
  $(MOCK_AND_FAKE_SRC_FILES)

include $(CPPUTEST_MAKFILE_INFRA)
