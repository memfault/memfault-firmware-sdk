#
# Copyright (c) Memfault, Inc.
# See LICENSE for details
#

from __future__ import annotations

import os
import re
import shutil
import sys
from typing import TYPE_CHECKING

from invoke import Collection, task

from .gdb import gdb_build_cmd

if TYPE_CHECKING:
    from tasks.lib.invoke_utils import Context

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


def _run_idf_script(ctx: "Context", *args: str, **kwargs: object) -> None:
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
            env={
                "IDF_PATH": ESP32_IDF_ROOT,
                # newer versions of ESP-IDF require this too (it's much easier
                # when we can just do '. export.sh' instead!). provide a
                # reasonable default guess if unset.
                "ESP_ROM_ELF_DIR": os.getenv(
                    "ESP_ROM_ELF_DIR", "~/.espressif/tools/esp-rom-elfs/20241011/"
                ),
            },
            **kwargs,
        )


@task
def run_xtensa_toolchain_check(ctx: "Context") -> None:
    if sys.version_info.major < 3:
        # shutil which is only available for python3
        return

    xtensa_toolchain = shutil.which("xtensa-esp32-elf-gcc")
    if xtensa_toolchain is None:
        msg = (
            "Couldn't find esp32 toolchain. Currently using the toolchain which can be"
            " found here {}".format(
                "https://docs.espressif.com/projects/esp-idf/en/v3.1/get-started/index.html#setup-toolchain"
            )
        )

        raise FileNotFoundError(msg)


@task(pre=[run_xtensa_toolchain_check])
def esp32_app_build(ctx: "Context") -> None:
    """Build the ESP32 test app"""
    _run_idf_script(ctx, "build")


@task
def esp32_app_clean(ctx: "Context") -> None:
    """Clean the ESP32 test app"""
    _run_idf_script(ctx, "fullclean")


@task(pre=[run_xtensa_toolchain_check])
def esp32s2_app_build(ctx: "Context") -> None:
    """Build the ESP32-S2 test app"""
    # !NOTE! 'set-target' was added in ESP-IDF v4.1. If you are using an older
    # version of ESP-IDF, building for the ESP32-S2 + ESP32-S3 won't work.
    _run_idf_script(ctx, "set-target esp32s2")
    _run_idf_script(ctx, "build")


@task(pre=[run_xtensa_toolchain_check])
def esp32s3_app_build(ctx: "Context") -> None:
    """Build the ESP32-S3 test app"""
    _run_idf_script(ctx, "set-target esp32s3")
    _run_idf_script(ctx, "build")


@task
def esp32_app_flash(ctx: "Context") -> None:
    """Flash the ESP32 test app"""
    _run_idf_script(ctx, "flash")


@task
def esp32_console(ctx: "Context") -> None:
    """Flash the ESP32 test app"""
    _run_idf_script(ctx, "monitor")


@task
def esp32_app_menuconfig(ctx: "Context") -> None:
    """Run menuconfig for the ESP32 test app"""
    _run_idf_script(ctx, "menuconfig", pty=True)


@task
def esp32_openocd(ctx: "Context") -> None:
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
def esp32_app_gdb(ctx: "Context", gdb: int | None = None, reset: bool = False) -> None:
    """Launches xtensa-gdb with app elf and connects to openocd gdb server"""
    if gdb is None:
        gdb = OPENOCD_GDB_PORT_DEFAULT
    with ctx.cd(ESP32_TEST_APP_ROOT):
        gdb_cmd = gdb_build_cmd(
            "", ESP32_TEST_APP_ELF, gdb, gdb_prefix="xtensa-esp32-elf-", reset=reset
        )
        ctx.run(gdb_cmd, pty=True)


@task
def esp32_decode_backtrace(
    ctx: "Context", backtrace_str: str, symbol_file: str, verbose: bool = False
) -> None:
    """Decode a backtrace emitted by ESP-IDF panic handling

    The backtrace_str should be passed as a string of separated address pairs
    where each pair has the format "pc:sp". For example:
        "0x40081cda:0x3ffd00a0 0x40082ce3:0x3ffd00c0 0x4008927d:0x3ffd00e0"

    This backtrace is printed by the ESP-IDF debug helpers here:
    https://github.com/espressif/esp-idf/blob/v5.4.2/components/esp_system/port/arch/xtensa/debug_helpers.c

    Note: `idf.py monitor` will automatically decode the backtrace for you
    https://docs.espressif.com/projects/esp-idf/en/stable/esp32/api-guides/tools/idf-monitor.html#automatic-address-decoding
    so this task is primarily used if only the raw backtrace is supplied (e.g. via support ticket or logs).
    """

    addr_tuples = [
        (int(pc, 16), int(sp, 16))
        for pc, sp in re.findall(r"0x([0-9a-fA-F]+):0x([0-9a-fA-F]+)", backtrace_str)
    ]

    for i in range(len(addr_tuples)):
        result = ctx.run(
            "addr2line -e {} {address:x}".format(symbol_file, address=addr_tuples[i][0]),
            hide=True,
            warn=True,
        )
        if result.ok:
            print("Frame {frame:>2}: {line}".format(frame=i, line=result.stdout.strip()))
            if verbose:
                print("  PC  = 0x{address:x}".format(address=addr_tuples[i][0]))
                print("  SP  = 0x{sp:x}".format(sp=addr_tuples[i][1]))
        else:
            print(
                "Error processing 0x{address:x}: {error}".format(
                    address=addr_tuples[i][0], error=result.stderr.strip()
                )
            )


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
ns.add_task(esp32_decode_backtrace, name="decode-backtrace")
