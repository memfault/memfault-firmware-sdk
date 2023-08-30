# Memfault Demo App

A modified version of the the samples/nrf9160/https_client in the nRF Connect
SDK which includes a Memfault Integration!

## Setup

An API key will need to be baked into the demo app to enable it to communicate
with Memfault's web services. Provision a project and key from
https://goto.memfault.com/create-key/nrf91

## Compiling

You can compile for any board supported by the nRF Connect SDK. For example,
targeting the nRF91 PDK would look like:

```bash
$ west init -l memfault_demo_app
$ west update
# Replace ${YOUR_PROJECT_KEY} with the Project Key from https://mflt.io/project-key
$ west build -b nrf9160dk_nrf9160_ns memfault_demo_app -- -DCONFIG_MEMFAULT_NCS_PROJECT_KEY=\"${YOUR_PROJECT_KEY}\"
...
[181/181] Linking C executable zephyr/zephyr.elf
```

Note that the board argument (`-b`) for the nRF9160DK has changed across releases -- if you are targeting nRF Connect SDK <= 1.6, use, `-b nrf9160_pca10090ns`. Also, you may want to target a version of the DK by passing `-b <board>@<revision>`, for example `nrf9160dk_nrf9160@1.0.0`, in order to use [specific hardware](https://developer.nordicsemi.com/nRF_Connect_SDK/doc/latest/zephyr/boards/arm/nrf9160dk_nrf9160/doc/index.html#additional-hardware-in-v0-14-0).

## Testing the Integration

Commands to test the integration are exposed under the `mflt` submenu in the CLI

```
uart:~$ mflt help
mflt - Memfault Test Commands
Subcommands:
  clear_core          :clear coredump collected
  export              :dump chunks collected by Memfault SDK using
                       https://mflt.io/chunk-data-export
  get_core            :check if coredump is stored and present
  get_device_info     :display device information
  get_latest_release  :checks to see if new ota payload is available
  post_chunks         :Post Memfault data to cloud
  test                :commands to verify memfault data collection
                       (https://mflt.io/mcu-test-commands)
```

## Adding the Memfault SDK to your Project

For more details on how to add the memfault-firmware-sdk to your own nRF Connect
SDK based project, follow our step-by-step guide available here:

https://mflt.io/nrf-connect-sdk-integration-guide
