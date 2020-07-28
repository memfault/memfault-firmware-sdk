# Memfault Demo App

A modified version of the Zephyr "Hello World" extended to include a Memfault
Integration.

## Setup

```bash
$ west init -l memfault_demo_app
$ west update
$ cd zephyr
$ git apply ../modules/memfault-firmware-sdk/ports/nrf-connect-sdk/zephyr/01-v1.3.0-full-esf.patch
```

## Compiling

You can compile for any board supported by the nRF Connect SDK. For example,
targetting the nRF52 PDK would look like:

```bash
$ west build  -b nrf52840dk_nrf52840 memfault_demo_app
...
[181/181] Linking C executable zephyr/zephyr.elf
```

## Testing the Integration

Commands to test the integration are exposed under the `mflt` submenu in the CLI

```
uart:~$ mflt help
mflt - Memfault Test Commands
Subcommands:
  get_core         :gets the core
  clear_core       :clear the core
  crash            :trigger a crash
  export_data      :dump chunks collected by Memfault SDK using
                    https://mflt.io/chunk-data-export
  trace            :Capture an example trace event
  get_device_info  :display device information
```

## Adding the Memfault SDK to your Project

For more details on how to add the memfault-firmware-sdk to your own nRF Connect
SDK based project, follow our step-by-step guide available here:

https://mflt.io/nrf-connect-sdk-integration-guide
