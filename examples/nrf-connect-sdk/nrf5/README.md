# nRF-Connect Memfault Example

This is an example application showing a Memfault integration running on a
Nordic Semiconductor development board, using the nRF Connect SDK. Any nRF board
supporting `CONFIG_BT` should work. The example has been tested on:

- nRF52840-DK (`nrf52840dk/nrf52840`)
- nRF5340-DK (`nrf5340dk/nrf5340/cpuapp`)
- nRF54L15-DK (`nrf54l15dk/nrf54l15/cpuapp`)
- nRF54LM20-DK (`nrf54lm20dk/nrf54lm20a/cpuapp`)

You can follow a guided tutorial for this example here:

<https://mflt.io/quickstart-nrf52-53-54>

## Usage

Make sure you have the Zephyr / nRF-Connect tools installed first:

<https://docs.nordicsemi.com/bundle/ncs-3.1.1/page/nrf/installation/install_ncs.html>

This application is tested on nRF Connect SDK v3.1.1.

To build and flash this example to an nRF5340-DK (PCA10095), run the following
commands:

```bash
❯ west init --local memfault_demo_app
❯ west update
❯ west build --sysbuild --pristine=always --board nrf5340dk/nrf5340/cpuapp memfault_demo_app -- \
  -DCONFIG_MEMFAULT_NCS_PROJECT_KEY=\"YOUR_PROJECT_KEY\"
❯ west flash
```

Open a serial terminal to access the console:

```bash
# for example, pypserial-miniterm
❯ pyserial-miniterm --raw /dev/ttyACM0 115200
```

The console has several Memfault test commands available:

```bash
mflt - Memfault Test Commands
Subcommands:
  clear_core          : clear coredump collected
  export              : dump chunks collected by Memfault SDK using
                        https://mflt.io/chunk-data-export
  get_core            : check if coredump is stored and present
  get_device_info     : display device information
  get_latest_url      : gets latest release URL
  get_latest_release  : performs an OTA update using Memfault client
  coredump_size       : print coredump computed size and storage capacity
  metrics_dump        : dump current heartbeat or session metrics
  test                : commands to verify memfault data collection
                        (https://mflt.io/mcu-test-commands)

```

The `mflt test` subgroup contains commands for testing Memfault functionality:

```bash
uart:~$ mflt test help
test - commands to verify memfault data collection
       (https://mflt.io/mcu-test-commands)
Subcommands:
  busfault        : trigger a busfault
  hardfault       : trigger a hardfault
  memmanage       : trigger a memory management fault
  usagefault      : trigger a usage fault
  hang            : trigger a hang
  zassert         : trigger a zephyr assert
  stack_overflow  : trigger a stack overflow
  assert          : trigger memfault assert
  loadaddr        : test a 32 bit load from an address
  double_free     : trigger a double free error
  badptr          : trigger fault via store to a bad address
  isr_badptr      : trigger fault via store to a bad address from an ISR
  isr_hang        : trigger a hang in an ISR
  reboot          : trigger a reboot and record it using memfault
  heartbeat       : trigger an immediate capture of all heartbeat metrics
  log_capture     : trigger capture of current log buffer contents
  logs            : writes test logs to log buffer
  trace           : capture an example trace event
```

For example, to test the coredump functionality:

1. run `mflt test hardfault` and wait for the board to reset
2. run `mflt get_core` to confirm the coredump was saved
3. run `mflt export` to print out the base-64 chunks:

   ```plaintext
   uart:~$ mflt export
   <inf> <mflt>: MC:SE4DpwIEAwEKbW5yZjUyX2V4YW1wbGUJZTAuMC4xBmFhC0Z5RE1gF8EEhgFpSW5mbyBsb2chAmxXYXJuaW5nIGxvZyEDakVycm9yIGxvZyE=:
   <inf> <mflt>: MC:gE6A/A==:
   ```

4. upload the chunks to Memfault. see here for details:

   <https://mflt.io/chunks-debug>
