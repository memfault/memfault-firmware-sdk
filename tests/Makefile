# Collects all the Makefile_*.mk in this directory and then invokes them using
# recursive make (sorry). This let's us have unit tests for our different C SDK files!

TEST_MAKEFILE_ROOT := makefiles
TEST_MAKEFILE_FILTER ?= *

TEST_MAKEFILES := $(wildcard $(TEST_MAKEFILE_ROOT)/Makefile_$(TEST_MAKEFILE_FILTER))

MKFILE_PATH := $(abspath $(lastword $(MAKEFILE_LIST)))
CURRENT_DIR := $(dir $(MKFILE_PATH))

# Convenience defines that can be used for individual tests in makefiles/
export BUILD_DIR=build
export PROJECT_DIR := $(abspath $(CURRENT_DIR)/..)
export MFLT_TEST_ROOT := $(CURRENT_DIR)
export PROJECT_ROOT_DIR := $(PROJECT_DIR)
export MFLT_PORTS_DIR := $(PROJECT_DIR)/ports
export MFLT_COMPONENTS_DIR := $(PROJECT_DIR)/components
export MFLT_UTILS_SRC_DIR := $(MFLT_COMPONENTS_DIR)/util/src

export MFLT_TEST_SRC_DIR ?= $(MFLT_TEST_ROOT)/src
export MFLT_TEST_COMMON_SRC_DIR := $(MFLT_TEST_ROOT)/src
export MFLT_TEST_FAKE_DIR := $(MFLT_TEST_ROOT)/fakes
export MFLT_TEST_MOCK_DIR := $(MFLT_TEST_ROOT)/mocks
export MFLT_TEST_STUB_DIR := $(MFLT_TEST_ROOT)/stubs
export CPPUTEST_MAKFILE_INFRA := $(MFLT_TEST_ROOT)/MakefileWorkerOverrides.mk

# Gather -Icomponents/*/src and -Icomponents/*/include:
MFLT_COMPONENTS_ALL_INCLUDE_DIRS := \
  $(wildcard $(MFLT_COMPONENTS_DIR)/*/include) \
  $(PROJECT_DIR)/components/include \
  $(PROJECT_DIR)/ports/include

MFLT_COMPONENTS_ALL_SRC_DIRS := $(wildcard $(MFLT_COMPONENTS_DIR)/*/src)
MFLT_COMPONENTS_ALL_INCLUDE_FLAGS := \
  $(foreach COMPONENT_INCLUDE, $(MFLT_COMPONENTS_ALL_INCLUDE_DIRS) $(MFLT_COMPONENTS_ALL_SRC_DIRS), -I$(COMPONENT_INCLUDE))

export MEMFAULT_EXTRA_INC_PATHS += \
  $(MFLT_COMPONENTS_ALL_INCLUDE_FLAGS)

all: $(TEST_MAKEFILES)

compile: CPPUTEST_BUILD_RULE=start
compile: $(TEST_MAKEFILES)

.PHONY: print-makefiles
print-makefiles:
	@echo $(TEST_MAKEFILES)

# if you are building with alternate compilers, be sure gcov is compatible. you
# may need to do something like this, for example:
# > CC=gcc-8 CXX=g++-8 GCOV_TOOL=gcov-8 make lcov
GCOV_TOOL ?= gcov

# if the compiler is clang or gcc < 9, use lcov. gcc 9+ will use fastcov
CC_VERSION_OUTPUT ="$(shell $(CXX) -v 2>&1)"
CLANG_STR = clang
# check if the compiler is clang
ifeq ($(findstring $(CLANG_STR),$(CC_VERSION_OUTPUT)),$(CLANG_STR))
  USE_LCOV = 1
  # use llvm-cov
  LLVM_COV ?= llvm-cov
  # terminate early if LLVM_COV is not on the path
  ifeq (,$(shell command -v $(LLVM_COV)))
    $(error $(LLVM_COV) not found. Check your llvm install and path for $(LLVM_COV))
  endif
  GCOV_TOOL = $(abspath $(MFLT_TEST_ROOT)/llvm-cov-wrapper.sh)
else
  # if the compiler is gcc < 9, use lcov. gcc 9+ will use fastcov
  GCC_VERSION :=  $(shell $(CC) -dumpversion)
  GCC_MAJOR_VERSION := $(firstword $(subst ., ,$(GCC_VERSION)))
  GCC_VERSION_LT9 = $(shell if [ $(GCC_MAJOR_VERSION) -lt "9" ]; then echo 1; fi)
  ifeq ($(GCC_VERSION_LT9),1)
    USE_LCOV = 1
  endif
endif

# Exclude these directory stems from the coverage results
LCOV_EXCLUDE_DIRS := \
  '*/tests/*' \
  '*/CppUTestExt/*' \
  '/usr/include/*' \

# set additional args for genhtml in environment var, like --dark-mode
GENHTML_EXTRA_ARGS ?=
LCOV_INFO_FILE = $(BUILD_DIR)/lcov.info
lcov: $(TEST_MAKEFILES)
ifeq ($(USE_LCOV),1)
	lcov --gcov-tool $(GCOV_TOOL) --rc lcov_branch_coverage=1 \
		--base-directory . --directory . -c -o $(LCOV_INFO_FILE) \
		$(addprefix --exclude ,$(LCOV_EXCLUDE_DIRS))
else
	fastcov --gcov $(GCOV_TOOL) --lcov -o $(LCOV_INFO_FILE) --branch-coverage \
		--process-gcno --exclude-glob $(LCOV_EXCLUDE_DIRS)
endif
	genhtml --prefix $(PROJECT_DIR) --branch-coverage --highlight --legend \
		-o test_coverage -t "coverage" --num-spaces 4 \
		$(LCOV_INFO_FILE) -o $(BUILD_DIR)/test_coverage/ $(GENHTML_EXTRA_ARGS)
	@echo See coverage output in file://$(abspath $(BUILD_DIR)/test_coverage/index.html)

$(TEST_MAKEFILES):
	$(SILENCE)$(MAKE) -f $@ $(CPPUTEST_BUILD_RULE) COMPONENT_NAME=$(patsubst makefiles/Makefile_%.mk,%,$@)

clean:
	rm -rf $(BUILD_DIR)

.PHONY: all clean $(TEST_MAKEFILES)
