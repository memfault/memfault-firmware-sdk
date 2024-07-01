# Board-specific build/debug properties

FREERTOS_PORT = ARM_CM33_NTZ/non_secure
MEMFAULT_ARCH = arm

# doc for -mcmse
# https://gcc.gnu.org/onlinedocs/gcc-14.1.0/gcc/ARM-Options.html#index-mcmse
ARCH_CFLAGS += \
    -mcpu=cortex-m33 \
    -mthumb \
    -mfloat-abi=hard \
    -mfpu=fpv5-sp-d16 \
    -DconfigENABLE_FPU=1 \
    -DconfigENABLE_MPU=0 \
    -DconfigENABLE_TRUSTZONE=0 \
    -DconfigRUN_FREERTOS_SECURE_ONLY=1 \

FREERTOS_SRCS += \
    $(FREERTOS_DIR)/portable/GCC/$(FREERTOS_PORT)/port.c \
    $(FREERTOS_DIR)/portable/GCC/$(FREERTOS_PORT)/portasm.c \

ATTACH_DEBUGGER = -s -S

# From:
# https://github.com/Introduction-To-System-On-Chip/QEMU_an505/blob/master/Makefile
MACHINE_NAME := mps2-an505

# RUN_COMMAND = \
#     qemu-system-arm \
#     -machine $(MACHINE_NAME) \
#     -cpu cortex-m33 \
#     -m 16M \
#     -monitor null -semihosting \
#     --semihosting-config enable=on,target=native \
#     -kernel $(ELF) \
#     -serial stdio -nographic \

RUN_COMMAND = \
    qemu-system-arm \
    -machine $(MACHINE_NAME) \
    -cpu cortex-m33 \
    -m 16M \
    -monitor none \
    -nographic \
    --semihosting-config enable=on,target=native \
    -d cpu_reset \
    -kernel $(ELF)

DEBUG_COMMAND = $(RUN_COMMAND) $(ATTACH_DEBUGGER)

# Permit overriding the gdb executable
GDB ?= gdb
GDB_COMMAND = \
    $(GDB) --ex 'target extended-remote :1234' $(ELF)
