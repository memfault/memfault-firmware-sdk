## Memfault Zephyr `nucleo_wba55cg` Example Application

This folder contains an example integration of the Memfault SDK using the port
provided in `ports/zephyr`

The demo was tested using the
[ST NUCLEO-WBA55CG board](https://docs.zephyrproject.org/3.6.0/boards/st/nucleo_wba55cg/doc/nucleo_wba55cg.html).

### Prerequisite

We assume you already have a working Zephyr toolchain installed locally.
Step-by-step instructions can be found in the
[Zephyr Documentation](https://docs.zephyrproject.org/2.5.0/getting_started/index.html#build-hello-world).

### Setup

From a Zephyr-enabled shell environment (`west` tool is available), initialize
the workspace:

```bash
west init -l memfault_demo_app
west update
west blobs fetch stm32
```

### Building

```bash
west build --board nucleo_wba55cg memfault_demo_app
[...]
[271/271] Linking C executable zephyr/zephyr.elf
```

### Flashing

The build can be flashed on the development board using `west flash` ( See
Zephyr
["Building, Flashing, & Debugging" documentation](https://docs.zephyrproject.org/3.6.0/guides/west/build-flash-debug.html?highlight=building%20flashing#flashing-west-flash))

The default runner is `STM32_Programmer_CLI`, which must be available on `PATH`
when running `west flash`. `STM32_Programmer_CLI` is installed
[standalone](https://www.st.com/en/development-tools/stm32cubeprog.html) or from
the STM32CubeIDE installation.
