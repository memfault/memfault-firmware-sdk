# Memfault FreeRTOS Demo Application

This example demonstrates integrating Memfault with FreeRTOS applications. The example uses the
QEMU target, MPS2 Cortex M3 AN385, to run the application and is based on the
[MPS2 Cortex M3 AN385 FreeRTOS Demo](https://github.com/FreeRTOS/FreeRTOS/tree/main/FreeRTOS/Demo/CORTEX_M3_MPS2_QEMU_GCC).
This file outlines various components of the example application. For more information on the
MCU SDK, please see our documentation at [docs.memfault.com](docs.memfault.com).

## Prerequisites

The project requires the following:

- git
- GNU Make
- A version of the [Arm GNU Toolchain](https://developer.arm.com/downloads/-/arm-gnu-toolchain-downloads)
  - **Note**: The project assumes the toolchain can be found via your environment's PATH, otherwise
  you may specify by setting `ARM_CC` with the full path to the toolchain `/bin` directory

## FreeRTOS Installation

By default the project will clone FreeRTOS V10.4.6 into the project directory. If you would like to use a
different version, please set `FREERTOS_DIR` to an existing FreeRTOS directory.

## Building

To build, from the project directory run:

```bash
make
```

## Running

To run, from the project directory run:

```bash
make run
```

With the console open you may test out various features using the demo CLI commands. A simple example of
testing assert handling and data export:

```bash
MFLT:[INFO] GNU Build ID: 8a45620abae6842848670c379411d8b31846eadc
MFLT:[INFO] S/N: freertos-example
MFLT:[INFO] SW type: qemu-app
MFLT:[INFO] SW version: 1.0.0
MFLT:[INFO] HW version: qemu-mps2-an385
MFLT:[INFO] Memfault Initialized!

mflt> test_assert
MFLT:[INFO] GNU Build ID: 8a45620abae6842848670c379411d8b31846eadc
MFLT:[INFO] S/N: freertos-example
MFLT:[INFO] SW type: qemu-app
MFLT:[INFO] SW version: 1.0.0
MFLT:[INFO] HW version: qemu-mps2-an385
MFLT:[INFO] Memfault Initialized!

mflt> export
MFLT:[INFO] MC:SK8SgQlDT1JFAgYAAwwTFAABTAYAIwGAAAD//wAAe30AAEBmACB7fQwACKUHgFMAICilE3BTACA3WAAAhG8KABNhMP8/IHBTACAMDgABFAY=:
MFLT:[INFO] MC:wE0AKYpFYgq65oQoSGcMN5QR2LMYRurcAg4AARAGACFmcmVlcnRvcy1leGFtcGxlCg4AAQUGAAsxLjAuMAsOAAEIBgARcWVtdS1hcHAEDgA=:
MFLT:[INFO] MC:wJsBAQ8GAB9xZW11LW1wczItYW4zODUHDgABBAYAASgGAAEFDgABBAYACQGAAAABBgAJJO0A4BwSAAEBCAABQCAAAQEGAAkY7QDgDBoABP8=:
MFLT:[INFO] MC:wOgBAQEGAAkE4ADgEB4ACQcAAQABBgAJBO0A4AgGAAcD+AAECAABAQYACfztAOAEDgABAQgAB+EA4AQOAAEBCAAH4gDgBA4AAQEIAAfjAOA=:
MFLT:[INFO] MC:wLUCBA4AAQEIAAfkAOAgRgABAQYACUhlACAoBgAGARoAAakIACcCAADMWQAgzFkAIAACAABpvQAAAQYA4QLMWQAgAAIAAAE2R05VIEJ1aWw=:
# Output clipped
```

The exported output can be pasted into the Memfault web app for processing. See this [page](https://docs.memfault.com/docs/mcu/arm-cortex-m-guide/#post-data-to-cloud-via-local-debug-setup)
in our docs.

## Debugging

To debug, from the project directory run:

```bash
make debug
```

QEMU will start the application and wait for a debugger to attach. From a separate terminal run:

```bash
make gdb
```

## Application Features

### Console and Memfault Demo CLI

The application is built with the MCU SDK's demo CLI. This CLI includes commands to trigger faults;
collect data from logs, trace events, and metrics; and export data to Memfault's web application
for processing. For more info on the commands available use the "help" command in the console or
view the source code at `memfault-firmware-sdk/components/demo`.

### Application Metrics

Two simple tasks are used to illustrate creating metrics to measure your application's performance.
The first task, Heap Task, allocates and deallocates memory periodically. A second tasks, Metrics Task,
sets metrics based on the state of the system heap. The two metrics produced are:

- Example_HeapFreeBytes: Measures current total bytes free in the heap
- Example_MinHeapBytesFree: Measures the lowest total bytes free in the heap since boot

These metrics are collected every 60 seconds in this example. This can be changed by modifying
`MEMFAULT_METRICS_HEARTBEAT_INTERVAL_SECS` in `memfault_platform_config.h`.

### Heap Tracing

The heap tracing capabilities of the MCU SDK are enabled by default. The SDK will collect a set of
the most recent allocations with their location in code and size when a coredump is saved.

### Logging Collection

The application is configured to collect the most recent logs along whenever a coredump is collected.
