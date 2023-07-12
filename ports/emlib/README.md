# Silicon Labs emlib Port

This port adds implementations for coredump storage, reboot reasons, and
watchdog features using the Gecko SDK/emblib.

## Compatibility

The port has been built against v3.0 of the Gecko SDK. The port has been built
and tested against v4.3.0 using SimplicityStudio.

## Features

The port provides the following features:

- Coredump storage using a linker-defined region in internal Flash
- Reboot reason implementation for collecting reboot events
- An integration with the Watchdog peripheral to capture watchdog-triggered
  traces

## Adding To Your Project

The simplest way to add this to your project is to do the following:

1. Add the source files in `ports/emblib` to your build system
2. Add the include directory at `ports/include`
