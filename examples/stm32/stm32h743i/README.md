# Memfault for the STM32H7xx Family

`memfault_sdk.mk` includes an example makefile that can be included into a build
system to integrate the reference port into a stm32h7 device. It expects two
variables to be defined prior to including the makefile

- `EMBEDDED_MFLT_SDK_ROOT` - The location for the root directory of this
  repository
- `STM32_CUBE_DIR` - The location of the root directory for the STM32 Cube HAL.
  This demo was tested with the V1.5.0 STM32H7CUBE HAL available
  [here](https://www.st.com/content/st_com/en/products/embedded-software/mcu-mpu-embedded-software/stm32-embedded-software/stm32cube-mcu-mpu-packages/stm32cubeh7.html#overview)

## Example Steps for Adding port to ChibiOS Demo

### Setup

Download the following items:

- `git clone https://github.com/ChibiOS/ChibiOS.git`
- `cd ChibiOS/demos/STM32/RT-STM32H743I-NUCLEO144` and
  - Move the downloaded contents of `STM32Cube_FW_H7_V1.5.0` (link above) to
    stm32_cube
  - clone the memfault firmware sdk:
    `git clone https://github.com/memfault/memfault-firmware-sdk.git`

Integrate Memfault into the ChibiOS build system. This can be achieved by
running:
`git apply <path_to_memfault_sdk>/examples/stm32/stm32h743i/chibios-memfault-integration.patch`

The patch does three things:

- includes `memfault_sdk.mk` into the ChibiOS build system
- defines a new rule for building the files needed for memfault (sdk, platform
  port, and flash stm32 hal files)
- forces an assert in the main loop within main.c for simple testing

## Testing

At this point you should be able to run `make` and compile the code & then load
the resultant elf at `build/ch.elf` onto your board.

For example, I'm connected to the nucleo board with JLink & GDB:

`JLinkGDBServer -if swd -device STM32H753ZI -speed auto -port 2331`

And run the following to flash the board:

`arm-none-eabi-gdb-py --eval-command="target remote localhost:2331" --ex="mon reset" --ex="load" --ex="mon reset" --se=build/ch.elf`

Soon after boot, you should hit the assert in the main loop and a breakpoint:

```
memfault_platform_halt_if_debugging () at ./memfault-firmware-sdk/examples/stm32/stm32h743i/platform_reference_impl/memfault_platform_core.c:17
17      __asm("bkpt");
(gdb) bt
#0  memfault_platform_halt_if_debugging () at ./memfault-firmware-sdk/examples/stm32/stm32h743i/platform_reference_impl/memfault_platform_core.c:17
#1  0x08009af2 in memfault_fault_handling_assert (pc=<optimized out>, lr=lr@entry=0x800034d <endinitloop+4>, extra=extra@entry=0) at ./memfault-firmware-sdk/components/panics/src/memfault_fault_handling_arm.c:162
#2  0x0800967e in main () at main.c:76
```

You'll want to step over the breakpoint and then continue. At this point the
coredump saving logic will run and the next breakpoint hit will be right before
reboot

```
(gdb) next
memfault_fault_handling_assert (pc=<optimized out>, lr=lr@entry=0x800034d <endinitloop+4>, extra=extra@entry=0) at ./memfault-firmware-sdk/components/panics/src/memfault_fault_handling_arm.c:174
174   *icsr |= nmipendset_mask;
(gdb) continue
Continuing.

Program received signal SIGTRAP, Trace/breakpoint trap.
memfault_platform_halt_if_debugging () at ./memfault-firmware-sdk/examples/stm32/stm32h743i/platform_reference_impl/memfault_platform_core.c:17
17      __asm("bkpt");
(gdb) bt
#0  memfault_platform_halt_if_debugging () at ./memfault-firmware-sdk/examples/stm32/stm32h743i/platform_reference_impl/memfault_platform_core.c:17
#1  memfault_platform_reboot () at ./memfault-firmware-sdk/examples/stm32/stm32h743i/platform_reference_impl/memfault_platform_core.c:22
#2  0x08009a24 in memfault_fault_handler (regs=<optimized out>, reason=<optimized out>) at ./memfault-firmware-sdk/components/panics/src/memfault_fault_handling_arm.c:124
#3  <signal handler called>
#4  0x00000000 in ?? ()
Backtrace stopped: previous frame identical to this frame (corrupt stack?)
```

Now you can extract the coredump via gdb. The example port is configured to save
coredumps in the last sector of flash and can be extracted from GDB using:

`dump binary memory memfault-core.bin 0x81E0000 0x8200000`

You can then navigate to the Issues page on the memfault dashboard and click on
"Manual Upload" to upload the file collected.
