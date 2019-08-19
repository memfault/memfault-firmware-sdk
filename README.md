# Memfault Firmware SDK

Ship Firmware with Confidence.

# Getting Started

The Memfault SDK can be integrated with _any_ ARM Cortex-M based MCU.

If you'd like to see the type of data which can be collected before doing any
integration, check out https://try.memfault.com

The easiest way to get started integrating is by using a development board for
which the Memfault SDK already has a reference integration. At the moment,
examples are available for:

- WICED SDK / Cypress BCM943364WCD1
- nRF5 SDK / Nordic nRF52840 (PCA10056)

Coming soon:

- ESP-IDF / Espressif ESP32
- Apache Mynewt / Nordic nRF52840 (PCA10056)

If you have one of the supported boards and want to jump right in, look at the
"Getting Started" section in `platforms/**/README.md`.

Otherwise, continue reading to get an overview of the Memfault Firmware SDK.

# Components

The SDK is designed as a collection of components, so you can include only what
is needed for your project. The `components` folder contains the various
components of the SDK.

Each component contains a `README.md`, source code, header files and "platform"
header files.

The platform header files describe the interfaces which the component relies on.
For some of the popular platforms, we have already implemented platform
dependencies. Also see the `platforms` folder.

### Main components

- `panics` – fault handling, coredump and reboot tracking and reboot loop
  detection API.

Please refer to the `README.md` in each of these for more details.

### Support components

- `core` – common code that is used by all other components.
- `demo` - common code that is used by demo apps for the various platforms.
- `http` – http client API, to post coredumps and events directly to the
  Memfault service from devices.
- `util` – various utilities.

# Platforms

The `platforms` folder contains support files for some of the popular platforms.
The structure of each platform folder varies from platform to platform,
following the idiosyncracies of each.

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
  nrf.clean             Build a demo application that runs on the nrf52
  nrf.console           Start a RTT console session

... etc ...
```

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
    coredumps. Also make sure the firmware ("Release") version and hardware
    version reported by the device (see "Device information" in
    `components/core/README.md`) match the "Release" version and hardware
    version that was entered when creating the Release and symbol artifact
    online.
    [More information on creating Releases and uploading symbols can be found here](https://www.notion.so/memfault/Releasing-Firmware-UI-eb9499017e86432fa75e8b78dfc17891).

- I'm getting error XYZ, what to do now?

  - Don't hesitate to contact us for help! You can reach us through
    [support@memfault.com](mailto:support@memfault.com).
