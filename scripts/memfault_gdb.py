#
# Copyright (c) 2019-Present Memfault, Inc.
# See License.txt for details
#
# -*- coding: utf-8 -*-
import argparse
import re
from struct import pack
from tempfile import TemporaryFile

try:
    import gdb
except ImportError:
    error_str = """
    This script can only be run within gdb!
    """
    raise Exception(error_str)


# Note: not using `requests` but using the built-in http.client instead, so
# there will be no additional dependencies other than Python itself.
try:
    from httplib import HTTPSConnection
except ImportError:
    from http.client import HTTPSConnection


MEMFAULT_DEFAULT_INGRESS_HOST = "ingress.memfault.com"


# FIXME: De-duplicate with code from gdb_util.py
UINT8_TYPE = gdb.lookup_type("uint8_t")
UINT16_TYPE = gdb.lookup_type("uint16_t")
UINT32_TYPE = gdb.lookup_type("uint32_t")
UINT64_TYPE = gdb.lookup_type("uint64_t")


def scalar_value_to_bytes(gdb_scalar_value, little_endian=True):
    """
    This helper is meant to be used with values that are not addressable, i.e. registers.
    If you've got an addressable value, it's probably faster/better to use the Inferior.read_memory() API.
    """
    SCALAR_VALUE_MAPPING = {
        1: (UINT8_TYPE, "B"),
        2: (UINT16_TYPE, "H"),
        4: (UINT32_TYPE, "I"),
        8: (UINT64_TYPE, "Q"),
    }
    width = gdb_scalar_value.type.sizeof
    gdb_type, fmt = SCALAR_VALUE_MAPPING[width]
    uint_value = int(gdb_scalar_value.cast(gdb_type))
    return pack(fmt, uint_value)


# FIXME: De-duplicate with code from trace_builder_coredump.py
def get_registers_for_thread(gdb_thread):
    gdb_thread.switch()
    frame = gdb.newest_frame()
    frame.select()

    register_names = []
    # GDB Doesn't have a convenient way to know all of the registers in Python, so this is the
    #  best way. Call this, rip out the first element in each row...that's the register name
    for reg_row in gdb.execute("info reg", to_string=True).strip().split("\n"):
        name = reg_row.split()[0]
        register_names.append(name)

    # Iterate over all register names and pull the value out of the frame
    register_list = {}
    for reg_name in register_names:
        # `info reg` will print all registers, even though they are not part of the core.
        # If that's the case, doing frame.read_register() will raise a gdb.error.
        try:
            value = frame.read_register(reg_name)
            value_str = str(value)
            if value_str != "<unavailable>":
                register_list[reg_name] = scalar_value_to_bytes(value)
        except:
            pass

    return register_list


# FIXME: De-duplicate with code from rtos_register_stacking.py
def concat_registers_dict_to_core_elf_layouted_data(regs):
    arm_v7m_register_names = (
        "r0",
        "r1",
        "r2",
        "r3",
        "r4",
        "r5",
        "r6",
        "r7",
        "r8",
        "r9",
        "r10",
        "r11",
        "r12",
        "sp",
        "lr",
        "pc",
        "xpsr",
    )
    result = b""
    for reg_name in arm_v7m_register_names:
        if reg_name not in regs:
            if reg_name == "xpsr" and "cpsr" in regs:
                reg_name = "cpsr"
            else:
                result += b"\x00\x00\x00\x00"
                continue
        result += regs[reg_name]
    return result


# FIXME: De-duplicate with code from core_convert.py
MEMFAULT_COREDUMP_MAGIC = 0x45524F43
MEMFAULT_COREDUMP_VERSION = 1
MEMFAULT_COREDUMP_FILE_HEADER_FMT = "<III"  # magic, version, file length (incl. file header)
MEMFAULT_COREDUMP_BLOCK_HEADER_FMT = "<bxxxII"  # type, address, block payload length


class MemfaultCoredumpBlockType(object):  # (IntEnum):  # trying to be python2.7 compatible
    CURRENT_REGISTERS = 0
    MEMORY_REGION = 1
    DEVICE_SERIAL = 2
    FIRMWARE_VERSION = 3
    HARDWARE_REVISION = 4
    TRACE_REASON = 5
    PADDING_REGION = 6
    MACHINE_TYPE = 7
    VENDOR_COREDUMP_ESP_IDF_V2_TO_V3_1 = 8


