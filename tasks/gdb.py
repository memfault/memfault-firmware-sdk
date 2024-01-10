#
# Copyright (c) Memfault, Inc.
# See License.txt for details
#

try:
    from shutil import which  # Python 3
except ImportError:  # Python 2
    from distutils.spawn import find_executable as which


def gdb_find(prefix=None):
    if prefix is None:
        prefix = "arm-none-eabi-"
    base_name = "{}gdb".format(prefix)
    ordered_names = ("{}-py".format(base_name), base_name)
    for name in ordered_names:
        gdb = which(name)
        if gdb:
            return gdb
    raise FileNotFoundError("Cannot find {}".format(" or ".join(ordered_names)))


def gdb_build_cmd(extra_args, elf, gdb_port, reset=True, gdb_prefix=None):
    base_cmd = '{gdb} --eval-command="target remote localhost:{port}"'.format(
        gdb=gdb_find(gdb_prefix), port=gdb_port
    )
    if extra_args is None:
        extra_args = ""
    reset_cmd = '--ex="mon reset"' if reset else ""
    base_cmd += " {reset_cmd} {} {reset_cmd} --se={}".format(extra_args, elf, reset_cmd=reset_cmd)
    return base_cmd
