# Example Memfault Zephyr QEMU application

Based on [Zephyr's Example Application](https://github.com/zephyrproject-rtos/example-application), this
provides a minimal reference for Memfault integration.

> Note: this example can also target other boards and should work normally.
> It's primarily tested on the `mps2/an385` (the default board), but also:
>
> - `nrf52840dk_nrf52840`
> - `mimxrt1170_evk/mimxrt1176/cm7`
> - `nucleo_f756zg`
> - `nucleo_l496zg`
>
> Note: You may need to add the board HAL module to the `name-allowlist` in the applications
> `west.yml` (modules limited to reduce `west update` time).

## Usage

After setting up a Zephyr development environment (see [Getting Started](https://docs.zephyrproject.org/latest/getting_started/index.html) to set up), you can run
the following commands to test the application:

```shell
# initialize this project
❯ west init --local qemu-app
❯ west update

# build and run the target program
❯ west build qemu-app
❯ west build --target run

*** Booting Zephyr OS build zephyr-v4.0.0  ***
[00:00:00.000,000] <inf> mflt: GNU Build ID: 4ffb5879ed5923582035133086015bbf65504364
[00:00:00.000,000] <inf> main: 👋 Memfault Demo App! Board mps2/an385

[00:00:00.000,000] <inf> mflt: S/N: DEMOSERIAL
[00:00:00.000,000] <inf> mflt: SW type: zephyr-app
[00:00:00.000,000] <inf> mflt: SW version: 1.0.0+6c108c40f1
[00:00:00.000,000] <inf> mflt: HW version: mps2/an385

uart:~$
```
