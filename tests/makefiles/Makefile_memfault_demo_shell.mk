COMPONENT_NAME=memfault_demo_shell

SRC_FILES = \
  $(MFLT_COMPONENTS_DIR)/demo/src/memfault_demo_shell.c

MOCK_AND_FAKE_SRC_FILES +=

TEST_SRC_FILES = \
  $(MFLT_TEST_SRC_DIR)/test_memfault_demo_shell.c \
  $(MOCK_AND_FAKE_SRC_FILES)

include $(CPPUTEST_MAKFILE_INFRA)
