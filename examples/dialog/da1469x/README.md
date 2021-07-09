# Memfault for the DA1469x Family of Wireless Microcontrollers

This example application shows an integration with Dialog Semiconductors SDK10
for the DA1469x family of Wireless Microcontrollers.

## Getting Started

We assume you have a working setup for firmware development on the Dialog
DA1469x family of devices:

- have Dialogs SmartSnippets Studio installed
- have installed the latest version of the DA1469x SDK (this example was tested
  with version 10.0.10.118)

### Adding Memfault to the Dialog SDK

Create a directory called `memfault` in the `middleware` folder of the Dialog
SDK and then clone the `memfault-firmware-sdk` repository into this folder. The
resulting directory structure should look like the following:

```
<DIALOG_SDK_ROOT>
├── binaries
├── config
├── doc
├── projects
├── sdk
│   ├── bsp
│   ├── free_rtos
│   ├── interfaces
│   └── middleware
│       ├── adapters
│       ├── cli
│       ├── config
│       ├── console
│       ├── dgtl
│       ├── haptics
|       ├── hw_tools
│       ├── logging
│       ├── mcif
│       ├── memfault
│       │   └── memfault-firmware-sdk
│       │       ├── cmake
│       │       ├── components
│       │       ├── examples
│       │       ├── makefiles
│       │       ├── ports
│       │       ├── scripts
│       │       ├── tasks
│       │       └── tests
│       ├── monitoring
│       ├── osal
│       └── segger_tools
└── utilities
```

## Using the demo app

The demo app is compatible with the following DA1469x Development Kit boards:

[DA14695 Development Kit USB](https://www.dialog-semiconductor.com/products/bluetooth-low-energy/da14695-development-kit-usb)

[DA14695 Development Kit PRO](https://www.dialog-semiconductor.com/products/bluetooth-low-energy/da14695-development-kit-pro)

### Building the demo app

The demo application can be built using the Eclipse/GCC (SmartSnippets Studio)
toolchain.

First, a few small changes must be made to the Dialog SDK to enable integration
of Memfault support. To make these changes navigate to the root of the Dialog
SDK and apply the following patches:

`git apply --ignore-whitespace ./sdk/middleware/memfault/memfault-firmware-sdk/ports/dialog/da1469x/gnu-build-id.patch`

`git apply --ignore-whitespace ./sdk/middleware/memfault/memfault-firmware-sdk/ports/dialog/da1469x/freertos-config.patch`

`git apply --ignore-whitespace ./sdk/middleware/memfault/memfault-firmware-sdk/ports/dialog/da1469x/fault-handlers.patch`

Next, open SmartSnippert Studio and import the demo application project, which
can be found in the following location:

`<DIALOG_SDK_ROOT>/sdk/middleware/memfault/memfault-firmware-sdk/examples/dialog/da1469x/apps/memfault_demo_app`

Build the application and then program into the flash memory contained on the
Development Kit board.

### Running the demo app

The demo app is a simple console based app that has commands to cause a crash in
several ways. Once a coredump is captured, it can be sent to Memfault's web
services to get analyzed.

Once the DA1469x Development Kit has been programmed, connect to it using a
Terminal emulator (such as TeraTerm etc.) at 115200-8-N-1.

**NOTE:** The DA1469x will enumerate two COM ports when connected to a PC,
select the lowest numbered of the two ports when connecting via a terminal
emulator.

The following commands are supported by the demo app and can be initiated by
sending the command listed:

- crash: Generate different types of crashes with crash [0..3]
- trace: Generate a trace event
- get_core: Get coredump
- clear_core: Clear coredump
- test_storage: Test coredump storage
- get_device_info: Display device information
- reboot: Reboot & record event
- export: Export chunk
- help: show list of available commands
