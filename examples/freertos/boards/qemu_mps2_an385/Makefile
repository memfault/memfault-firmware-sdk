# Board-specific build/debug properties

FREERTOS_PORT = ARM_CM3
MEMFAULT_ARCH = arm

ARCH_CFLAGS += \
    -mthumb \
    -mcpu=cortex-m3 \

FREERTOS_SRCS += \
    $(FREERTOS_DIR)/portable/GCC/$(FREERTOS_PORT)/port.c \

ATTACH_DEBUGGER = -s -S
RUN_COMMAND = qemu-system-arm -machine mps2-an385 -monitor null -semihosting \
        --semihosting-config enable=on,target=native \
        -kernel $(ELF) \
        -serial stdio -nographic \

DEBUG_COMMAND = $(RUN_COMMAND) $(ATTACH_DEBUGGER)

# Permit overriding the gdb executable
GDB ?= gdb
GDB_COMMAND = \
    $(GDB) --ex 'target extended-remote :1234' $(ELF)
