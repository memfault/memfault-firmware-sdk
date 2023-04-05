SRC_FILES = \
  $(MFLT_COMPONENTS_DIR)/util/src/memfault_base64.c \
  $(MFLT_COMPONENTS_DIR)/core/src/memfault_log.c \
  $(MFLT_COMPONENTS_DIR)/core/src/memfault_log_data_source.c \
  $(MFLT_COMPONENTS_DIR)/core/src/memfault_serializer_helper.c \
  $(MFLT_COMPONENTS_DIR)/util/src/memfault_circular_buffer.c \
  $(MFLT_COMPONENTS_DIR)/util/src/memfault_crc16_ccitt.c \
  $(MFLT_COMPONENTS_DIR)/util/src/memfault_minimal_cbor.c \

MOCK_AND_FAKE_SRC_FILES += \
  $(MFLT_TEST_FAKE_DIR)/fake_memfault_build_id.c \
  $(MFLT_TEST_FAKE_DIR)/fake_memfault_platform_debug_log.c \
  $(MFLT_TEST_FAKE_DIR)/fake_memfault_platform_get_device_info.c \
  $(MFLT_TEST_FAKE_DIR)/fake_memfault_platform_locking.c \
  $(MFLT_TEST_FAKE_DIR)/fake_memfault_platform_time.c \
  $(MFLT_TEST_FAKE_DIR)/fake_memfault_sdk_assert.c

TEST_SRC_FILES = \
  $(MFLT_TEST_SRC_DIR)/test_memfault_log_data_source.cpp \
  $(MOCK_AND_FAKE_SRC_FILES)

include $(CPPUTEST_MAKFILE_INFRA)
