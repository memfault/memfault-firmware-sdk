COMPONENT_NAME=memfault_demo_cli

SRC_FILES = \
  $(MFLT_COMPONENTS_DIR)/demo/src/memfault_demo_cli_print_chunk.c \
  $(MFLT_COMPONENTS_DIR)/http/src/memfault_http_client.c

MOCK_AND_FAKE_SRC_FILES += \
  $(MFLT_COMPONENTS_DIR)/core/src/memfault_data_packetizer.c \
  $(MFLT_COMPONENTS_DIR)/util/src/memfault_chunk_transport.c \
  $(MFLT_COMPONENTS_DIR)/util/src/memfault_crc16_ccitt.c \
  $(MFLT_COMPONENTS_DIR)/util/src/memfault_varint.c \
  $(MFLT_COMPONENTS_DIR)/demo/src/http/memfault_demo_http.c \
  $(MFLT_TEST_FAKE_DIR)/fake_memfault_platform_get_device_info.c \
  $(MFLT_TEST_FAKE_DIR)/fake_memfault_platform_http_client.c \
  $(MFLT_TEST_MOCK_DIR)/mock_memfault_coredump.cpp \
  $(MFLT_TEST_MOCK_DIR)/mock_memfault_platform_debug_log.cpp \
  $(MFLT_TEST_STUB_DIR)/stub_memfault_log_save.c \

TEST_SRC_FILES = \
  $(MFLT_TEST_SRC_DIR)/test_memfault_demo_cli.cpp \
  $(MOCK_AND_FAKE_SRC_FILES)

# Allow cast from (const char **) to (char **)
CPPUTEST_CPPFLAGS += -Wno-c++11-compat-deprecated-writable-strings -Wno-write-strings

include $(CPPUTEST_MAKFILE_INFRA)
