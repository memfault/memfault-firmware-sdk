# Error on any uset variable
MAKEFLAGS=--warn-undefined-variables

# Default to a silent build. Run make with --trace for a verbose build.
.SILENT:

BOARD ?= qemu_mps2_an385
BOARD_DIR := boards/$(BOARD)

BUILD_DIR := build/$(BOARD)

ELF = $(BUILD_DIR)/main.elf

ARM_CC ?= arm-none-eabi-gcc
# if cc isn't set by the user, set it to ARM_CC
ifeq ($(origin CC),default)
CC := $(ARM_CC)
endif

# Check if FreeRTOS location set, add rule to download if not
ifeq ($(FREERTOS_DIR),)
FREERTOS_DIR = FreeRTOS-Kernel
FREERTOS_VER = V10.4.6

$(FREERTOS_DIR)/.clonedone:
	$(info FREERTOS_DIR does not exist, downloading local copy)
	git -c advice.detachedHead=false clone --branch $(FREERTOS_VER) --depth 1 git@github.com:FreeRTOS/FreeRTOS-Kernel.git $(FREERTOS_DIR)
	# sad check for clone completion, :(
	touch $@

.DEFAULT_GOAL :=
endif

include $(BOARD_DIR)/Makefile

# Gather list of FreeRTOS sources
FREERTOS_SRCS = \
	$(FREERTOS_DIR)/tasks.c \
	$(FREERTOS_DIR)/queue.c \
	$(FREERTOS_DIR)/list.c \
	$(FREERTOS_DIR)/timers.c \
	$(FREERTOS_DIR)/portable/GCC/$(FREERTOS_PORT)/port.c \
	$(FREERTOS_DIR)/portable/MemMang/heap_4.c \

# Add application sources
SRCS += \
	main.c \
	$(BOARD_DIR)/startup.c \
	console.c \
	$(BOARD_DIR)/memfault_platform_impl.c \
	memfault/memfault_platform_port.c \
	$(FREERTOS_SRCS) \

# Use Memfault SDK worker to gather initial Memfault SDK sources and include dirs
MEMFAULT_SDK_ROOT := ../..
MEMFAULT_COMPONENTS := core util panics metrics demo
include $(MEMFAULT_SDK_ROOT)/makefiles/MemfaultWorker.mk

# Add additional SDK sources to project for FreeRTOS and RAM-backed coredump
SRCS += \
	$(MEMFAULT_COMPONENTS_SRCS) \
	$(MEMFAULT_SDK_ROOT)/ports/freertos/src/memfault_core_freertos.c \
	$(MEMFAULT_SDK_ROOT)/ports/freertos/src/memfault_freertos_ram_regions.c \
	$(MEMFAULT_SDK_ROOT)/ports/freertos/src/memfault_metrics_freertos.c \
	$(MEMFAULT_SDK_ROOT)/ports/freertos/src/memfault_panics_freertos.c \
	$(MEMFAULT_SDK_ROOT)/ports/panics/src/memfault_platform_ram_backed_coredump.c \

# Fixup build path for objects of the Memfault SDK, all build output kept within build/
OBJS := $(subst $(MEMFAULT_SDK_ROOT),memfault-firmware-sdk,$(SRCS:%.c=$(BUILD_DIR)/%.o))

INCLUDE_PATHS += \
  -I$(FREERTOS_DIR)/include \
  -I. \
  -I./memfault \
  -I$(MEMFAULT_COMPONENTS_INC_FOLDERS) \
  -I$(MEMFAULT_SDK_ROOT)/ports/include \
  -I$(FREERTOS_DIR)/portable/GCC/$(FREERTOS_PORT) \

# generic (non-arch-specific) CFLAGS
CFLAGS += \
	-nostartfiles \
	-mthumb \
	-mcpu=cortex-m3 \
	-Werror \
	-Wall \
	-Wextra \
	-ggdb3 \
	-Og \
	-MD \

LDFLAGS += \
	-Wl,-T$(BOARD_DIR)/linker.ld \
	-Wl,--gc-sections \
	--specs=nano.specs \
	--specs=rdimon.specs \
	-Wl,-lc \
	-Wl,-lrdimon \
	-Wl,-Map=$(BUILD_DIR)/main.map \
	-Wl,--build-id \

.PHONY: all
all: $(ELF)

# Require clone to complete as prereq for sources
$(SRCS): $(FREERTOS_DIR)/.clonedone

# Add rules for patched build objects from SDK
$(BUILD_DIR)/memfault-firmware-sdk/%.o: $(MEMFAULT_SDK_ROOT)/%.c
	mkdir -p $(dir $@)
	$(info Compiling $<)
	$(CC) $(CFLAGS) $(INCLUDE_PATHS) -c $< -o $@

$(BUILD_DIR)/%.o: %.c
	mkdir -p $(dir $@)
	$(info Compiling $<)
	$(CC) $(CFLAGS) $(INCLUDE_PATHS) -c $< -o $@

$(ELF): $(OBJS)
	$(info Linking $@)
	$(CC) $(CFLAGS) $(LDFLAGS) $(OBJS) -o $@

-include $(OBJS:.o=.d)

.PHONY: debug
debug: $(ELF)
	$(info Starting debugger)
	$(DEBUG_COMMAND)

.PHONY: gdb
gdb: $(ELF)
	$(GDB_COMMAND)

.PHONY: run
run: $(ELF)
	$(RUN_COMMAND)

.PHONY: clean
clean:
	rm -rf build
