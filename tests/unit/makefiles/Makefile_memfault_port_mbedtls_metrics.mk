SRC_FILES = \
	$(MFLT_PORTS_DIR)/mbedtls/memfault_mbedtls_metrics.c \
	$(MFLT_TEST_STUB_DIR)/stub_mbedtls_mem.c \

INCLUDE_DIRS = \
	$(MFLT_PORTS_DIR)/mbedtls/config \

MOCK_AND_FAKE_SRC_FILES = \
	$(MFLT_TEST_MOCK_DIR)/mock_memfault_metrics.cpp \

TEST_SRC_FILES = \
	$(MFLT_TEST_SRC_DIR)/test_memfault_port_mbedtls_metrics.cpp \
	$(MOCK_AND_FAKE_SRC_FILES) \

CPPUTEST_CPPFLAGS += \
	-DTEST_MBEDTLS_METRICS=1 \

CPPUTEST_CFLAGS += \
	-include $(MFLT_TEST_ROOT)/stub_includes/mbedtls_mem.h

include $(CPPUTEST_MAKFILE_INFRA)
