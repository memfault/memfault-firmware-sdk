SRC_FILES = \
  $(MFLT_COMPONENTS_DIR)/core/src/memfault_self_test.c \
  $(MFLT_COMPONENTS_DIR)/core/src/memfault_self_test_utils.c \

MOCK_AND_FAKE_SRC_FILES += \
  $(MFLT_TEST_FAKE_DIR)/fake_memfault_build_id.c \
  $(MFLT_TEST_FAKE_DIR)/fake_memfault_coredump_utils.c \
  $(MFLT_TEST_FAKE_DIR)/fake_memfault_platform_boot_time.c \
  $(MFLT_TEST_FAKE_DIR)/fake_memfault_platform_debug_log.c \
  $(MFLT_TEST_FAKE_DIR)/fake_memfault_platform_get_device_info.c \
  $(MFLT_TEST_FAKE_DIR)/fake_memfault_platform_time.c \
  $(MFLT_TEST_FAKE_DIR)/fake_memfault_sdk_assert.c \
  $(MFLT_TEST_MOCK_DIR)/mock_memfault_reboot_tracking.cpp \
  $(MFLT_TEST_STUB_DIR)/stub_memfault_log_save.c \
  $(MFLT_TEST_STUB_DIR)/stub_component_booted.c \
  $(MFLT_TEST_STUB_DIR)/stub_memfault_coredump_regions.c \
  $(MFLT_TEST_STUB_DIR)/stub_memfault_coredump_storage_debug.c \
  $(MFLT_TEST_STUB_DIR)/stub_memfault_coredump.c \

TEST_SRC_FILES = \
  $(MFLT_TEST_SRC_DIR)/test_memfault_self_test_reboot_reason.cpp \
  $(MOCK_AND_FAKE_SRC_FILES)

# Needed to prevent error on memfault_platform_reboot stub returning
CPPUTEST_CPPFLAGS += -DMEMFAULT_NORETURN=""

include $(CPPUTEST_MAKFILE_INFRA)
