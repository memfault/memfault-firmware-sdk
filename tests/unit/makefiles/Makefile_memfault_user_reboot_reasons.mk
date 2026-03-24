TEST_SRC_FILES = \
  $(MFLT_TEST_SRC_DIR)/test_memfault_user_reboot_reasons.cpp

# macos ar does not allow empty archives, add a stub as the source to work around
SRC_FILES = \
  $(MFLT_TEST_STUB_DIR)/stub_assert.c

include $(CPPUTEST_MAKFILE_INFRA)
