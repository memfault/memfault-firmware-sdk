SRC_FILES = \
  $(MFLT_COMPONENTS_DIR)/core/src/memfault_data_source_rle.c \
  $(MFLT_COMPONENTS_DIR)/util/src/memfault_rle.c \
  $(MFLT_COMPONENTS_DIR)/util/src/memfault_varint.c

TEST_SRC_FILES = \
  $(MFLT_TEST_SRC_DIR)/test_memfault_data_source_rle.cpp

include $(CPPUTEST_MAKFILE_INFRA)
