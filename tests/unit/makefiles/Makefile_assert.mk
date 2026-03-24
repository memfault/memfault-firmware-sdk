# need to provide a stub object for cpputest to pack into an archive, to work
# around the behavior of the old 2005 version of 'ar' present on mac
SRC_FILES = \
  $(MFLT_TEST_STUB_DIR)/stub_assert.c

MOCK_AND_FAKE_SRC_FILES += \

TEST_SRC_FILES = \
  $(MFLT_TEST_SRC_DIR)/test_assert.cpp \
  $(MOCK_AND_FAKE_SRC_FILES)

# Blank out noreturn for this test
CPPUTEST_CPPFLAGS += -DMEMFAULT_NORETURN=""

include $(CPPUTEST_MAKFILE_INFRA)
