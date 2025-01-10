# nRF-Connect Memfault Example

This is a small example application showing a Memfault integration running on an
nrf52840 development board, using the nRF Connect SDK. Any nRF board should also
work. The example has been tested on:

- nRF52840-DK (`nrf52840dk/nrf52840`)
- nRF5340-DK (`nrf5340dk/nrf5340/cpuapp`)
- nRF54L15-DK (`nrf54l15dk/nrf54l15/cpuapp`)

## Usage

Make sure you have the Zephyr / nRF-Connect tools installed first:

<https://docs.nordicsemi.com/bundle/ncs-2.9.0/page/nrf/installation/install_ncs.html>

This application is tested on nRF Connect SDK v2.9.0.

To build and flash this example to an nRF5340-DK (PCA10095), run the following
commands:

```bash
❯ west init --local memfault_demo_app
❯ west update
❯ west build --board nrf5340dk/nrf5340/cpuapp memfault_demo_app
❯ west flash
```

Open a serial terminal to access the console:

```bash
# for example, pypserial-miniterm
❯ pyserial-miniterm --raw /dev/ttyACM0 115200
```

The console has several Memfault test commands available:

```bash
uart:~$ mflt help
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
  post_chunks         :Post Memfault data to cloud
  get_latest_release  :checks to see if new ota payload is available
```

The `mflt test` subgroup contains commands for testing Memfault functionality:

```bash
uart:~$ mflt test help
test - commands to verify memfault data collection
       (https://mflt.io/mcu-test-commands)
Subcommands:
  assert       :trigger memfault assert
  busfault     :trigger a busfault
  hang         :trigger a hang
  hardfault    :trigger a hardfault
  memmanage    :trigger a memory management fault
  usagefault   :trigger a usage fault
  zassert      :trigger a zephyr assert
  reboot       :trigger a reboot and record it using memfault
  heartbeat    :trigger an immediate capture of all heartbeat metrics
  log_capture  :trigger capture of current log buffer contents
  logs         :writes test logs to log buffer
  trace        :capture an example trace event
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
