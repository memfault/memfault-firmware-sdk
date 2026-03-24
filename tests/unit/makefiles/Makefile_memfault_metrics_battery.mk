SRC_FILES = \
  $(MFLT_COMPONENTS_DIR)/metrics/src/memfault_metrics_battery.c \

MOCK_AND_FAKE_SRC_FILES = \
  $(MFLT_TEST_STUB_DIR)/stub_memfault_log_save.c \
  $(MFLT_TEST_MOCK_DIR)/mock_memfault_metrics.cpp \
  $(MFLT_TEST_FAKE_DIR)/fake_memfault_platform_debug_log.c \

TEST_SRC_FILES = \
$(MFLT_TEST_SRC_DIR)/test_memfault_metrics_battery.cpp \
  $(MOCK_AND_FAKE_SRC_FILES)

CPPUTEST_CPPFLAGS += -DMEMFAULT_METRICS_BATTERY_ENABLE=1
include $(CPPUTEST_MAKFILE_INFRA)
