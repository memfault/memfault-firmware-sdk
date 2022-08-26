SRC_FILES = \
  $(MFLT_PORTS_DIR)/panics/src/memfault_platform_ram_backed_coredump.c

TEST_SRC_FILES = \
  $(MFLT_TEST_SRC_DIR)/test_memfault_ram_backed_coredump_port.cpp

include $(CPPUTEST_MAKFILE_INFRA)