class MemfaultCoredumpWriter(object):
    def __init__(self):
        self.device_serial = "fakeserial"
        self.firmware_version = "1.0.0"
        self.hardware_revision = "proto"
        self.trace_reason = 5  # Debugger Halted
        self.machine_type = 40  # ARM
        self.regs = {}
        self.sections = []

    def add_section(self, section):
        self.sections.append(section)

    def _write(self, write, file_length=0):
        # file header:
        write(
            pack(
                MEMFAULT_COREDUMP_FILE_HEADER_FMT,
                MEMFAULT_COREDUMP_MAGIC,
                MEMFAULT_COREDUMP_VERSION,
                file_length,
            )
        )

        def _write_block(type, payload, address=0):
            write(pack(MEMFAULT_COREDUMP_BLOCK_HEADER_FMT, type, address, len(payload)))
            write(payload)

        _write_block(
            MemfaultCoredumpBlockType.CURRENT_REGISTERS,
            concat_registers_dict_to_core_elf_layouted_data(self.regs),
        )
        _write_block(MemfaultCoredumpBlockType.DEVICE_SERIAL, self.device_serial.encode("utf8"))
        _write_block(
            MemfaultCoredumpBlockType.FIRMWARE_VERSION, self.firmware_version.encode("utf8")
        )
        _write_block(
            MemfaultCoredumpBlockType.HARDWARE_REVISION, self.hardware_revision.encode("utf8")
        )
        _write_block(MemfaultCoredumpBlockType.MACHINE_TYPE, pack("<I", self.machine_type))
        _write_block(MemfaultCoredumpBlockType.TRACE_REASON, pack("<I", self.trace_reason))
        for section in self.sections:
            _write_block(MemfaultCoredumpBlockType.MEMORY_REGION, section.data, section.addr)

    def write(self, out_f):
        # Count the total size first:
        total_size = {"size": 0}

        def _counting_write(data):
            # nonlocal total_size  # Not python 2.x compatible :(
            # total_size += len(data)
            total_size["size"] = total_size["size"] + len(data)

        self._write(_counting_write)

        # Actually write out to the file:
        self._write(out_f.write, total_size["size"])


class Section(object):
    def __init__(self, addr, size, name, read_only=True):
        self.addr = addr
        self.size = size
        self.name = name
        self.read_only = read_only
        self.data = b""

    def __eq__(self, other):
        return (
            self.addr == other.addr
            and self.size == other.size
            and self.name == other.name
            and self.read_only == other.read_only
            and self.data == other.data
        )


def parse_maintenance_info_sections(output):
    fn_match = re.search(r"`([^']+)', file type", output)
    if fn_match is None:
        return None, None
    # Using groups() here instead of fn_match[1] for python2.x compatibility
    fn = fn_match.groups()[0]
    # Grab start addr, end addr, name and flags for each section line:
    # [2]     0x6b784->0x6b7a8 at 0x0004b784: .gnu_build_id ALLOC LOAD READONLY DATA HAS_CONTENTS
    section_matches = re.findall(
        r"\s+\[\d+\]\s+(0x[\da-fA-F]+)[^0]+(0x[\da-fA-F]+)[^:]+: ([^ ]+) (.*)$",
        output,
        re.MULTILINE,
    )

    def _tuple_to_section(tpl):
        addr = int(tpl[0], base=16)
        size = int(tpl[1], base=16) - addr
        name = tpl[2]
        read_only = "READONLY" in tpl[3]
        return Section(addr, size, name, read_only)

    sections = map(_tuple_to_section, section_matches)
    return fn, list(sections)


def is_debug_info_section(section):
    return section.name.startswith(".debug_")


def should_capture_section(section):
    # Assuming we never want to grab .text:
    if section.name == ".text":
        return False
    # Assuming we never want to grab debug info:
    if is_debug_info_section(section):
        return False
    # Sometimes these get flagged incorrectly as READONLY:
    if filter(lambda n: n in section.name, (".heap", ".bss", ".data", ".stack")):
        return True
    # Only grab non-readonly stuff:
    return not section.read_only


def http_post_coredump(coredump_file, project_key, host=MEMFAULT_DEFAULT_INGRESS_HOST):
    headers = {"Content-type": "application/octet-stream", "Memfault-Project-Key": project_key}
    conn = HTTPSConnection(host)
    conn.request("POST", "/api/v0/upload/coredump", body=coredump_file, headers=headers)
    response = conn.getresponse()
    status = response.status
    reason = response.reason
    conn.close()
    return status, reason


