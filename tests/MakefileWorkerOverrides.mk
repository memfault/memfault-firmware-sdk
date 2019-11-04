# where the cpputest includes and *.a are located
# The defaults are what CI uses
#
# For osx, the brew path will look something like /usr/local/Cellar/cpputest/3.8
CPPUTEST_HOME ?= /usr
TARGET_PLATFORM ?= x86_64-linux-gnu

MKFILE_PATH := $(abspath $(lastword $(MAKEFILE_LIST)))
CURRENT_DIR := $(dir $(MKFILE_PATH))

LD_LIBRARIES = -L$(CPPUTEST_HOME)/$(TARGET_PLATFORM)lib -lCppUTest -lCppUTestExt

TEST_SRC_FILES += \
  $(MFLT_TEST_COMMON_SRC_DIR)/AllTests.cpp

MEMFAULT_EXTRA_INC_PATHS += \
  -I$(CPPUTEST_HOME)/include \
  -I$(PROJECT_DIR)/src \
  -I$(MFLT_TEST_ROOT)/ \
  -I$(MFLT_TEST_ROOT)/stub_includes \
  -I$(PROJECT_DIR)

CPPUTEST_CPPFLAGS += $(MEMFAULT_EXTRA_INC_PATHS) \
  -DMEMFAULT_UNITTEST

export SILENCE ?= @

export CPPUTEST_USE_EXTENSIONS=Y
export CPPUTEST_USE_MEM_LEAK_DETECTION=Y
export CPPUTEST_USE_GCOV=Y
# Enable branch coverage reporting
export GCOV_ARGS=-b -c

# These clang  warninsgs  aren't particularly helpful
CPPUTEST_WARNINGFLAGS = \
  -Wno-missing-braces \
  -Wno-missing-field-initializers \
  -Wno-packed \
  -Wno-switch-enum \
  -Wno-unused-parameter \
  -Wno-vla

CC_VERSION_OUTPUT ="$(shell $(CXX) -v 2>&1)"
CLANG_STR = clang
ifeq ($(findstring $(CLANG_STR),$(CC_VERSION_OUTPUT)),$(CLANG_STR))
COMPILER_SPECIFIC_WARNINGS += \
  -Wno-c++11-extensions		\
  -Wno-c11-extensions \
  -Wno-c99-extensions \
  -Wno-covered-switch-default \
  -Wno-documentation \
  -Wno-documentation-unknown-command \
  -Wno-flexible-array-extensions \
  -Wno-gnu-variable-sized-type-not-at-end \
  -Wno-keyword-macro \
  -Wno-reserved-id-macro \
  -Wno-shorten-64-to-32 \
  -Wno-vla-extension
endif

CPPUTEST_WARNINGFLAGS += $(COMPILER_SPECIFIC_WARNINGS)
CPPUTEST_WARNINGFLAGS += \
  -Werror \
  -fsanitize=address

export CPPUTEST_WARNINGFLAGS

CPPUTEST_LDFLAGS += \
  -fsanitize=address

export CPPUTEST_LDFLAGS

MEMFAULT_RESULT_DIR=$(BUILD_DIR)/$(COMPONENT_NAME)

export TEST_TARGET=$(MEMFAULT_RESULT_DIR)/$(COMPONENT_NAME)_tests
export CPPUTEST_OBJS_DIR=$(MEMFAULT_RESULT_DIR)/objs
export CPPUTEST_LIB_DIR=$(MEMFAULT_RESULT_DIR)/lib

COV_INCLUDE_FILES = $(notdir $(SRC_FILES))
COV_INCLUDE_ARG = $(patsubst %.c, --include *%.c, $(COV_INCLUDE_FILES))

# run MakefileWorker.mk with the variables defined here
include ${CURRENT_DIR}/MakefileWorker.mk
