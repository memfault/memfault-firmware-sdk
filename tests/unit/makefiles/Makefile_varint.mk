SRC_FILES = \
  $(MFLT_COMPONENTS_DIR)/util/src/memfault_varint.c

TEST_SRC_FILES = \
  $(MFLT_TEST_SRC_DIR)/test_memfault_varint.cpp

include $(CPPUTEST_MAKFILE_INFRA)
