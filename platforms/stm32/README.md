# Memfault for STM32 Devices

The subdirectories in this folder contain example integrations of the memfault
coredump feature for STM32 MCUs.

The example integrations are set up to store coredumps to regions of internal
microflash. They make use of the STM32Cube HAL for the flash interface to the
coredump (`stm32*\_hal_flash.c` & `stm32*\_hal_flash_ex.c`) which can be
downloaded for free on the STM32 website.

Within each subdirectory, more detailed descriptions can be found about how to
integrate memfault into the target MCU
