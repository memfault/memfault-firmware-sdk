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
❯ west build -b qemu_cortex_m3 --pristine=always qemu-app
❯ west build -t run

*** Booting Zephyr OS build zephyr-v3.2.0  ***
[00:00:00.000,000] <inf> mflt: GNU Build ID: 4ffb5879ed5923582035133086015bbf65504364
[00:00:00.000,000] <inf> main: 👋 Memfault Demo App! Board qemu_cortex_m3

[00:00:00.000,000] <inf> mflt: S/N: DEMOSERIAL
[00:00:00.000,000] <inf> mflt: SW type: zephyr-app
[00:00:00.000,000] <inf> mflt: SW version: 1.0.0+6c108c40f1
[00:00:00.000,000] <inf> mflt: HW version: qemu_cortex_m3

uart:~$
```
