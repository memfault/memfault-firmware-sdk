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

## Bluetooth SMP DFU session report

This example instruments the Bluetooth SMP (MCUmgr) DFU flow with a Memfault
[session report](https://mflt.io/session-metrics) named `bt_smp`. A new session
is recorded for every firmware image the phone uploads over the air, capturing:

| Metric                 | Meaning                                                       |
| ---------------------- | ------------------------------------------------------------- |
| (session duration)     | Wall-clock time of the upload (recorded automatically)        |
| `bt_smp_bytes_received`| Image payload bytes accepted from the phone                   |
| `bt_smp_image_size`    | Total image size the phone declared in its first request      |
| `bt_smp_status`        | `0` = success, `-1` = aborted, `-2` = disconnected mid-upload |

The implementation lives in
[`src/bt_smp_dfu_session.c`](memfault_demo_app/src/bt_smp_dfu_session.c) and is
self-contained - `main.c` is untouched. It works as follows:

- The DFU lifecycle is observed through the MCUmgr notification callback API
  (`MGMT_EVT_OP_IMG_MGMT_DFU_*`), enabled by `CONFIG_MCUMGR_GRP_IMG_STATUS_HOOKS`
  and `CONFIG_MCUMGR_GRP_IMG_UPLOAD_CHECK_HOOK` in `prj.conf`. The session starts
  on the first upload request, accumulates received bytes per request, and ends
  on `DFU_PENDING`/`DFU_CONFIRMED` (success) or `DFU_STOPPED` (abort). A
  `BT_CONN_CB` also ends the session if the link drops mid-upload.

- Memfault event storage on this port is RAM-backed, so a session report would
  be lost across the reboot into the new image. To get it to the phone first,
  the MCUmgr reset command is hooked (`MGMT_EVT_OP_OS_MGMT_RESET`, enabled by
  `CONFIG_MCUMGR_GRP_OS_RESET_HOOK`). After a successful DFU, that hook blocks
  until the Memfault packetizer drains (up to `BT_SMP_DFU_DRAIN_TIMEOUT_MS`)
  before letting MCUmgr reboot. The hook runs on MCUmgr's dedicated SMP work
  queue, *not* the system work queue that streams MDS data, so blocking it lets
  the Diagnostic Service keep pushing chunks to the phone while we wait. Resets
  that aren't preceded by a successful DFU are not delayed.

  > For the report to actually transfer before the reboot, the phone must have
  > the [Memfault Diagnostic Service (MDS)](https://mflt.io/mds) notifications
  > enabled so the device can stream chunks to it. If nothing is draining the
  > data, the device waits out the timeout and then reboots.

After an OTA update, look for the `bt_smp` session under the device's metrics in
the Memfault UI (or run `mflt metrics_dump` to print session metrics locally).

### Bench testing without a phone

A `bt_smp` shell command group drives the same code paths a real DFU does, so
you can validate metric recording and the drain logic over the serial console:

```bash
uart:~$ bt_smp sim 65536   # simulate a full, successful 64 KiB DFU upload
uart:~$ mflt metrics_dump  # confirm the bt_smp session + metrics were recorded
uart:~$ bt_smp abort       # record a session that ends as aborted
uart:~$ bt_smp drain       # exercise the pre-reboot drain logic (no reboot)
```
