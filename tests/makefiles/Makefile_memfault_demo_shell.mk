SRC_FILES = \
  $(MFLT_COMPONENTS_DIR)/demo/src/memfault_demo_shell.c

MOCK_AND_FAKE_SRC_FILES +=

TEST_SRC_FILES = \
  $(MFLT_TEST_SRC_DIR)/test_memfault_demo_shell.c \
  $(MOCK_AND_FAKE_SRC_FILES)

# add the -DMEMFAULT_DEMO_SHELL_COMMAND_EXTENSIONS=1 define to enable the
# extended commands
CPPUTEST_CPPFLAGS += -DMEMFAULT_DEMO_SHELL_COMMAND_EXTENSIONS=1

include $(CPPUTEST_MAKFILE_INFRA)
