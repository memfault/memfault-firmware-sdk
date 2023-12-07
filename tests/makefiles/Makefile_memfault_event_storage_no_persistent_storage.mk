SRC_FILES = \
  $(MFLT_COMPONENTS_DIR)/core/src/memfault_event_storage.c

MOCK_AND_FAKE_SRC_FILES += \
  $(MFLT_COMPONENTS_DIR)/util/src/memfault_circular_buffer.c \
  $(MFLT_COMPONENTS_DIR)/util/src/memfault_minimal_cbor.c \
  $(MFLT_TEST_FAKE_DIR)/fake_memfault_platform_debug_log.c \
  $(MFLT_TEST_FAKE_DIR)/fake_memfault_platform_locking.c \
  $(MFLT_TEST_FAKE_DIR)/fake_memfault_sdk_assert.c

TEST_SRC_FILES = \
  $(MFLT_TEST_SRC_DIR)/test_memfault_event_storage.cpp \
  $(MOCK_AND_FAKE_SRC_FILES)

CPPUTEST_CPPFLAGS += -DMEMFAULT_TEST_PERSISTENT_EVENT_STORAGE_DISABLE=1
CPPUTEST_CPPFLAGS += -DMEMFAULT_EVENT_STORAGE_READ_BATCHING_ENABLED=0

include $(CPPUTEST_MAKFILE_INFRA)
