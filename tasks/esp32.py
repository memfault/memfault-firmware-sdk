#
# Copyright (c) Memfault, Inc.
# See License.txt for details
#

import os
import platform
import shutil
import sys
from glob import glob

from invoke import Collection, task
from pyftdi.usbtools import UsbTools

from .gdb import gdb_build_cmd

TASKS_DIR = os.path.dirname(__file__)
MEMFAULT_SDK_ROOT = os.path.join(TASKS_DIR, "..")
ESP32_PLATFORM_ROOT = os.path.join(MEMFAULT_SDK_ROOT, "examples", "esp32")
ESP32_IDF_ROOT = os.path.join(ESP32_PLATFORM_ROOT, "esp-idf")
ESP32_IDF_SCRIPT = os.path.join(ESP32_IDF_ROOT, "tools", "idf.py")
ESP32_COREDUMP_SCRIPT = os.path.join(ESP32_IDF_ROOT, "components", "espcoredump", "espcoredump.py")
ESP32_TEST_APP_ROOT = os.path.join(ESP32_PLATFORM_ROOT, "apps", "memfault_demo_app")
ESP32_TEST_APP_ELF = os.path.join(ESP32_TEST_APP_ROOT, "build", "memfault-esp32-demo-app.elf")
ESP32_FTDI_VID_PID = [(0x0403, 0x6010)]

OPENOCD_GDB_PORT_DEFAULT = 3333


def _run_idf_script(ctx, *args, **kwargs):
    # allow selecting a specific python interpreter instead of the active one.
    # this is necessary in CI, because the mbed build task modifies the python
    # environment 😖, and idf.py runs a pkg_resources.require() which fails if
    # the mbed task modified any deps in an incompatible way. it's better to
    # keep the esp_idf environment isolated and just use it to run idf.py
    python = os.getenv("ESP_IDF_PYTHON", sys.executable)

    with ctx.cd(ESP32_TEST_APP_ROOT):
        ctx.run(
            "{python} {idf} {args}".format(
                python=python, idf=ESP32_IDF_SCRIPT, args=" ".join(args)
            ),
            env={"IDF_PATH": ESP32_IDF_ROOT},
            **kwargs,
        )


def _esp32_guess_console_port():
    def _esp32_find_console_port():
        host_os = platform.system()
        if host_os == "Darwin":
            usb_paths = glob("/dev/cu.usbserial-*1")
            if usb_paths:
                return usb_paths[0]

            # Try pyftdi only on MacOS as it fails on Linux.
            # Actually, Espressif's esptool.py seems not to
            # like this at all.
            print("Trying FTDI first")
            devs = UsbTools.find_all(ESP32_FTDI_VID_PID)
            if devs:
                return "ftdi://ftdi:2232/2"
        elif host_os == "Linux":
            # Try ttyUSB1, no need to glob.
            serial_device = "/dev/ttyUSB1"
            print("Trying {serdev}".format(serdev=serial_device))
            if os.path.exists(serial_device):
                return serial_device

        print(
            "Cannot find ESP32 console /dev/... nor ftdi:// path, please specify it manually using"
            " --port"
        )
        sys.exit(1)

    port = _esp32_find_console_port()
    print("No --port specified, using console port {port}".format(port=port))
    return port


@task
def run_xtensa_toolchain_check(ctx):
    if sys.version_info.major < 3:
        # shutil which is only available for python3
        return

    xtensa_toolchain = shutil.which("xtensa-esp32-elf-gcc")
    if xtensa_toolchain is None:
        msg = (
            "Couldn't find arm toolchain. Currently using the toolchain which can be"
            " found here {}".format(
                "https://docs.espressif.com/projects/esp-idf/en/v3.1/get-started/index.html#setup-toolchain"
            )
        )

        raise FileNotFoundError(msg)


@task(pre=[run_xtensa_toolchain_check])
def esp32_app_build(ctx):
    """Build the ESP32 test app"""
    _run_idf_script(ctx, "build")


@task
def esp32_app_clean(ctx):
    """Clean the ESP32 test app"""
    _run_idf_script(ctx, "fullclean")


@task(pre=[run_xtensa_toolchain_check])
def esp32s2_app_build(ctx):
    """Build the ESP32-S2 test app"""
    # !NOTE! 'set-target' was added in ESP-IDF v4.1. If you are using an older
    # version of ESP-IDF, building for the ESP32-S2 + ESP32-S3 won't work.
    _run_idf_script(ctx, "set-target esp32s2")
    _run_idf_script(ctx, "build")


@task(pre=[run_xtensa_toolchain_check])
def esp32s3_app_build(ctx):
    """Build the ESP32-S3 test app"""
    _run_idf_script(ctx, "set-target esp32s3")
    _run_idf_script(ctx, "build")


@task
def esp32_app_flash(ctx, port=None):
    """Flash the ESP32 test app"""
    if port is None:
        port = _esp32_guess_console_port()
    _run_idf_script(ctx, "-p {port}".format(port=port), "flash")


@task
def esp32_console(ctx, port=None):
    """Flash the ESP32 test app"""
    if port is None:
        port = _esp32_guess_console_port()
    # For now, just use miniterm, idf_monitor.py doesn't play nice with pyftdi for some reason :(
    # _run_idf_script(ctx, '-p {port}'.format(port=port), 'monitor', pty=True)
    ctx.run("miniterm.py --eol CR --raw {port} 115200".format(port=port), pty=True)


@task
def esp32_app_menuconfig(ctx):
    """Run menuconfig for the ESP32 test app"""
    _run_idf_script(ctx, "menuconfig", pty=True)


@task
def esp32_openocd(ctx):
    """Launch openocd"""
    if "ESP32_OPENOCD" not in os.environ:
        print("Set ESP32_OPENOCD environment variable to point to openocd-esp32 root directory!")
        print(
            "Download the openocd-esp32 binaries here:"
            " https://github.com/espressif/openocd-esp32/releases"
        )
        sys.exit(-1)
    with ctx.cd(os.environ["ESP32_OPENOCD"]):
        ctx.run(
            "bin/openocd -s share/openocd/scripts "
            "-f interface/ftdi/esp32_devkitj_v1.cfg -f board/esp-wroom-32.cfg",
            pty=True,
        )


@task
def esp32_app_gdb(ctx, gdb=None, reset=False):
    """Launches xtensa-gdb with app elf and connects to openocd gdb server"""
    if gdb is None:
        gdb = OPENOCD_GDB_PORT_DEFAULT
    with ctx.cd(ESP32_TEST_APP_ROOT):
        gdb_cmd = gdb_build_cmd(
            "", ESP32_TEST_APP_ELF, gdb, gdb_prefix="xtensa-esp32-elf-", reset=reset
        )
        ctx.run(gdb_cmd, pty=True)


ns = Collection("esp32")
ns.add_task(esp32_console, name="console")
ns.add_task(esp32_openocd, name="gdbserver")
ns.add_task(esp32_app_build, name="build")
ns.add_task(esp32s2_app_build, name="build-s2")
ns.add_task(esp32s3_app_build, name="build-s3")
ns.add_task(esp32_app_clean, name="clean")
ns.add_task(esp32_app_flash, name="flash")
ns.add_task(esp32_app_gdb, name="app-gdb")
ns.add_task(esp32_app_menuconfig, name="app-menuconfig")
