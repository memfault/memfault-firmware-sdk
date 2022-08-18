# nRF-Connect Memfault Example

This is a small example application showing a Memfault integration running on an
nrf52840 development board, using the nRF Connect SDK. Any nRF board should also
work. The example has been tested on:

- nRF52840-DK
- nRF5340-DK

## Usage

Make sure you have the Zephyr / nRF-Connect tools installed first:

<https://developer.nordicsemi.com/nRF_Connect_SDK/doc/2.0.2/nrf/gs_installing.html>

To build and flash this example to an nRF52840-DK (PCA10056), run the following
commands:

```bash
❯ west init --local memfault_demo_app
❯ west update
❯ west build --board nrf52840dk_nrf52840 memfault_demo_app
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
mflt - Memfault Test Commands
Subcommands:
  reboot              :trigger a reboot and record it using memfault
  get_core            :gets the core
  clear_core          :clear the core
  crash               :trigger a crash
  test_log            :Writes test logs to log buffer
  trigger_logs        :Trigger capture of current log buffer contents
  hang                :trigger a hang to test watchdog functionality
  export              :dump chunks collected by Memfault SDK using
                       https://mflt.io/chunk-data-export
  trace               :Capture an example trace event
  get_device_info     :display device information
  post_chunks         :Post Memfault data to cloud
  trigger_heartbeat   :Trigger an immediate capture of all heartbeat metrics
  get_latest_release  :checks to see if new ota payload is available
```

For example, to test the coredump functionality:

1. run `mflt crash` and wait for the board to reset
2. run `mflt get_core` to confirm the coredump was saved
3. run `mflt export` to print out the base-64 chunks:

   ```plaintext
   uart:~$ mflt export
   <inf> <mflt>: MC:SE4DpwIEAwEKbW5yZjUyX2V4YW1wbGUJZTAuMC4xBmFhC0Z5RE1gF8EEhgFpSW5mbyBsb2chAmxXYXJuaW5nIGxvZyEDakVycm9yIGxvZyE=:
   <inf> <mflt>: MC:gE6A/A==:
   ```

4. upload the chunks to Memfault. see here for details:

   <https://mflt.io/chunks-debug>
