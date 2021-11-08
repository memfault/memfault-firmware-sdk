# Example Platform Ports with Demo Applications

## Demo Applications

We have example integrations available for a number of development boards which
can be used as a reference while working on your integration or to explore the
Memfault SDK:

- Arm Mbed OS 5 / STMicroelectronics STM32F4 series (STM32F429I-DISC1)
- nRF5 SDK / Nordic nRF52840 (PCA10056)
- Quantum Leap Quantum Platform in C / STMicroelectronics STM32F4 series
  (STM32F407G-DISC1)
- WICED SDK / Cypress BCM943364WCD1
- Zephyr / STMicroelectronics STM32L4 series (B-L475E-IOT01A Discovery kit)
- Amazon FreeRTOS / Cypress PSoC 64 MCU (CY8CKIT-064S0S2-4343W)
- Dialog DA145xx and DA1469x
- Espressif ESP-IDF / ESP32 (ESP-WROVER-KIT)
- nRF Connect SDK / Nordic nRF9160 (PCA10090)

## Using the Demo Application

The demo application is a typical debug console style application that showcases
the various SDK features through console commands.

If you have one of the supported boards and want to jump right in, look at the
"Getting Started" section in `examples/**/README.md`. The file contains detailed
information on how to build and run the demo application for the specific
platform.

## Directory Structure

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

# Common Tasks

The SDK uses [pyinvoke] to offer a convenient way to run common tasks such as
building, flashing and debugging demo applications.

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
