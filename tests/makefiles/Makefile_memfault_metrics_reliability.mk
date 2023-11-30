SRC_FILES = \
  $(MFLT_COMPONENTS_DIR)/metrics/src/memfault_metrics_reliability.c \

MOCK_AND_FAKE_SRC_FILES += \
  $(MFLT_TEST_MOCK_DIR)/mock_memfault_metrics.cpp \
  $(MFLT_TEST_MOCK_DIR)/mock_memfault_reboot_tracking.cpp \
  $(MFLT_TEST_STUB_DIR)/stub_memfault_log_save.c \

TEST_SRC_FILES = \
  $(MFLT_TEST_SRC_DIR)/test_memfault_metrics_reliability.cpp \
  $(MOCK_AND_FAKE_SRC_FILES)

include $(CPPUTEST_MAKFILE_INFRA)
