# Memfault SDK for Arm Mbed OS 5

This folder contains support files to demonstrate how Memfault could be
integrated into an Arm Mbed OS 5 project.

The demo app is tested on the
[STM32F429I-DISC1](https://www.st.com/en/evaluation-tools/32f429idiscovery.html)
evaluation board. The instructions below assume this development board is being
used. However, the port for any board using Mbed should look very similar.

The demo app is tested on Mbed OS 5.14 with RTOS enabled. It should integrate in
a similar manner with other 5.x versions as well. Mbed OS 2 is not supported.

## Getting Started

Make sure you have read the instructions in the `README.md` in the root of the
SDK and performed the installation steps that are mentioned there.

You must have the
[GCC ARM](https://developer.arm.com/tools-and-software/open-source-software/developer-tools/gnu-toolchain/gnu-rm/downloads)
toolchain and the
[Mbed command line tools](https://os.mbed.com/docs/mbed-os/v5.15/tools/developing-mbed-cli.html)
installed before you begin.

### Downloading the demo app dependencies

We use [Invoke](http://www.pyinvoke.org/) to make it easier and more repeatable
to build and run the SDK demos. The first step is to download Mbed OS 5 and the
other dependencies of the demo app. Run this Invoke command from the root of the
SDK to do it:

```
$ invoke mbed.update
```

Note:

- The update process may emit Mbed warnings saying that the demo app is not
  under source control. Mbed has opinions about how applications should be
  structured and one of them is that an application should be at the root of a
  source control repository. For this demo, those warnings can be ignored.

- All of the Invoke (`invoke`) commands are wrappers around the usual Mbed
  command line workflow (`mbed` commands). You can use `invoke --echo` to show
  the `mbed` commands that are being executed. The `mbed` commands must be run
  from the demo app base directory
  (`$SDK/examples/mbed/apps/memfault_demo_app`).

### Updating the demo app with your Project Key

A Project Key will need to be compiled into the demo app to enable it to
communicate with the Memfault cloud.

To get a Project Key:

1.  Go to https://app.memfault.com/
1.  Navigate to (or create) the project you want to use.
1.  Select "Settings".
1.  Find the "Project Key" on the page and copy it.

Open the file `$SDK/examples/mbed/apps/memfault_demo_app/mbed_app.json` and
replace the text `<YOUR PROJECT KEY HERE>` with your Project Key.

### Building and flashing the demo app

You should now be able to compile the [demo app](https://mflt.io/demo-cli) with
the Memfault components included and run it!

Connect the target, then run this command to build and flash:

```
$ invoke mbed.build --flash
```

Note:

- The STM32F429I-DISC1 evaluation board has two USB ports: a micro-USB (near the
  LCD) and a mini-USB (away from the LCD). The mini-USB is the one for the
  built-in debugger used to flash.

- The build output will show the full path to the ELF file that was built. Keep
  this handy because you'll need to upload the ELF to Memfault later on.

### Using the demo app

The demo app is a command interpreter. With the demo app flashed onto the
target, you can open a serial console interact with it:

```
$ invoke mbed.console
```

Press enter and you should see the `mflt>` prompt. Type `help` and press enter
to see a list of commands.

The demo app can cause the target to crash in several ways. Upon crashing, the
`memfault/panics` component of the Memfault SDK will capture a coredump and save
it to the internal device flash. For the purposes of this demo, once a coredump
is captured, it can be dumped out over the console and posted to the Memfault
cloud.

Let's walk through the coredump process step by step:

### Check the device info

As a sanity check, let's request the device info from the debug console, enter
`get_device_info` and press enter:

```
mflt> get_device_info
MFLT: [INFO] S/N: 1E00230001234567890ABCDE
MFLT: [INFO] SW type: mbed-main
MFLT: [INFO] SW version: 1.0.0
MFLT: [INFO] HW version: mbed-proto
mflt>
```

In this reference implementation, the hardware version is hard-coded to
`mbed-proto`, software type is hard-coded to `mbed-main` and the STM32 unique ID
is used as serial number. You can change this to match your requirements (see
`libraries/memfault/platform_reference_impl/memfault_platform_device_info.c`).

### Causing a crash

Command `test_hardfault` will trigger a hard fault due to a bad instruction fetch at a
non-existing address, `0xbadcafe`.

```
mflt> test_hardfault
... etc ...
```

Upon crashing, the Memfault SDK component running from the ARM fault handler
will write coredump to internal flash memory. This can take a few seconds. After
the coredump is recorded, the Memfault SDK component will reboot the system.

When the system comes back up, you'll see the startup banner. You can then use
the `get_core` command to check if a coredump was stored:

```
MFLT: [INFO] Memfault Mbed OS 5 demo app started...

mflt> get_core
MFLT: [INFO] Has coredump with size: 21632
```

This confirms that a coredump of 21632 bytes (the entire space allocated for the
stack) has been captured to internal flash.

### Posting the coredump to Memfault for analysis

#### Uploading symbols

Memfault needs the symbols for the firmware in order to analyze the coredump.
The full path to the ELF file is displayed when you build with
`invoke mbed.build`.

This ELF file contains the symbols (debug information) amongst other things.

[More information on Build Ids and uploading Symbol Files can be found here](https://mflt.io/symbol-file-build-ids).

#### Posting the coredump

In a production environment, the data could be stored and transferred using any
of the facilities offered by Mbed OS. For example, the filesystem drivers could
be used to store the coredump and the HTTP client and wifi drivers used to post
it directly to Memfault over wifi. If you are interested in an implementation,
please don't hesitate to reach out!

For the purposes of this demo, we will just grab the core information from the
CLI. Enter the `print_chunk` command:

```
mflt> print_chunk

echo \
434f5245010000004881030000000000000000004400000001000000025b002000000000feca\
[...]
| xxd -p -r | curl -X POST https://ingress.memfault.com/api/v0/upload/coredump\
 -H 'Memfault-Project-Key:<YOUR PROJECT KEY HERE>'\
 -H 'Content-Type:application/octet-stream' --data-binary @- -
```

The coredump should look like the above. Assuming you are still interacting with
the CLI through `invoke mbed.console`, the Invoke wrapper should detect that a
core was printed and offer to post it for you:

```
Invoke CLI wrapper detected 'print_chunk' call
Would you like to run the command displayed above? [y/n]
```

Otherwise, you can copy & paste this output to a terminal to send the coredump
to the Memfault cloud. The coredump will be processed by Memfault and then show
up shortly under Issues. If it does not, take a look at the FAQ in the
`README.md` in the root of the SDK.

After printing a coredump with `print_chunk`, it will be automatically cleared
from the flash storage.

Note:

- A new coredump will only be saved to the flash storage if it is not already
  occupied. If you do not use `print_chunk`, you can also clear the storage
  manually with the `clear_core` command.

# Debugger Notes

With a debugger attached, you can observe the coredump process more easily. This
reference implementation will detect if a debugger is attached. If so, it will
halt on a breakpoint after entering the `HardFault` handler and will breakpoint
again before reboot.

Mbed does not have a command to start a debug session (there's no `mbed debug`
or similar). You will have to start a debug session yourself. The exact process
used varies by target and debugger. This shows the two breakpoints under `gdb`:

```
Program received signal SIGTRAP, Trace/breakpoint trap.
memfault_platform_halt_if_debugging () at ../../libraries/memfault/platform_reference_impl/memfault_platform_core.c:13
13    __BKPT(0);
(gdb) next

memfault_platform_reboot () at ../../libraries/memfault/platform_reference_impl/memfault_platform_core.c:20
20    NVIC_SystemReset();
(gdb) continue
Continuing.
```

# Integrating into existing Mbed OS 5 projects

## Modify the Mbed application config file

In your existing project, modify the `mbed_app.json` file:

1. Add `MBED_FAULT_HANDLER_DISABLED=1` to the `macros` section. MBed OS 5 has
   its own crash reporting mechanism that runs from the `HardFault` handler. The
   Memfault SDK's `panics` component needs this disabled because it provides its
   own `HardFault` handler.

1. Add `memfault.project_api_key` to the `target_overrides` section with your
   API key. It must have quotes within the value (an Mbed configuration format
   quirk). See the `mbed_app.json` file in the demo app for the exact syntax.

## Copy the Memfault components

Copy the directory `$SDK/examples/mbed/apps/memfault_demo_app/memfault/` and
everything under it into your application. It needs to be placed on the same
level as `mbed-os/`. After copying, your app should look like:

```
your_app/
    mbed-os/
    memfault/
        components/
        platform_reference_impl/
        mbed_lib.json
    main.cpp
    mbed_app.json
```

Note that some of the files that need to be copied are symlinks to other
locations within the SDK. In your app, you'll want them copied over as regular
files. On most Unix-like systems, you can use `cp -RL` to do it:

```
memfault_demo_app$ cp -RL memfault /path/to/your_app/
```

The Mbed build system will automatically build the new files. Your app should
build successfully with `mbed compile` after copying and you should see the
files under `memfault/` in the build output.

## Modify the Memfault files

In the file `memfault_platform_device_info.c`:

- Rewrite the function `memfault_platform_get_device_info()`. This needs to
  report your product's identifying information, such as the unit's serial
  number.

In the file `memfault_platform_coredump.cpp`:

- Review the function `memfault_platform_coredump_get_regions()`. This function
  defines the regions of RAM that will be collected during a crash. You should
  collect as much RAM as your storage area will allow. The example collects only
  the interrupt stack (Mbed OS 5's internal use) and the BSS segment (statically
  allocated variables not initialized to a value). Alternatively, you may choose
  to capture all of RAM or add other regions of RAM such as the Mbed heap.

- Review the function `memfault_platform_coredump_storage_write()` and related
  functions. This reference implementation saves the coredump to an area of
  flash memory. You may change the size or location of the coredump in flash, or
  may store the coredump in an entirely different way. However, an
  implementation restriction is that this code runs from the `HardFault` handler
  where the RTOS is not running. Typically, Mbed OS 5 features implemented in
  C++ (`.cpp` files) use the RTOS and so they can't be used here. These files
  are often wrappers around lower-level C (`.c` files) that can usually be used.

## Modify your application

- Review how your application allocates its thread stacks. Many Mbed examples
  use the RTOS to allocate the thread stacks on the heap at runtime. This is
  fine but the heap is not captured by the reference implementation. You may
  want to change your application to use statically allocated areas for their
  stacks. This will allow them to be more easily collected.

- Determine how and when coredumps will be uploaded to Memfault. After a crash
  and the system comes back online, one of your RTOS threads will need to
  transfer the coredump to the Memfault cloud. Study the demo application to
  understand how to retrieve the coredump information (particularly the
  `get_core`, `print_chunk`, and `clear_core` functions. Implement your
  transport based on your product's requirements.

- If your app is using `MBED_ERROR()` and related functions, you may want to use
  [`mbed_set_error_hook()`](https://os.mbed.com/docs/mbed-os/v5.9/reference/error-handling.html))
  to catch them. This reference implementation does not catch errors from
  `MBED_ERROR()` but they can be cause in `mbed_set_error_hook()` where you can
  trigger a crash.
