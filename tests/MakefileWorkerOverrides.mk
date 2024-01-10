# where the cpputest includes and *.a are located
# The defaults are what CI uses
#
# For osx, the brew path will look something like /usr/local/Cellar/cpputest/3.8
CPPUTEST_HOME ?= /usr

MKFILE_PATH := $(abspath $(lastword $(MAKEFILE_LIST)))
CURRENT_DIR := $(dir $(MKFILE_PATH))

# Explicitly insert the cpputest lib directory to search paths; this supports
# the conda use case, where the libs are not set to the system linker
LD_LIBRARIES = -L$(CPPUTEST_HOME)/lib -lCppUTest -lCppUTestExt

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

# Clang defaults to c++98 but all modern cpp compilers implement most of the features (such as
# variadic macros) introduced in c99 that were later added as part of the c++11 specification
# so we pin the unit tests cpp standard to c++11.
#
# memfault/core/compact_log_helpers.h makes use of the gnu expansion rules for ,## __VA_ARGS__. We
# explicitly opt in unit tests that need this feature with different gnu standard args for these
# tests
ifeq (,$(findstring MEMFAULT_COMPACT_LOG_ENABLE=1,$(CPPUTEST_CPPFLAGS)))
CPPUTEST_CXXFLAGS += -std=c++11
else
CPPUTEST_CFLAGS += -std=gnu11
CPPUTEST_CXXFLAGS += -std=gnu++11
endif

export SILENCE ?= @

export CPPUTEST_USE_EXTENSIONS=Y
export CPPUTEST_USE_MEM_LEAK_DETECTION=Y
export CPPUTEST_USE_GCOV=Y
# Enable branch coverage reporting
export GCOV_ARGS=-b -c

CPPUTEST_WARNINGFLAGS = \
  -Wall \
  -Wextra \
  -Werror \
  -Wshadow \
  -Wswitch-default

CPPUTEST_CFLAGS += \
  -Wbad-function-cast

# These warnings aren't particularly helpful for our use case so we disable them
CPPUTEST_WARNINGFLAGS += \
  -Wno-missing-braces \
  -Wno-missing-field-initializers \
  -Wno-packed \
  -Wno-vla \

CC_VERSION_OUTPUT ="$(shell $(CXX) -v 2>&1)"
CLANG_STR = clang
ifeq ($(findstring $(CLANG_STR),$(CC_VERSION_OUTPUT)),$(CLANG_STR))
COMPILER_SPECIFIC_WARNINGS += \
  -Wno-bad-function-cast \
  -Wno-c++11-extensions \
  -Wno-c++98-compat \
  -Wno-c++98-compat-pedantic \
  -Wno-c11-extensions \
  -Wno-c99-extensions \
  -Wno-covered-switch-default \
  -Wno-documentation \
  -Wno-documentation-unknown-command \
  -Wno-flexible-array-extensions \
  -Wno-gnu-variable-sized-type-not-at-end \
  -Wno-gnu-zero-variadic-macro-arguments \
  -Wno-inconsistent-missing-destructor-override \
  -Wno-keyword-macro \
  -Wno-missing-noreturn \
  -Wno-reserved-id-macro \
  -Wno-shorten-64-to-32 \
  -Wno-vla-extension \
  -Wno-zero-as-null-pointer-constant \
  -Wno-unknown-warning-option \
  -Wno-poison-system-directories \
  -Wno-suggest-override \
  -Wno-declaration-after-statement \
  -Wno-reserved-identifier \
  -Wno-zero-length-array \
  -Wno-unsafe-buffer-usage \
  -Wno-cast-function-type-strict \

else

# GCC-only warnings
COMPILER_SPECIFIC_WARNINGS += \
  -Wformat-signedness \

# Only enable -fanalyzer if GCC version is >= 12
GCC_VERSION_GTEQ_12 := $(shell expr $$($(CC) -dumpversion | cut -f1 -d.) \>= 12)
ifeq "$(GCC_VERSION_GTEQ_12)" "1"
CFLAGS += \
  -fanalyzer
endif

# Permit disabling the sanitizers via environment variable
ifneq ($(MEMFAULT_DISABLE_ASAN),1)
# Enable sanitizers, and crash on error (don't attempt to recover sanely) so the
# test fails on sanitizer violation
CPPUTEST_WARNINGFLAGS += \
  -fsanitize=address \
  -fsanitize=undefined \
  -fno-sanitize-recover=all

CPPUTEST_LDFLAGS += \
  -fsanitize=address \
  -fsanitize=undefined \
  -fno-sanitize-recover=all
endif


endif

CPPUTEST_WARNINGFLAGS += $(COMPILER_SPECIFIC_WARNINGS)

export CPPUTEST_WARNINGFLAGS

export CPPUTEST_LDFLAGS

MEMFAULT_RESULT_DIR=$(BUILD_DIR)/$(COMPONENT_NAME)

export TEST_TARGET=$(MEMFAULT_RESULT_DIR)/$(COMPONENT_NAME)_tests
export CPPUTEST_OBJS_DIR=$(MEMFAULT_RESULT_DIR)/objs
export CPPUTEST_LIB_DIR=$(MEMFAULT_RESULT_DIR)/lib

COV_INCLUDE_FILES = $(notdir $(SRC_FILES))
COV_INCLUDE_ARG = $(patsubst %.c, --include *%.c, $(COV_INCLUDE_FILES))

# run MakefileWorker.mk with the variables defined here
include ${CURRENT_DIR}/MakefileWorker.mk
