#
# Copyright (c) Memfault, Inc.
# See LICENSE for details
#

from __future__ import annotations

from collections.abc import Generator
from contextlib import contextmanager
from os import uname
from typing import TYPE_CHECKING

if TYPE_CHECKING:
    from tasks.lib.invoke_utils import Context

APPLE_FTDI_DRIVER_BUNDLE_ID = "com.apple.driver.AppleUSBFTDI"


def is_macos() -> bool:
    return uname()[0] == "Darwin"


def _unload_apple_ftdi_driver_if_needed(ctx: Context) -> bool | None:
    if not is_macos():
        return None
    result = ctx.run("kextstat -b {}".format(APPLE_FTDI_DRIVER_BUNDLE_ID), hide=True)
    loaded = APPLE_FTDI_DRIVER_BUNDLE_ID in result.stdout
    if loaded:
        print("Unloading Apple FTDI driver...")
        ctx.run("sudo kextunload -b {}".format(APPLE_FTDI_DRIVER_BUNDLE_ID))
    return loaded


def _load_apple_ftdi_driver_if_needed(ctx: Context) -> None:
    if not is_macos():
        return
    print("Re-loading Apple FTDI driver...")
    ctx.run("sudo kextload -b {}".format(APPLE_FTDI_DRIVER_BUNDLE_ID))


@contextmanager
def apple_ftdi_driver_disable(ctx: Context) -> Generator[None, None, None]:
    was_ftdi_driver_loaded = _unload_apple_ftdi_driver_if_needed(ctx)
    try:
        yield
    finally:
        if was_ftdi_driver_loaded:
            _load_apple_ftdi_driver_if_needed(ctx)
