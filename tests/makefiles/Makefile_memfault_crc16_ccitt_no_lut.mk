COMPONENT_NAME=memfault_crc16_ccitt_no_lut

SRC_FILES = \
  $(MFLT_COMPONENTS_DIR)/util/src/memfault_crc16_ccitt.c

TEST_SRC_FILES = \
  $(MFLT_TEST_SRC_DIR)/test_memfault_crc16_ccitt.cpp

CPPUTEST_CPPFLAGS += -DMEMFAULT_CRC16_LOOKUP_TABLE_ENABLE=0

include $(CPPUTEST_MAKFILE_INFRA)
