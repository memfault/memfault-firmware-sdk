COMPONENT_NAME=memfault_trace_event

SRC_FILES = \
  $(MFLT_COMPONENTS_DIR)/core/src/memfault_serializer_helper.c \
  $(MFLT_COMPONENTS_DIR)/core/src/memfault_trace_event.c \
  $(MFLT_COMPONENTS_DIR)/util/src/memfault_minimal_cbor.c

MOCK_AND_FAKE_SRC_FILES += \
  $(MFLT_TEST_FAKE_DIR)/fake_memfault_event_storage.cpp \
  $(MFLT_TEST_FAKE_DIR)/fake_memfault_platform_debug_log.c \
  $(MFLT_TEST_FAKE_DIR)/fake_memfault_platform_get_device_info.c \
  $(MFLT_TEST_FAKE_DIR)/fake_memfault_platform_time.c \

TEST_SRC_FILES = \
  $(MFLT_TEST_SRC_DIR)/test_memfault_trace_event.cpp \
  $(MOCK_AND_FAKE_SRC_FILES)

CPPUTEST_CPPFLAGS += -DMEMFAULT_TRACE_REASON_USER_DEFS_FILE=\"memfault_trace_reason_user_config.def\"
CPPUTEST_CPPFLAGS += -DMEMFAULT_EVENT_INCLUDE_DEVICE_SERIAL=1
include $(CPPUTEST_MAKFILE_INFRA)
