SRC_FILES = \
	$(MFLT_PORTS_DIR)/freertos/src/memfault_sdk_metrics_freertos.c \

INCLUDE_DIRS = \
	$(MFLT_PORTS_DIR)/freertos/config \

MOCK_AND_FAKE_SRC_FILES = \
	$(MFLT_TEST_MOCK_DIR)/mock_memfault_metrics.cpp

TEST_SRC_FILES = \
$(MFLT_TEST_SRC_DIR)/test_memfault_sdk_freertos_metrics.cpp \
	$(MOCK_AND_FAKE_SRC_FILES)

CPPUTEST_CPPFLAGS += -DTEST_FREERTOS_METRICS=1
include $(CPPUTEST_MAKFILE_INFRA)
