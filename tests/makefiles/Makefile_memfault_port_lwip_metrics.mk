SRC_FILES = \
	$(MFLT_PORTS_DIR)/lwip/memfault_lwip_metrics.c \

INCLUDE_DIRS = \
	$(MFLT_PORTS_DIR)/lwip/config \

MOCK_AND_FAKE_SRC_FILES = \
	$(MFLT_TEST_MOCK_DIR)/mock_memfault_metrics.cpp

TEST_SRC_FILES = \
	$(MFLT_TEST_SRC_DIR)/test_memfault_port_lwip_metrics.cpp \
	$(MOCK_AND_FAKE_SRC_FILES)

CPPUTEST_CPPFLAGS += -DTEST_LWIP_METRICS=1
include $(CPPUTEST_MAKFILE_INFRA)
