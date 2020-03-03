[![CircleCI](https://circleci.com/gh/memfault/memfault-firmware-sdk.svg?style=svg)](https://circleci.com/gh/memfault/memfault-firmware-sdk)
[![Coverage](https://img.shields.io/codecov/c/gh/memfault/memfault-firmware-sdk/master)](https://codecov.io/gh/memfault/memfault-firmware-sdk/)

# Memfault Firmware SDK

Ship Firmware with Confidence.

# Getting Started

The Memfault SDK can be integrated with _any_ ARM Cortex-M based MCU.

If you'd like to see the type of data which can be collected before writing
_any_ code, check out https://try.memfault.com

## Quick Start Guides

We have Cortex-M step-by-step integration guides available for the following
compilers:

- [GNU GCC Guide](https://mflt.io/gcc-tutorial)
- [ARM MDK Guide](https://mflt.io/mdk-tutorial)
- [IAR Guide](https://mflt.io/iar-tutorial)

## Example Applications

We also have example integrations available for a number of development boards
which can be used as a reference while working on your integration or to explore
the Memfault SDK:

- Arm Mbed OS 5 / STMicroelectronics STM32F4 series (STM32F429I-DISC1)
- nRF5 SDK / Nordic nRF52840 (PCA10056)
- Quantum Leap Quantum Platform in C / STMicroelectronics STM32F4 series
  (STM32F407G-DISC1)
- WICED SDK / Cypress BCM943364WCD1
- Zephyr / STMicroelectronics STM32L4 series (B-L475E-IOT01A Discovery kit)

If you have one of the supported boards and want to jump right in, look at the
"Getting Started" section in `platforms/**/README.md`.

Otherwise, continue reading to get an overview of the Memfault Firmware SDK.

# Components

The SDK is designed as a collection of components, so you can include only what
is needed for your project. The `components` folder contains the various
components of the SDK.

Each component contains a `README.md`, source code, header files and "platform"
header files.

The platform header files describe the interfaces which the component relies on
that you must implement.

For some of the platform dependencies we have provided ports that can be linked
into your system without modification. You can find them in the `ports` folder.

For some of the popular MCUs & vendor SDKs, we have already provided a reference
implementation for platform dependencies which can be found in the `platforms`
folder. These can also serve as a good example when initially setting up the SDK
on your platform.

### Main components

- `panics` – fault handling, coredump and reboot tracking and reboot loop
  detection API.
- `metrics` - used to monitor device health over time (i.e. connectivity,
  battery life, MCU resource utilization, hardware degradation, etc.)

Please refer to the `README.md` in each of these for more details.

### Support components

- `core` – common code that is used by all other components.
- `demo` - common code that is used by demo apps for the various platforms.
- `http` – http client API, to post coredumps and events directly to the
  Memfault service from devices.
- `util` – various utilities.

# Integrating the Memfault SDK

## Add Memfault SDK to Your Repository

The Memfault SDK can be added directly into your repository. The structure
typically looks like:

```
<YOUR_PROJECT>
├── memfault
│   └── sdk
│       # Contents of _this_ repo
│       # (https://github.com/memfault/memfault-firmware-sdk)
│   └── platform_port
│       # Implementations for the Memfault SDK dependencies
│       # specified in the platform include directory for each component in the SDK
│       ├── memfault_platform_core.c
│       ├── memfault_platform_coredump.c
│       ├── [...]
```

If you are using `git`, the Memfault SDK is typically added to a project as a
submodule:

```
$ git submodule add git@github.com:memfault/memfault-firmware-sdk.git $YOUR_PROJECT/memfault/sdk
```

This makes it easy to track the history of the Memfault SDK. You should not need
to make modifications to the Memfault SDK itself so an update usually just
involves pulling the latest upstream, checking [CHANGES.md](CHANGES.md) to see
if any modifications are needed, and updating to the new submodule commit to
your repo.

Alternatively, the Memfault SDK may be added to a project as a git subtree or by
copying the source into a project.

## Add sources to Build System

### Make

If you are using `make`, `makefiles/MemfaultWorker.mk` can be used to very
easily collect the source files and include paths required by the SDK.

```c
MEMFAULT_SDK_ROOT := <The to the root of this repo from your project>
MEMFAULT_COMPONENTS := <The SDK components to be used, i.e "core panics util">
include $(MEMFAULT_SDK_ROOT)/makefiles/MemfaultWorker.mk
<YOUR_SRC_FILES> += $(MEMFAULT_COMPONENTS_SRCS)
<YOUR_INCLUDE_PATHS> += $(MEMFAULT_COMPONENTS_INC_FOLDERS)
```

### Cmake

If you are using `cmake`, `cmake/Memfault.cmake` in a similar fashion to
collection source files and include paths:

```
MEMFAULT_SDK_ROOT := <The to the root of this repo from your project>
MEMFAULT_COMPONENTS := <The SDK components to be used, i.e "core panics util">
include(${MEMFAULT_SDK_ROOT}/cmake/Memfault.cmake)
memfault_library(${MEMFAULT_SDK_ROOT} MEMFAULT_COMPONENTS
MEMFAULT_COMPONENTS_SRCS MEMFAULT_COMPONENTS_INC_FOLDERS)

# ${MEMFAULT_COMPONENTS_SRCS} contains the sources
# needed for the library and ${MEMFAULT_COMPONENTS_INC_FOLDERS} contains the include paths
```

### Other Build Systems

If you are not using one of the above build systems, to include the SDK you need
to do is:

- Add the `.c` files located at `components/<component>/src/*.c` to your build
  system
- Add `components/<component>/include` to the include paths you pass to the
  compiler

# Platforms

The `platforms` folder contains support files for some of the popular platforms.
The structure of each platform folder varies from platform to platform,
following the idiosyncrasies of each.

Each platform folder contains at least:

- `README.md` - instructions on how to build and run the demo app, as well as
  integration notes specific for the given platform.
- `**/platform_reference_impl` – implementations of platform functionality that
  the Memfault SDK depends on for the given platform.
- `**/memfault_demo_app` – demo application for the given platform (see Demo
  applications below).
- Build system files (specific to the platform) to compile the components as
  well as the demo applications for the platform.

# Demo applications

The SDK includes a demo application for each supported platform. The demo
application is a typical debug console style application that showcases the
various SDK features through console commands.

Check out the `platforms` folder for the platform of your interest. The
`platforms/**/README.md` file contains detailed information on how to build and
run the demo application for the specific platform.

# Common Tasks

The SDK uses [pyinvoke] to offer a convenient way to run common tasks such as
building, flashing, debugging, etc.

### Installing Pyinvoke

To install `pyinvoke`, make sure you have a recent version of Python 3.x
installed. Then run `pip3 install -r requirements.txt` to install it.

> Tip: use a [virtualenv] to avoid conflicts with dependencies of other projects
> that use Python

[pyinvoke]: https://www.pyinvoke.org
[virtualenv]:
  https://packaging.python.org/tutorials/installing-packages/#creating-virtual-environments

### Tasks

To get a list of available "tasks", run `invoke --list` from anywhere inside the
SDK. It will print the list of available tasks, for example:

```bash
$ invoke --list
Available tasks:

  nrf.build             Build a demo application that runs on the nrf52
  nrf.clean             Clean demo application that runs on the nrf52
  nrf.console           Start a RTT console session

... etc ...
```

#### Running the unit tests

The SDK code is covered extensively by unit tests. They can be found in the
`tests/` folder. If you'd like to run them yourself, make sure you have
[CPPUTest](https://cpputest.github.io) installed. When using macOS, you can
install it from homebrew by running `brew install cpputest`.

To build and run the unit tests, just run

```bash
$ invoke test
```

To learn more about unit testing best practices for firmware development, check
out
[our blog post on this topic](https://interrupt.memfault.com/blog/unit-testing-basics)!

The unit tests are run by CircleCI upon every commit to this repo. See badges at
the top for build & test coverage status of the `master` branch.

# FAQ

- Where are the Makefiles?

  - For each supported platform (see `platforms/**`), build system files are
    included for the specific platforms (for example, Makefiles are included for
    WICED and CMake files for ESP32). At the moment, there are no "generic"
    build system files provided, but the platform-specific ones are simple
    enough to serve as examples of how they could be integrated into your own
    project.

- Why does a coredump not show up under "Issues" after uploading it?

  - Make sure to upload the symbols to the same project to which you upload
    coredumps. Also make sure the software type and software version reported by
    the device (see "Device information" in `components/core/README.md`) match
    the software type and software version that was entered when creating the
    Software Version and symbol artifact online.
    [More information on creating Software Versions and uploading symbols can be found here](https://mflt.io/2LGUDoA).

- I'm getting error XYZ, what to do now?

  - Don't hesitate to contact us for help! You can reach us through
    [support@memfault.com](mailto:support@memfault.com).
