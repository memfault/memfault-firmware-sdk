SRC_FILES = \
  $(MFLT_COMPONENTS_DIR)/util/src/memfault_base64.c

TEST_SRC_FILES = \
  $(MFLT_TEST_SRC_DIR)/test_memfault_base64.cpp

include $(CPPUTEST_MAKFILE_INFRA)
