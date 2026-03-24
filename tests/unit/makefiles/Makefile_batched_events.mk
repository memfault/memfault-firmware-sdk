SRC_FILES = \
  $(MFLT_COMPONENTS_DIR)/core/src/memfault_batched_events.c

TEST_SRC_FILES = \
  $(MFLT_COMPONENTS_DIR)/util/src/memfault_minimal_cbor.c \
  $(MFLT_TEST_FAKE_DIR)/fake_memfault_sdk_assert.c \
  $(MFLT_TEST_SRC_DIR)/test_memfault_batched_events.cpp


include $(CPPUTEST_MAKFILE_INFRA)
