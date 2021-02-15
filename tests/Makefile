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

LCOV_INFO_FILE = $(BUILD_DIR)/lcov.info
lcov: $(TEST_MAKEFILES)
	lcov --base-directory . --directory . -c -o $(LCOV_INFO_FILE) --exclude "*cpputest/*" --exclude "*tests/*"
	genhtml -o test_coverage -t "coverage" --num-spaces 4 $(LCOV_INFO_FILE) -o $(BUILD_DIR)/test_coverage/

$(TEST_MAKEFILES):
	$(MAKE) -f $@ $(CPPUTEST_BUILD_RULE)

clean:
	rm -rf $(BUILD_DIR)

.PHONY: all clean $(TEST_MAKEFILES)
