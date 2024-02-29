## Memfault for Quantum Leaps' QP™

This folder contains an example integration of the Memfault SDK using the QP/C
port provided in `ports/qp`.

The demo was tested using the
[STM32F407 discovery board](https://www.st.com/en/evaluation-tools/stm32f4discovery.html)
but should work for any STM32F4xx based board.

## Getting Started

Make sure you have read the instructions in the `README.md` in the root of the
SDK and performed the installation steps that are mentioned there.

### Setup

_Note: these instructions reference the `$MEMFAULT_SDK_ROOT` variable for
various paths. Either substitute the path to the Memfault SDK, or set it before
running the snippets, i.e.
`export MEMFAULT_SDK_ROOT=absolute/path/to/memfault/sdk`._

1. Go into the demo app directory:

   ```bash
   $ cd $MEMFAULT_SDK_ROOT/examples/qp/apps/memfault_demo_app
   ```

2. Clone the QP/C repo

   ```bash
   $ git clone git@github.com:QuantumLeaps/qpc.git --branch v6.6.0+ qpc
   ```

3. Apply patches to integrate Memfault SDK

   ```bash
   $ cd qpc
   $ patch include/qassert.h $MEMFAULT_SDK_ROOT/ports/qp/qassert.h.patch
   $ patch src/qf_pkg.h $MEMFAULT_SDK_ROOT/ports/qp/qf_pkg.h.patch
   ```

### Memfault Project Key

A Project Key will need to be baked into the
demo app to enable it to communicate with Memfault's
web services. Go to https://app.memfault.com/, navigate to the project you want
to use and select 'Settings'. Copy the 'Project API Key' and paste it into
`$MEMFAULT_SDK_ROOT/examples/qp/apps/memfault_demo_app/src/main.c`, replacing
`<YOUR PROJECT KEY HERE>` with your Project Key.

### Building the App

After performing the steps above, run `make` from the demo app directory:

```bash
$ cd $MEMFAULT_SDK_ROOT/examples/qp/apps/memfault_demo_app

$ EMBEDDED_MFLT_SDK_ROOT=$MEMFAULT_SDK_ROOT QPC_DIR=qpc make
```

The target is built to:
`$MEMFAULT_SDK_ROOT/examples/qp/apps/memfault_demo_app/build/memfault_demo_app.elf`

### Flashing the App

Run the `st-util` in one terminal:

```
$ st-util
st-util 1.5.1
2019-12-12T15:22:42 INFO common.c: Loading device parameters....
2019-12-12T15:22:42 INFO common.c: Device connected is: F4 device, id 0x10076413
[...]
```

Then flash the .elf using GDB:

```
$ arm-none-eabi-gdb --eval-command="target remote localhost:4242" --ex="mon reset" --ex="load" --ex="mon reset" \
  --se=$MEMFAULT_SDK_ROOT/examples/qp/apps/memfault_demo_app/build/memfault_demo_app.elf --batch
Generating /path/to/memfault-sdk/examples/qp/apps/memfault_demo_app/build/memfault_demo_app.elf
```

### Running the App

The USART2 peripheral of the board is used as a console/shell.

Follow the instructions in the
[user manual](https://www.st.com/content/ccc/resource/technical/document/user_manual/70/fe/4a/3f/e7/e1/4f/7d/DM00039084.pdf/files/DM00039084.pdf/jcr:content/translations/en.DM00039084.pdf)
(see section 6.1.3 "ST-LINK/V2-A VCP configuration") to connect the RX/TX to
either the on-board or an external USB-to-serial adapter.

At this point, the application should be running and you can open a console
to run the [Memfault demo CLI](https://mflt.io/demo-cli). To get started, run `help` to
see short descriptions of each command.

```
$ miniterm.py --raw /dev/cu.usbserial* 115200
--- Miniterm on /dev/cu.usbserial-1420  115200,8,N,1 ---
--- Quit: Ctrl+] | Menu: Ctrl+T | Help: Ctrl+T followed by Ctrl+H ---
Memfault QP demo app started...
mflt> help
get_core: Get coredump info
clear_core: Clear an existing coredump
export: Export base64-encoded chunks. To upload data see https://mflt.io/chunk-data-export
get_device_info: Get device info
help: Lists all commands
```

### Causing a crash

Detach the debugger now and hard-reset the board. Otherwise, if the debugger is
still attached while crashing, the demo application will pause at a breakpoint
instruction.

Command `test_hardfault` will trigger a hard fault due to a bad instruction fetch at a
non-existing address, `0xbadcafe`:

```
mflt> test_hardfault
Memfault QP demo app started...

mflt>
```

Upon crashing, a coredump will be written to noinit RAM. After that the demo app
will restart immediately.

To check whether the coredump has been captured, try running the `get_core`
command after the device reboots:

```
mflt> get_core
Has coredump with size: 584
```

This confirms that a coredump of 584 bytes has been captured.

### Posting Memfault Data for Analysis

#### Uploading Symbols

Memfault needs the symbols for the firmware in order to analyze the coredump.
The ELF is located at `build/memfault_demo_app.elf`. This .elf contains the
symbols (debug information) amongst other things.

[More information on Build Ids and uploading Symbol Files can be found here](https://mflt.io/symbol-file-build-ids).

#### Posting captured data to Memfault

The STM32F407 board does not have the capability to connect to the internet
directly. Therefore, for debug purposes, the messages to push to the Memfault
cloud can also be dumped from the CLI using the `export` command:

```
mflt> export
MC:SLMTgQlDT1JFAgYAA/QNFAABTAYAGwGAAADQHwAgSHEAIOnhDAABAQYAIbYlAQDgHwAgYCAAIAD2AAACDgAbtiUBANAfACCldAAApHQKABM=:
MC:wE0hsEgAINAfACAMDgABFAYAKYGX7c6KNvuWbdNF6vmTpvwqJEu6Ag4AAQoGABVERU1PU0VSSUFMCg4AARAGACExLjAuMCs4Y2VkMjQ2YjY=:
MC:wJsBZgsOAAEKBgAVemVwaHlyLWFwcAQOAAEOBgAdcWVtdV9jb3J0ZXhfbTMHDgABBAYAASgGAAEFDgABBAYACQGAAAAGDgABAgoAAQEGAAk=:
MC:wOgBJO0A4BwGAAUIAAcGAAEBKgABAQYACRjtAOAMGgAF4CABBgAJBOAA4BAGAAEBFgABBwYAAQEGAAkE7QDgCAYAAwYIDAABAQYACfztAOA=:
MC:wLUCBA4AAQEIAAfhAOAEBgABYAYAAQEIAAfiAOAEDgABAQgAB+MA4AQOAAEBCAAH5ADgIAYACiAEgDIgAQEGAAlUDAAgKAYABgEKAAELBgA=:
MC:wIIDC/MBAADYAQYAJwIAAHgXACB4FwAgAAIAAD3mAAABBgAPeBcAIAACAAAGIElUaW1lclRhc2tGcmVlU3RhY2s6IDMyMjQBIjxpbmY+IG0=:
MC:wM8DZmx0OgYgQUhlYXBfQnl0ZXNGcmVlOiA0MDIwASo8aW5mPiBtZmx0OgYgmQFNYWluU3RhY2tfTWluQnl0ZXNGcmVlOiAzMjI0ASI8aW4=:
MC:wJwEZj4gbWZsdDogSGVhcnRiZWF0IGtleXMvdmFsdWVzOgEtPGluZj4gbWZsdDoGIFdNZW1mYXVsdFNka01ldHJpY19JbnRlcnZhbE1zOiA=:
MC:wOkEMAE7PGluZj4gbWZsdDoGIHNNZW1mYXVsdFNka01ldHJpY19VbmV4cGVjdGVkUmVib290Q291bnQ6IG51bGwBJTxpbmY+IG1mbHQ6BiA=:
...
```

The command will print out a sequence of base64-encoded chunks like above.
You can copy & paste this output into the ["Chunks Debug" view](https://mflt.io/chunks-debug) in the Memfault UI
or upload using the [desktop CLI tool](https://mflt.io/chunk-data-export).

For more details on how to use
to CLI to explore each of Memfault's subsystems, see the [Memfault docs](https://mflt.io/demo-cli).
