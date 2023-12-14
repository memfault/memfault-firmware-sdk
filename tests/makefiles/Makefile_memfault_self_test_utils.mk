SRC_FILES = \
  $(MFLT_COMPONENTS_DIR)/core/src/memfault_self_test_utils.c \

TEST_SRC_FILES = \
  $(MFLT_TEST_SRC_DIR)/test_memfault_self_test_utils.cpp \

include $(CPPUTEST_MAKFILE_INFRA)