# FIXME: Duped from tools/gdb_memfault.py
class MemfaultGdbArgumentParseError(Exception):
    pass


class MemfaultGdbArgumentParser(argparse.ArgumentParser):
    def exit(self, status=0, message=None):
        if message:
            self._print_message(message)
        # Don't call sys.exit()
        raise MemfaultGdbArgumentParseError()


class Memfault(gdb.Command):
    def __init__(self):
        super(Memfault, self).__init__("memfault", gdb.COMMAND_USER, gdb.COMPLETE_NONE, prefix=True)

    def invoke(self, arg, from_tty):
        gdb.execute("help memfault")


class MemfaultCoredump(gdb.Command):
    """Captures a coredump from the target and uploads it to Memfault for analysis"""

    def __init__(self):
        super(MemfaultCoredump, self).__init__("memfault post_core", gdb.COMMAND_USER)

    def invoke(self, unicode_args, from_tty):
        try:
            parsed_args = self.parse_args(unicode_args)
        except MemfaultGdbArgumentParseError:
            return

        # import pydevd_pycharm
        # pydevd_pycharm.settrace("localhost", port=5555, stdoutToServer=True, stderrToServer=True)

        cd_writer = self.build_coredump_writer()
        with TemporaryFile() as f_out:
            cd_writer.write(f_out)
            f_out.seek(0)
            status, reason = http_post_coredump(
                f_out, parsed_args.key, host=parsed_args.ingress_host
            )

        if status == 409:
            print("Coredump already exists!")
        elif status >= 200 and status < 300:
            print("Coredump uploaded successfully!")
        else:
            print("Error occurred... HTTP Status {} {}".format(status, reason))

    def parse_args(self, unicode_args):
        parser = MemfaultGdbArgumentParser(description="Posts a coredump to Memfault for analysis")
        parser.add_argument(
            "source",
            help="Source of coredump: 'live' (default) or 'storage' (target's live RAM or stored coredump)",
            default="live",
            choices=["live", "storage"],
            nargs="?",
        )
        parser.add_argument("--key", help="Project Key", default="")
        parser.add_argument(
            "--ingress-host",
            default=MEMFAULT_DEFAULT_INGRESS_HOST,
            help="The Memfault server host for coredump ingression (default: {})".format(
                MEMFAULT_DEFAULT_INGRESS_HOST
            ),
        )
        args = list(filter(None, unicode_args.split(" ")))
        return parser.parse_args(args)

    def build_coredump_writer(self):
        inferior = gdb.selected_inferior()

        # Make sure thread info has been requested before doing anything else:
        gdb.execute("info threads", to_string=True)
        threads = inferior.threads()

        if not inferior or len(threads) == 0:
            print("No target!? Make sure to attach to a target first.")
            return

        # [MT]: How to figure out which thread/context the CPU is actually in?
        print("Note: for correct results, do not switch threads before invoking this command!")
        thread = gdb.selected_thread()
        if not thread:
            print("Did not find any threads!?")
            return

        if not inferior.architecture().name().startswith("arm"):
            print("This command is currently only supported for ARM targets!")
            return

        elf_fn, sections = parse_maintenance_info_sections(
            gdb.execute("maintenance info sections", to_string=True)
        )
        if elf_fn is None or sections is None:
            print(
                """Could not find file and sections.
This command requires that you use the 'load' command to load a binary/symbol (.elf) file first"""
            )
            return

        if not any(map(is_debug_info_section, sections)):
            print("WARNING: no debug info sections found!")
            print(
                "Hints: did you compile with -g or similar flags? did you inadvertently strip the binary?"
            )

        cd_writer = MemfaultCoredumpWriter()
        cd_writer.regs = get_registers_for_thread(thread)

        selected_sections = list(filter(should_capture_section, sections))
        selected_sections.append(Section(0xE000ED24, 28, "ARMv7-M System Control and ID blocks"))

        def _read_memory_and_add_section(section):
            print(
                "Capturing {} @ 0x{:x} ({} bytes)".format(section.name, section.addr, section.size)
            )
            section.data = inferior.read_memory(section.addr, section.size)
            cd_writer.add_section(section)

        map(_read_memory_and_add_section, selected_sections)

        return cd_writer


Memfault()
MemfaultCoredump()
