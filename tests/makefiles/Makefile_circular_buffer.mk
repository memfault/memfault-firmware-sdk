COMPONENT_NAME=memfault_circular_buffer

SRC_FILES = \
  $(MFLT_COMPONENTS_DIR)/util/src/memfault_circular_buffer.c

TEST_SRC_FILES = \
  $(MFLT_TEST_SRC_DIR)/test_memfault_circular_buffer.c

include $(CPPUTEST_MAKFILE_INFRA)
