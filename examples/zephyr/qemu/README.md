# Example Memfault Zephyr QEMU application

Based on https://github.com/zephyrproject-rtos/example-application , this
provides a minimal reference for Memfault integration.

## Usage

After setting up a zephyr development environment
(https://docs.zephyrproject.org/latest/getting_started/index.html), you can run
the following commands to test the application:

```shell
# initialize this project
❯ west init -l qemu-app
❯ west update

# build the target program
# highly recommend '-DCONFIG_QEMU_ICOUNT=n' otherwise the guest runs too fast
❯ west build -b qemu_cortex_m3 --pristine=always zephyr-memfault-example/app -- -DCONFIG_QEMU_ICOUNT=n
❯ west build -t run

*** Booting Zephyr OS build zephyr-v3.1.0  ***
[00:00:00.000,000] <inf> mflt: GNU Build ID: a3f2f5da83bc62ceb0351f88a8b30d5cdab59ae9
[00:00:00.000,000] <inf> main: Memfault Demo App! Board qemu_cortex_m3

[00:00:00.000,000] <inf> mflt: S/N: DEMOSERIAL
[00:00:00.000,000] <inf> mflt: SW type: zephyr-app
[00:00:00.000,000] <inf> mflt: SW version: 1.0.0-dev
[00:00:00.000,000] <inf> mflt: HW version: qemu_cortex_m3
```
