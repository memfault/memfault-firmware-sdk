SRC_FILES = \
  $(MFLT_COMPONENTS_DIR)/metrics/src/memfault_metrics_serializer.c

MOCK_AND_FAKE_SRC_FILES += \
  $(MFLT_COMPONENTS_DIR)/core/src/memfault_serializer_helper.c \
  $(MFLT_COMPONENTS_DIR)/util/src/memfault_minimal_cbor.c \
  $(MFLT_TEST_FAKE_DIR)/fake_memfault_build_id.c \
  $(MFLT_TEST_FAKE_DIR)/fake_memfault_event_storage.cpp \
  $(MFLT_TEST_FAKE_DIR)/fake_memfault_platform_debug_log.c \
  $(MFLT_TEST_FAKE_DIR)/fake_memfault_platform_time.c \
  $(MFLT_TEST_STUB_DIR)/stub_memfault_log_save.c \

TEST_SRC_FILES = \
  $(MFLT_TEST_SRC_DIR)/test_memfault_metrics_serializer.cpp \
  $(MFLT_TEST_FAKE_DIR)/fake_memfault_platform_get_device_info.c \
  $(MOCK_AND_FAKE_SRC_FILES)

include $(CPPUTEST_MAKFILE_INFRA)
