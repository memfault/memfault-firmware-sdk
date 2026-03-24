SRC_FILES = \
  $(MFLT_COMPONENTS_DIR)/core/src/memfault_ram_reboot_info_tracking.c

TEST_SRC_FILES = \
  $(MFLT_TEST_SRC_DIR)/test_memfault_ram_reboot_tracking.cpp

include $(CPPUTEST_MAKFILE_INFRA)
