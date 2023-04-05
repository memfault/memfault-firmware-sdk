SRC_FILES = \
  $(MFLT_COMPONENTS_DIR)/util/src/memfault_base64.c \
  $(MFLT_COMPONENTS_DIR)/core/src/memfault_log.c \
  $(MFLT_COMPONENTS_DIR)/util/src/memfault_minimal_cbor.c

MOCK_AND_FAKE_SRC_FILES += \
  $(MFLT_COMPONENTS_DIR)/util/src/memfault_circular_buffer.c \
  $(MFLT_COMPONENTS_DIR)/util/src/memfault_crc16_ccitt.c \
  $(MFLT_TEST_MOCK_DIR)/mock_memfault_platform_debug_log.cpp \
  $(MFLT_TEST_FAKE_DIR)/fake_memfault_platform_locking.c

TEST_SRC_FILES = \
  $(MFLT_TEST_SRC_DIR)/test_memfault_log.cpp \
  $(MOCK_AND_FAKE_SRC_FILES)

CPPUTEST_CPPFLAGS += -DMEMFAULT_COMPACT_LOG_ENABLE=1
CPPUTEST_CPPFLAGS += -DMEMFAULT_LOG_EXPORT_CHUNK_MAX_LEN=10

include $(CPPUTEST_MAKFILE_INFRA)
