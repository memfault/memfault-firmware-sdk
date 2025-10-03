#
# Copyright (c) Memfault, Inc.
# See LICENSE for details
#

from __future__ import annotations

try:
    from shutil import which  # Python 3
except ImportError:  # Python 2
    from distutils.spawn import find_executable as which


def gdb_find(prefix: str | None = None) -> str:
    if prefix is None:
        prefix = "arm-none-eabi-"
    base_name = "{}gdb".format(prefix)
    ordered_names = ("{}-py".format(base_name), base_name)
    for name in ordered_names:
        gdb = which(name)
        if gdb:
            return gdb
    raise FileNotFoundError("Cannot find {}".format(" or ".join(ordered_names)))


def gdb_build_cmd(
    extra_args: str | None,
    elf: str,
    gdb_port: int,
    reset: bool = True,
    gdb_prefix: str | None = None,
) -> str:
    base_cmd = '{gdb} --eval-command="target remote localhost:{port}"'.format(
        gdb=gdb_find(gdb_prefix), port=gdb_port
    )
    if extra_args is None:
        extra_args = ""
    reset_cmd = '--ex="mon reset"' if reset else ""
    base_cmd += " {reset_cmd} {} {reset_cmd} --se={}".format(extra_args, elf, reset_cmd=reset_cmd)
    return base_cmd
