#
# Copyright (c) Memfault, Inc.
# See LICENSE for details
#

from __future__ import annotations

import os
import shutil
import sys
from typing import TYPE_CHECKING

from invoke import Collection, task

from .gdb import gdb_build_cmd
from .print_chunk_watcher import PrintChunkWatcher

if TYPE_CHECKING:
    from tasks.lib.invoke_utils import Context

TASKS_DIR = os.path.dirname(__file__)
MEMFAULT_SDK_ROOT = os.path.dirname(TASKS_DIR)
NRF_ROOT = os.path.join(MEMFAULT_SDK_ROOT, "examples", "nrf5")
NRF_SDK_ROOT = os.path.join(NRF_ROOT, "nrf5_sdk")
NRF_DEMO_APP_ROOT = os.path.join(NRF_ROOT, "apps", "memfault_demo_app")
NRF_DEMO_APP_ELF = os.path.join(NRF_DEMO_APP_ROOT, "build", "memfault_demo_app_nrf52840_s140.out")

JLINK_GDB_SERVER_DEFAULT_PORT = 2331
JLINK_TELNET_SERVER_DEFAULT_PORT = 19021


@task
def run_arm_toolchain_check(ctx: Context) -> None:
    if sys.version_info.major < 3:
        # shutil which is only available for python3
        return

    arm_toolchain = shutil.which("arm-none-eabi-gcc")
    if arm_toolchain is None:
        msg = "Couldn't find arm toolchain. Currently using {} which can be found here {}".format(
            "8-2018-q4-major",
            "https://developer.arm.com/open-source/gnu-toolchain/gnu-rm/downloads",
        )

        raise FileNotFoundError(msg)


@task
def nrf_console(ctx: Context, telnet: int = JLINK_TELNET_SERVER_DEFAULT_PORT) -> None:
    """Start a RTT console session"""
    ctx.run(
        "JLinkRTTClient -LocalEcho Off -RTTTelnetPort {telnet_port}".format(telnet_port=telnet),
        watchers=[PrintChunkWatcher(ctx)],
    )


def _run_demo_app_make_command(ctx: Context, rules: str | None = None) -> None:
    cmd_base = "make -j5"
    cmd = cmd_base if rules is None else "{} {}".format(cmd_base, rules)
    with ctx.cd(NRF_DEMO_APP_ROOT):
        ctx.run(cmd, env={"GNU_INSTALL_ROOT": ""})


@task(pre=[run_arm_toolchain_check])
def nrf_build(ctx: Context, skip_app_sign: bool = False) -> None:
    """Build a demo application that runs on the nrf52"""
    _run_demo_app_make_command(ctx)


@task(pre=[run_arm_toolchain_check])
def nrf_clean(ctx: Context) -> None:
    """Clean demo application that runs on the nrf52"""
    _run_demo_app_make_command(ctx, "clean")


@task(
    pre=[run_arm_toolchain_check],
    help={
        "skip-softdevice-flash": "Skip flashing the softdevice. It's not changing so it only needs to be done once",
        "elf": "The elf to flash",
    },
)
def nrf_flash(
    ctx: Context,
    elf: str = NRF_DEMO_APP_ELF,
    gdb: int = JLINK_GDB_SERVER_DEFAULT_PORT,
    skip_softdevice_flash: bool = False,
) -> None:
    """Flash the application & softdevice (Master boot record, MBR + BLE stack)"""
    if not skip_softdevice_flash:
        _run_demo_app_make_command(ctx, "flash_softdevice")
    app_args = '--ex="load"'
    cmd = gdb_build_cmd(app_args, elf, gdb)
    ctx.run(cmd, pty=True)


@task
def nrf_gdbserver(
    ctx: Context,
    sn: str | None = None,
    device: str = "nRF52840_xxAA",
    gdb: int = JLINK_GDB_SERVER_DEFAULT_PORT,
    telnet: int = JLINK_TELNET_SERVER_DEFAULT_PORT,
) -> None:
    """Start a GDBServer that can attach to an nrf52 dev kit"""

    select_arg = "-select USB={}".format(sn) if sn else ""
    ctx.run(
        "JLinkGDBServer {select_arg} -if swd -device {device} -speed auto -port {gdb_port} \
-RTTTelnetPort {telnet_port}".format(
            device=device, select_arg=select_arg, gdb_port=gdb, telnet_port=telnet
        )
    )


@task
def nrf_eraseflash(ctx: Context) -> None:
    """Erase all flash contents of the nrf device putting the board back in a clean state.
    It's a good idea to run this before flashing a new application on the dev board"""
    cmd = "nrfutil device erase --all"
    ctx.run(cmd)


@task
def nrf_debug(
    ctx: Context, elf: str = NRF_DEMO_APP_ELF, gdb: int = JLINK_GDB_SERVER_DEFAULT_PORT
) -> None:
    """Run GDB, load the demo app ELF, and attach to the target via a running GDBServer"""
    cmd = gdb_build_cmd(None, elf, gdb, reset=False)
    ctx.run(cmd, pty=True)


ns = Collection("nrf")
ns.add_task(nrf_build, name="build")
ns.add_task(nrf_clean, name="clean")
ns.add_task(nrf_console, name="console")
ns.add_task(nrf_debug, name="debug")
ns.add_task(nrf_eraseflash, name="eraseflash")
ns.add_task(nrf_flash, name="flash")
ns.add_task(nrf_gdbserver, name="gdbserver")
