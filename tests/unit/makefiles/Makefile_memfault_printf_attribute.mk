# Add a file to create dummy TARGET_LIB
# We don't actually call anything from this lib so just add
# the current source
SRC_FILES = \
  $(MFLT_TEST_SRC_DIR)/test_memfault_printf_attribute.cpp

TEST_SRC_FILES = \
  $(MFLT_TEST_SRC_DIR)/test_memfault_printf_attribute.cpp \
  $(MOCK_AND_FAKE_SRC_FILES)

include $(CPPUTEST_MAKFILE_INFRA)
