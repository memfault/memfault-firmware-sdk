SRC_FILES = \
  $(MFLT_COMPONENTS_DIR)/core/src/memfault_compact_log_serializer.c

MOCK_AND_FAKE_SRC_FILES += \
  $(MFLT_COMPONENTS_DIR)/util/src/memfault_minimal_cbor.c \

TEST_SRC_FILES = \
  $(MFLT_TEST_SRC_DIR)/test_memfault_compact_log_serializer.cpp \
  $(MOCK_AND_FAKE_SRC_FILES)

CPPUTEST_CPPFLAGS += -DMEMFAULT_COMPACT_LOG_ENABLE=1

include $(CPPUTEST_MAKFILE_INFRA)
