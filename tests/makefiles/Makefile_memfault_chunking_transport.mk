SRC_FILES = \
  $(MFLT_COMPONENTS_DIR)/util/src/memfault_chunk_transport.c

MOCK_AND_FAKE_SRC_FILES += \
  $(MFLT_TEST_FAKE_DIR)/fake_memfault_platform_debug_log.c \
  $(MFLT_COMPONENTS_DIR)/util/src/memfault_varint.c \
  $(MFLT_COMPONENTS_DIR)/util/src/memfault_crc16_ccitt.c

TEST_SRC_FILES = \
  $(MFLT_TEST_SRC_DIR)/test_memfault_chunk_transport.cpp \
  $(MOCK_AND_FAKE_SRC_FILES)

include $(CPPUTEST_MAKFILE_INFRA)
