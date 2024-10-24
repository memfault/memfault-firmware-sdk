# Zephyr Specific Port

## Overview

This directory contains an implementation of the dependency functions needed to
integrate the Memfault SDK into the Zephyr RTOS.

The instructions below assume you have an environment already setup for building
and flashing a Zephyr application. If you do not, see the official
[getting started guide](https://docs.zephyrproject.org/2.0.0/getting_started/index.html#build-hello-world).

## Directory Structure

The Memfault Zephyr port is set up as a Zephyr module. The directory structure
is as follows:

```bash
├── CMakeLists.txt  # CMake file for the Memfault Zephyr module
├── common/         # Shared files for Zephyr integration
├── config/         # Configuration files for Memfault SDK
├── include/        # Header files for Zephyr integration
├── Kconfig         # Kconfig file for Memfault Zephyr module
├── ncs/            # Files specific to the Zephyr-based Nordic nRF-Connect SDK
├── panics/         # Files for handling panics in Zephyr
└── README.md       # This file
```

## Integrating SDK

Follow the guide here for how to integrate the Memfault SDK into a Zephyr
project:

<https://docs.memfault.com/docs/mcu/zephyr-guide>

## Demo

An example integration and instructions can be found for a QEMU-emulated board
in `$MEMFAULT_SDK_ROOT/examples/zephyr/qemu/README.md`
