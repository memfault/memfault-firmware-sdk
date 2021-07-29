# Memfault Demo App

A modified version of the the samples/nrf9160/https_client in the nRF Connect
SDK which includes a Memfault Integration!

## Setup

An API key will need to be baked into the demo app to enable it to communicate
with Memfault's web services. Go to https://app.memfault.com/, navigate to the
project you want to use and select 'Settings'. Copy the 'Project Key' and paste
it into [`src/main.c`](src/main.c), replacing `<YOUR PROJECT KEY HERE>` with
your Project Key.

## Compiling

You can compile for any board supported by the nRF Connect SDK. For example,
targetting the nRF52 PDK would look like:

```bash
$ west init -l memfault_demo_app
$ west update
$ west build -b nrf9160dk_nrf9160_ns memfault_demo_app
...
[181/181] Linking C executable zephyr/zephyr.elf
```

Note that you will need to use `nrf9160dk_nrf9160ns` instead on
versions of NCS based on Zephyr 2.6 and earlier.

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
  post_chunks      :Post Memfault data to cloud
```

## Adding the Memfault SDK to your Project

For more details on how to add the memfault-firmware-sdk to your own nRF Connect
SDK based project, follow our step-by-step guide available here:

https://mflt.io/nrf-connect-sdk-integration-guide
