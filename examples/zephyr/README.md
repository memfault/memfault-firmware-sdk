## Memfault for Zephyr RTOS

This folder contains an example integration of the Memfault SDK using the port
provided in `ports/zephyr`

The demo was tested using the
[STM32L4 Discovery kit](https://www.st.com/en/evaluation-tools/b-l475e-iot01a.html).

However, the example application can be compiled for any other ARM Cortex-M
board supported by Zephyr such as the nRF52840 (`--board=nrf52840dk_nrf52840`).

For the purposes of this demo we will be using the v2.5 release of Zephyr.

## Getting Started

Make sure you have read the instructions in the `README.md` in the root of the
SDK and performed the installation steps that are mentioned there.

### Prerequisite

We assume you already have a working Zephyr toolchain installed locally.
Step-by-step instructions can be found in the
[Zephyr Documentation](https://docs.zephyrproject.org/2.5.0/getting_started/index.html#build-hello-world).

### Setup

1. Clone the repo

   ```bash
   $ git clone git@github.com:zephyrproject-rtos/zephyr.git --branch v2.5-branch zephyr
   ```

2. Create a virtual environment

   ```bash
   python3 -m venv .venv
   source .venv/bin/activate
   pip install -r zephyr/scripts/requirements.txt
   ```

3. Initialize a new build environment:

   ```bash
   $ west init -l zephyr/
   $ west update
   ```

4. When using the STM32L4 discovery board, the following patch is also needed
   due to bugs in the stack:

   ```diff
   diff --git a/drivers/wifi/eswifi/Kconfig.eswifi b/drivers/wifi/eswifi/Kconfig.eswifi
   index 6468b98113..5f80c918cd 100644
   --- a/drivers/wifi/eswifi/Kconfig.eswifi
   +++ b/drivers/wifi/eswifi/Kconfig.eswifi
   @@ -9,7 +9,7 @@ menuconfig WIFI_ESWIFI
           select WIFI_OFFLOAD
           select NET_OFFLOAD
           select NET_SOCKETS
   -       select NET_SOCKETS_OFFLOAD
   +       imply NET_SOCKETS_OFFLOAD
           select GPIO

    if WIFI_ESWIFI
   diff --git a/drivers/wifi/eswifi/eswifi_socket.c b/drivers/wifi/eswifi/eswifi_socket.c
   index e31ca0eecd..119f55778d 100644
   --- a/drivers/wifi/eswifi/eswifi_socket.c
   +++ b/drivers/wifi/eswifi/eswifi_socket.c
   @@ -301,6 +301,6 @@ int __eswifi_socket_new(struct eswifi_dev *eswifi, int family, int type,
           k_delayed_work_init(&socket->read_work, eswifi_off_read_work);
           socket->usage = 1;
           LOG_DBG("Socket index %d", socket->index);
   -
   +       net_context_set_state(socket->context, NET_CONTEXT_CONNECTED);
           return socket->index;
    }
   ```

   You can find this patch at
   [`stm32l4_disco_zephyr2.5_wifi.patch`](./stm32l4_disco_zephyr2.5_wifi.patch),
   which can be applied by this command (assuming the `zephyr` repo is in the
   current working directory):

   ```bash
   git -C zephyr apply < stm32l4_disco_zephyr2.5_wifi.patch
   ```

### Memfault Project Key

An API key will need to be baked into the [demo app](https://mflt.io/demo-cli)
to enable it to communicate with Memfault's web services. Go to
https://app.memfault.com/, navigate to the project you want to use and select
'Settings'. Copy the 'Project Key' and paste it into
`$MEMFAULT_SDK/examples/zephyr/apps/memfault_demo_app/src/main.c`, replacing
`<YOUR PROJECT KEY HERE>` with your Project Key.

### Building

```bash
$ cd $MEMFAULT_SDK/examples/zephyr/
$ west build -b disco_l475_iot1 apps/memfault_demo_app
[...]
[271/271] Linking C executable zephyr/zephyr.elf
```

### Flashing

The build can be flashed on the development board using `west flash` ( See
Zephyr
["Building, Flashing, & Debugging" documentation](https://docs.zephyrproject.org/2.5.0/guides/west/build-flash-debug.html?highlight=building%20flashing#flashing-west-flash))

Or, alternatively, if you are using a SEGGER JLink:

1. In one terminal, start a GDB Server

```bash
$ JLinkGDBServer -if swd -device STM32L475VG
```

2. In another terminal, load `zephyr.elf` with GDB:

```bash
arm-none-eabi-gdb-py --eval-command="target remote localhost:2331"  --ex="mon reset" --ex="load"
--ex="mon reset"  --se=build/zephyr/zephyr.elf
```

### Using the App:

At this point, the application should be running and you can open a console:

```bash
$ miniterm.py /dev/cu.usbmodem* 115200 --raw
*** Booting Zephyr OS build zephyr-v2.5.0-56-gec0aa8331a65  ***
<inf> <mflt>: GNU Build ID: c20cef04e29e3ae7784002c3650d48f3a0b7b07d
<inf> <mflt>: S/N: DEMOSERIAL
<inf> <mflt>: SW type: zephyr-app
<inf> <mflt>: SW version: 1.0.0+c20cef
<inf> <mflt>: HW version: disco_l475_iot1
[00:00:00.578,000] <inf> mflt: Memfault Demo App! Board disco_l475_iot1

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
```

### Causing a crash

Command `mflt test hardfault` will trigger a hard fault due to a bad instruction fetch
at a non-existing address, `0xbadcafe`:

```
uart:~$ mflt test hardfault
... etc ...
```

Upon crashing, a coredump will be written to noinit RAM.

To check whether the coredump has been captured, try running the `get_core`
command after the device reboots:

```
uart:~$ mflt get_core
<inf> <mflt>: Has coredump with size: 580
```

This confirms that a coredump of 580 bytes has been captured.

### Posting Memfault Data for Analysis

#### Uploading Symbols

Memfault needs the symbols for the firmware in order to analyze the coredump.
The ELF is located at `build/zephyr/zephyr.elf`. This .elf contains the symbols
(debug information) amongst other things.

[More information on Build Ids and uploading Symbol Files can be found here](https://mflt.io/symbol-file-build-ids).

#### Connect WiFi

Before we can send the coredump, we need to make sure WiFi is connected to the
internet. Type `wifi connect <SSID> <SSID_STRLEN> 0 <PASSWORD>` in the console.
You should see something like:

```
Connection requested
Connected
uart:~$
```

#### Post data

Once connected, type `mflt post_chunks` and press enter. You should see:

```
uart:~$ mflt post_chunks
<dbg> <mflt>: Memfault Message Post Complete: Parse Status 0 HTTP Status 202!
<dbg> <mflt>: Body: Accepted
<dbg> <mflt>: Memfault Message Post Complete: Parse Status 0 HTTP Status 202!
<dbg> <mflt>: Body: Accepted
<dbg> <mflt>: No more data to send
```

A coredump is now being processed by Memfault's services and will show up
shortly under Issues. If it does not, take a look at the FAQ in the `README.md`
in the root of the SDK.

#### Without WiFi

The network stack dependencies can also be disabled completely by using the
`CONFIG_MEMFAULT_HTTP_SUPPORT` Kconfig option.

When using a board that does not have a network stack enabled or for debug
purposes, the messages to push to the Memfault cloud can also be dumped from the
CLI using the `mflt export` command:

```
uart:~$ mflt export
<inf> <mflt>: MC:CAKmAgIDAQpqemVwaHlyLWFwcAlsMS4wLjArYzIwY2VmBm9kaXNjb19sNDc1X2lvdDEEogECBQD8Fw==:
```

You can then copy and paste the output into the "Chunks Debug" view in the
Memfault UI for your project https://app.memfault.com/
