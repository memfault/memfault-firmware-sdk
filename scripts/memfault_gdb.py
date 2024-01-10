# -*- coding: utf-8 -*-
#
# Copyright (c) 2019, Memfault
# All rights reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are met:
#
# 1. Redistributions of source code or in binary form must reproduce
# the above copyright notice, this list of conditions and the following
# disclaimer in the documentation and/or other materials provided with the
# distribution.
#
# 2. Neither the name of Memfault nor the names of its contributors may be
# used to endorse or promote products derived from this software without
# specific prior written permission.
#
# 3. This software, with or without modification, must only be used with
# the Memfault services and integrated with the Memfault server.
#
# 4. Any software provided in binary form under this license must not be
# reverse engineered, decompiled, modified and/or disassembled.
#
# THIS SOFTWARE IS PROVIDED BY MEMFAULT "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES,
# INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY,
# NONINFRINGEMENT, AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT
# SHALL MEMFAULT OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
# SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
# PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
# OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
# WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
# ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
# THE POSSIBILITY OF SUCH DAMAGE.

import argparse
import os
import platform
import re
import sys
import traceback
import uuid
from binascii import b2a_base64
from hashlib import md5
from json import dump, dumps, load, loads
from os.path import expanduser
from struct import pack, unpack
from tempfile import TemporaryFile
from threading import Thread
from time import sleep, time

try:
    import gdb
except ImportError:
    error_str = """
    This script can only be run within gdb!
    """
    raise ImportError(error_str)  # noqa: B904  (no raise-from in Python 2.7)


# Note: not using `requests` but using the built-in http.client instead, so
# there will be no additional dependencies other than Python itself.
try:
    from httplib import HTTPConnection, HTTPSConnection
    from Queue import Queue
    from urlparse import urlparse, urlunparse
except ImportError:
    from http.client import HTTPConnection, HTTPSConnection
    from queue import Queue
    from urllib.parse import urlparse, urlunparse


MEMFAULT_DEFAULT_INGRESS_BASE_URI = "https://ingress.memfault.com"
MEMFAULT_DEFAULT_CHUNKS_BASE_URI = "https://chunks.memfault.com"
MEMFAULT_DEFAULT_API_BASE_URI = "https://api.memfault.com"


try:  # noqa: SIM105
    # In Python 3.x, raw_input was renamed to input
    # NOTE: Python 2.x also had an input() function which eval'd the input...!
    input = raw_input
except NameError:
    pass


class MemfaultConfig(object):
    ingress_uri = MEMFAULT_DEFAULT_INGRESS_BASE_URI
    api_uri = MEMFAULT_DEFAULT_API_BASE_URI
    email = None
    password = None
    organization = None
    project = None
    user_id = None

    # indirection so tests can mock this
    prompt = input

    # Added `json_path` and `input` as attributes on the config to aid unit testing:
    json_path = expanduser("~/.memfault/gdb.json")

    def can_make_project_api_request(self):
        return self.email and self.password and self.project and self.organization


MEMFAULT_CONFIG = MemfaultConfig()


def register_value_to_bytes(gdb_scalar_value, little_endian=True):
    """
    This helper is meant to be used with values that are not addressable, i.e. registers.
    If you've got an addressable value, it's probably faster/better to use the Inferior.read_memory() API.
    """

    # try to get the int representation of the value from gdb via py-value.c valpy_long/ valpy_int
    #
    # If that fails, fallback to parsing the string. Older versions of the GDB python API
    # can't do int conversions on certain types such as pointers which some registers get
    # defined as (i.e pc & sp)
    try:
        value_as_int = int(gdb_scalar_value)
    except gdb.error:
        # register string representation can look something like: "0x17d8 <__start>"
        value_as_str = str(gdb_scalar_value).split()[0]
        value_as_int = int(value_as_str, 0)

    # if the value we get back from gdb python is negative, use the signed representation instead
    # of the unsigned representation
    #

    # We represent all registers in the kMfltCoredumpBlockType_CurrentRegisters section of the coredump as 32 bit values (MfltCortexMRegs) so try
    # to convert to a 4 byte representation regardless of the width reported by the gdb-server
    fmt = "i" if value_as_int < 0 else "I"
    return pack(fmt, value_as_int)


def _get_register_value(reglist, name):
    return unpack("<I", reglist[name])[0]


def _pc_in_vector_table(register_list, exception_number, analytics_props):
    try:
        # The VTOR is implemented on armv6m and up though on chips like the Cortex-M0,
        # it will always read as 0
        vtor, _ = _read_register(0xE000ED08)
        curr_pc = _get_register_value(register_list, "pc")
        exc_handler, _ = _read_register(0x0 + vtor + (exception_number * 4))
        exc_handler &= ~0x1  # Clear thumb bit
        return exc_handler == curr_pc  # noqa: TRY300
    except Exception:
        analytics_props["pc_in_vtor_check_error"] = {"traceback": traceback.format_exc()}
        return False


def check_and_patch_reglist_for_fault(register_list, analytics_props):
    # Fault exceptions on armv6m, armv7m, & armv8m will fall between 2 and 10
    exception_number = _get_register_value(register_list, "xpsr") & 0xF
    analytics_props["exception_number"] = exception_number
    if exception_number < 2 or exception_number > 10:
        # Not in an exception so keep the register list we already have
        return

    # If we have faulted it's typically useful to get the backtrace for the code where the fault
    # originated. The integrated memfault-firmware-sdk does this by installing into the fault
    # handlers and capturing the necessary registers. For the try script, we'll try to apply the
    # same logic

    fault_start_prompt = """
We see you are trying out Memfault from a Fault handler. That's great!
For the best results and to mirror the behavior of our firmware SDK,
please run "memfault coredump" at exception entry before other code has run
in the exception handler
"""
    gdb_how_to_fault_prompt = """
It's easy to halt at exception entry by installing a breakpoint from gdb.
For example,
(gdb) breakpoint HardFault_Handler
"""

    exc_return = _get_register_value(register_list, "lr")
    if exc_return >> 28 != 0xF:
        print("{} {}".format(fault_start_prompt, gdb_how_to_fault_prompt))
        raise RuntimeError("LR no longer set to EXC_RETURN value")

    # DCRS - armv8m only - only relevant when chaining secure and non-secure exceptions
    # so pretty unlikely to be hit in a try test scenario
    if exc_return & (1 << 5) == 0:
        raise RuntimeError("DCRS exception unwinding unimplemented")

    if not _pc_in_vector_table(register_list, exception_number, analytics_props):
        analytics_props["displayed_fault_prompt"] = True
        # The pc is not at the start of the exception handler. Some firmware implementations
        # will redirect the vector table to a software vector table. If that's the case, it's
        # hard to detect programmatically, let's check in with the user
        y = MEMFAULT_CONFIG.prompt(
            "{}\nAre you currently at the start of an exception handler [y/n]?".format(
                fault_start_prompt
            )
        )
        if "Y" not in y.upper():
            print(gdb_how_to_fault_prompt)
            raise RuntimeError("User did not confirm being at beginning of exception")
    else:
        analytics_props["displayed_fault_prompt"] = False

    sp_name = "psp" if (exc_return & 0x4 != 0) else "msp"
    sp = _get_register_value(register_list, sp_name)

    # restore the register state prior to exception entry so we get an unwind
    # from where the exception actually occurred
    exception_frame = ("r0", "r1", "r2", "r3", "r12", "lr", "pc", "xpsr")
    for idx, r in enumerate(exception_frame):
        _, data = _read_register(sp + idx * 4)
        register_list[r] = data

    orig_sp_offset = 0x68 if (exc_return & (1 << 4) == 0) else 0x20
    if _get_register_value(register_list, "xpsr") & (1 << 9) != 0:
        orig_sp_offset += 0x4

    register_list["sp"] = pack("<I", sp + orig_sp_offset)


def is_debug_info_section(section):
    return section.name.startswith(".debug_")


def should_capture_section(section):
    if section.size == 0:
        return False
    # Assuming we never want to grab .text:
    if section.name == ".text":
        return False
    # Assuming we never want to grab debug info:
    if is_debug_info_section(section):
        return False
    # Sometimes these get flagged incorrectly as READONLY:
    if any(filter(lambda n: n in section.name, (".heap", ".bss", ".data", ".stack"))):
        return True
    # Only grab non-readonly stuff:
    return not section.read_only


class CoredumpArch(object):
    pass


class XtensaCoredumpArch(CoredumpArch):
    MACHINE_TYPE = 94  # XTENSA

    @property
    def num_cores(self):
        return 2

    @property
    def register_collection_list(self):
        # The order in which we collect XTENSA registers in a crash
        # fmt: off
        return (
            # We use the first word to convey what registers are collected in the coredump
            "collection_type",
            # Actual registers
            "pc", "ps", "ar0", "ar1", "ar2", "ar3", "ar4", "ar5", "ar6", "ar7", "ar8", "ar9",
            "ar10", "ar11", "ar12", "ar13", "ar14", "ar15", "ar16", "ar17", "ar18", "ar19", "ar20",
            "ar21", "ar22", "ar23", "ar24", "ar25", "ar26", "ar27", "ar28", "ar29", "ar30", "ar31",
            "ar32", "ar33", "ar34", "ar35", "ar36", "ar37", "ar38", "ar39", "ar40", "ar41", "ar42",
            "ar43", "ar44", "ar45", "ar46", "ar47", "ar48", "ar49", "ar50", "ar51", "ar52", "ar53",
            "ar54", "ar55", "ar56", "ar57", "ar58", "ar59", "ar60", "ar61", "ar62", "ar63", "sar",
            # Note: Only enabled for xtensa targets which define XCHAL_HAVE_LOOPS
            "lbeg",
            "lend",
            "lcount",
            # Special registers to collect
            "windowbase",
            "windowstart",
        )
        # fmt: on

    @property
    def alternative_register_name_dict(self):
        return {}

    def add_platform_specific_sections(self, cd_writer, inferior, analytics_props):
        pass

    def guess_ram_regions(self, elf_sections):
        # TODO: support esp32-s2 & esp8266
        # For now we just use the memory map from the esp32 for xtensa

        # Memory map:
        # https://github.com/espressif/esp-idf/blob/v3.3.1/components/soc/esp32/include/soc/soc.h#L286-L304
        regions = (
            # SOC_DRAM
            (0x3FFAE000, 0x52000),
            # SOC_RTC_DATA
            (0x50000000, 0x2000),
            # SOC_RTC_DRAM
            (0x3FF80000, 0x2000),
        )
        return regions

    def _read_registers(self, core, gdb_thread, analytics_props):
        # NOTE: The only way I could figure out to read raw registers for CPU1 was
        # to send the raw gdb command,"g" after explicitly setting the active core
        gdb.execute("mon set_core {}".format(core))
        registers = gdb.execute("maintenance packet g", to_string=True)
        registers = registers.split("received: ")[1].replace('"', "")

        # The order GDB sends xtensa registers (for esp32) in:
        #  https://github.com/espressif/xtensa-overlays/blob/master/xtensa_esp32/gdb/gdb/regformats/reg-xtensa.dat#L3-L107

        # fmt: off
        xtensa_gdb_idx_regs = (
            "pc", "ar0", "ar1", "ar2", "ar3", "ar4", "ar5", "ar6", "ar7",
            "ar8", "ar9", "ar10", "ar11", "ar12", "ar13", "ar14", "ar15", "ar16", "ar17", "ar18",
            "ar19", "ar20", "ar21", "ar22", "ar23", "ar24", "ar25", "ar26", "ar27", "ar28", "ar29",
            "ar30", "ar31", "ar32", "ar33", "ar34", "ar35", "ar36", "ar37", "ar38", "ar39", "ar40",
            "ar41", "ar42", "ar43", "ar44", "ar45", "ar46", "ar47", "ar48", "ar49", "ar50", "ar51",
            "ar52", "ar53", "ar54", "ar55", "ar56", "ar57", "ar58", "ar59", "ar60", "ar61", "ar62",
            "ar63", "lbeg", "lend", "lcount", "sar", "windowbase", "windowstart", "configid0",
            "configid1", "ps", "threadptr", "br", "scompare1", "acclo", "acchi", "m0", "m1", "m2",
            "m3", "expstate", "f64r_lo", "f64r_hi", "f64s", "f0", "f1", "f2", "f3", "f4", "f5",
            "f6", "f7", "f8", "f9", "f10", "f11", "f12", "f13", "f14", "f15", "fcr", "fsr",
        )
        # fmt: on

        # Scoop up all register values
        vals = []
        for i in range(len(xtensa_gdb_idx_regs)):
            start_idx = i * 8
            hexstr = registers[start_idx : start_idx + 8]
            vals.append(bytearray.fromhex(hexstr))

        register_list = {}
        for register_name in self.register_collection_list:
            # A "special" value we use to convey what ESP32 registers were collected
            if register_name == "collection_type":
                # A value of 0 means we collected the full set of windows. When a debugger is
                # active, some register windows may not have been spilled to the stack so we need
                # all the windows.
                register_list[register_name] = bytearray.fromhex("00000000")
                continue
            idx = xtensa_gdb_idx_regs.index(register_name)
            register_list[register_name] = vals[idx]

        return register_list

    def get_current_registers(self, gdb_thread, analytics_props):
        result = []
        try:
            for core_id in range(self.num_cores):
                result.append(self._read_registers(core_id, gdb_thread, analytics_props))
        except Exception:
            analytics_props["core_reg_collection_error"] = {"traceback": traceback.format_exc()}

        return result


class ArmCortexMCoredumpArch(CoredumpArch):
    MACHINE_TYPE = 40  # ARM

    @property
    def num_cores(self):
        return 1

    @property
    def register_collection_list(self):
        # The order in which we collect ARM registers in a crash
        return (
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
            "pc",  # 15
            "xpsr",
            "msp",
            "psp",
            "primask",
            "control",
        )

    @property
    def alternative_register_name_dict(self):
        # GDB allows the remote server to provide register names via the target description which can
        # be exchanged between client and server:
        #   https://sourceware.org/gdb/onlinedocs/gdb/Target-Description-Format.html#Target-Description-Format
        #
        # Different implementations of the gdb server use different names for these custom register sets
        # This dictionary holds known mappings between the names we will use and the ones in different GDBDebugContextFacade
        # server implementations
        #
        # NOTE: If the only difference is capitalization, that will be automagically resolved below
        return {"xpsr": ("cpsr",)}

    @staticmethod
    def _try_collect_mpu_settings():
        cpuid = 0xE000ED00
        reg_val, _ = _read_register(cpuid)
        partno = (reg_val >> 4) & 0xFFF

        cortex_m_cpuids = {
            0xC20: "M0",
            0xC21: "M1",
            0xC23: "M3",
            0xC24: "M4",
            0xC27: "M7",
            0xC60: "M0+",
        }

        if partno not in cortex_m_cpuids:
            return None

        print("Cortex-{} detected".format(cortex_m_cpuids[partno]))
        mpu_type = 0xE000ED90
        mpu_ctrl = 0xE000ED94
        mpu_rnr = 0xE000ED98
        mpu_rbar = 0xE000ED9C
        mpu_rasr = 0xE000EDA0

        result = b""
        mpu_type, mpu_type_data = _read_register(mpu_type)
        result += mpu_type_data

        mpu_ctrl, mpu_ctrl_data = _read_register(mpu_ctrl)
        result += mpu_ctrl_data

        num_regions = (mpu_type >> 8) & 0xFF
        for i in range(num_regions):
            _write_register(mpu_rnr, i)
            _, data = _read_register(mpu_rbar)
            result += data
            _, data = _read_register(mpu_rasr)
            result += data

        return result

    def add_platform_specific_sections(self, cd_writer, inferior, analytics_props):
        mem_mapped_regs = [
            ("ictr", "Interrupt Controller Type Register", 0xE000E004, 0xE000E008),
            ("systick", "ARMv7-M System Timer", 0xE000E010, 0xE000E020),
            ("scb", "ARMv7-M System Control Block", 0xE000ED00, 0xE000ED8F),
            ("scs_debug", "ARMv7-M SCS Debug Registers", 0xE000EDFC, 0xE000EE00),
            ("nvic", "ARMv7-M External Interrupt Controller", 0xE000E100, 0xE000E600),
        ]

        for mem_mapped_reg in mem_mapped_regs:
            try:
                short_name, desc, base, top = mem_mapped_reg
                section = Section(base, top - base, desc)
                section.data = inferior.read_memory(section.addr, section.size)
                cd_writer.add_section(section)
                analytics_props["{}_ok".format(short_name)] = True
            except Exception:
                analytics_props["{}_collection_error".format(short_name)] = {
                    "traceback": traceback.format_exc()
                }

        try:
            cd_writer.armv67_mpu = self._try_collect_mpu_settings()
            print("Collected MPU config")
        except Exception:
            analytics_props["mpu_collection_error"] = {"traceback": traceback.format_exc()}

    @staticmethod
    def _merge_memory_regions(regions):
        """Take a set of memory regions and merge any overlapping regions"""
        regions.sort(key=lambda x: x.addr)  # Sort the regions based on starting address
        merged_regions = []

        for region in regions:
            if not merged_regions:
                merged_regions.append(region)
            else:
                prev_region = merged_regions[-1]
                if region.addr <= prev_region.addr + prev_region.size:
                    prev_region.size = max(
                        prev_region.size, region.addr + region.size - prev_region.addr
                    )
                else:
                    merged_regions.append(region)

        return merged_regions

    def guess_ram_regions(self, elf_sections):
        capturable_elf_sections = list(filter(should_capture_section, elf_sections))

        def _is_ram(base_addr):
            # See Table B3-1 ARMv7-M address map in "ARMv7-M Architecture Reference Manual"
            return (base_addr & 0xF0000000) in (0x20000000, 0x30000000, 0x60000000, 0x80000000)

        capture_size = 1024 * 1024  # Capture up to 1MB per section
        for section in capturable_elf_sections:
            section.size = capture_size

        # merge any overlapping sections to make the core a little more efficient
        capturable_elf_sections = self._merge_memory_regions(capturable_elf_sections)
        filtered_addrs = set(filter(_is_ram, (s.addr for s in capturable_elf_sections)))
        # Capture up to 1MB for each region
        return [(addr, capture_size) for addr in filtered_addrs]

    def get_current_registers(self, gdb_thread, analytics_props):
        gdb_thread.switch()

        # GDB Doesn't have a convenient way to know all of the registers in Python, so this is the
        # best way. Call this, rip out the first element in each row...that's the register name

        #
        # NOTE: Using the 'all-registers' command below, because on some
        # versions of gdb "msp, psp, etc" are not considered part of them core
        # set. This will also dump all the fpu registers which we don't collect
        # but thats fine. 'info all-registers' is the preferred command, where
        # 'info reg [all]' is arch-specific, see:
        # https://sourceware.org/gdb/onlinedocs/gdb/Registers.html
        try:
            info_reg_all_list = gdb.execute("info all-registers", to_string=True)
        except gdb.error:
            # Some versions of gdb don't support 'all' and return an error, fall
            # back to 'info reg'
            info_reg_all_list = gdb.execute("info reg", to_string=True)

        return (lookup_registers_from_list(self, info_reg_all_list, analytics_props),)


# TODO: De-duplicate with code from rtos_register_stacking.py
def concat_registers_dict_to_bytes(arch, regs):
    result = b""
    for reg_name in arch.register_collection_list:
        assert reg_name.lower() == reg_name
        if reg_name not in regs:
            result += b"\x00\x00\x00\x00"
            continue
        result += regs[reg_name]
    return result


def _is_expected_reg(arch, reg_name):
    return reg_name in arch.register_collection_list


def _add_reg_collection_error_analytic(arch, analytics_props, reg_name, error):
    if not _is_expected_reg(arch, reg_name):
        return

    reg_collection_error = "reg_collection_error"
    if reg_collection_error not in analytics_props:
        analytics_props[reg_collection_error] = {}

    analytics_props[reg_collection_error][reg_name] = error


def _try_read_register(arch, frame, lookup_name, register_list, analytics_props, result_name=None):
    # `info reg` will print all registers, even though they are not part of the core.
    # If that's the case, doing frame.read_register() will raise a gdb.error.
    try:
        if hasattr(frame, "read_register"):
            value = frame.read_register(lookup_name)
        else:
            # GCC <= 4.9 doesn't have the read_register API
            value = gdb.parse_and_eval("${}".format(lookup_name))
        value_str = str(value)
        if value_str != "<unavailable>":
            name_to_use = lookup_name if result_name is None else result_name
            register_list[name_to_use] = register_value_to_bytes(value)
        else:
            _add_reg_collection_error_analytic(
                arch, analytics_props, lookup_name, "<unavailable> value"
            )
    except Exception:
        _add_reg_collection_error_analytic(
            arch, analytics_props, lookup_name, traceback.format_exc()
        )


def lookup_registers_from_list(arch, info_reg_all_list, analytics_props):
    frame = gdb.newest_frame()
    frame.select()

    register_names = []
    for reg_row in info_reg_all_list.strip().split("\n"):
        name = reg_row.split()[0]
        register_names.append(name)

    def _search_list_for_alt_name(reg, found_registers):
        # first see if it's just case getting in the way i.e 'CONTROL' instead of 'control'. We
        # need to preserve case when we actually issue the read so the gdb API works correctly
        for found_reg in found_registers:
            if found_reg.lower() == reg:
                return found_reg

        alt_reg_names = arch.alternative_register_name_dict.get(reg, [])
        for alt_reg_name in alt_reg_names:
            if alt_reg_name in found_registers:
                return alt_reg_name

        return None

    alt_reg_names = []
    for expected_reg in arch.register_collection_list:
        if expected_reg in register_names:
            continue

        alt_reg_name = _search_list_for_alt_name(expected_reg, register_names)
        if alt_reg_name:
            alt_reg_names.append((alt_reg_name, expected_reg))
            continue

        _add_reg_collection_error_analytic(
            arch, analytics_props, expected_reg, "Not found in register set"
        )

    # Iterate over all register names and pull the value out of the frame
    register_list = {}

    # Remove register_names we don't care about before actually looking up values
    register_names = filter(lambda n: _is_expected_reg(arch, n), register_names)

    for reg_name in register_names:
        _try_read_register(arch, frame, reg_name, register_list, analytics_props)

    for lookup_reg_name, result_reg_name in alt_reg_names:
        _try_read_register(
            arch, frame, lookup_reg_name, register_list, analytics_props, result_reg_name
        )

    # if we can't patch the registers, we'll just fallback to the active state
    try:
        check_and_patch_reglist_for_fault(register_list, analytics_props)
    except Exception:
        analytics_props["fault_register_recover_error"] = {"traceback": traceback.format_exc()}

    return register_list


# TODO: De-duplicate with code from core_convert.py
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
    ARM_V7M_MPU = 9
    SOFTWARE_VERSION = 10
    SOFTWARE_TYPE = 11


class MemfaultCoredumpWriter(object):
    def __init__(self, arch, device_serial, software_type, software_version, hardware_revision):
        self.device_serial = device_serial
        self.software_version = software_version
        self.software_type = software_type
        self.hardware_revision = hardware_revision
        self.trace_reason = 5  # Debugger Halted
        self.regs = {}
        self.sections = []
        self.armv67_mpu = None
        self.arch = arch

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

        def _write_block(type, payload, address=0):  # noqa: A002
            write(pack(MEMFAULT_COREDUMP_BLOCK_HEADER_FMT, type, address, len(payload)))
            write(payload)

        for core_regs in self.regs:
            _write_block(
                MemfaultCoredumpBlockType.CURRENT_REGISTERS,
                concat_registers_dict_to_bytes(self.arch, core_regs),
            )

        _write_block(MemfaultCoredumpBlockType.DEVICE_SERIAL, self.device_serial.encode("utf8"))
        _write_block(
            MemfaultCoredumpBlockType.SOFTWARE_VERSION, self.software_version.encode("utf8")
        )
        _write_block(MemfaultCoredumpBlockType.SOFTWARE_TYPE, self.software_type.encode("utf8"))
        _write_block(
            MemfaultCoredumpBlockType.HARDWARE_REVISION, self.hardware_revision.encode("utf8")
        )
        _write_block(MemfaultCoredumpBlockType.MACHINE_TYPE, pack("<I", self.arch.MACHINE_TYPE))
        _write_block(MemfaultCoredumpBlockType.TRACE_REASON, pack("<I", self.trace_reason))

        # Record the time in a fake memory region. By doing this we guarantee a unique coredump
        # will be generated each time "memfault coredump" is run. This makes it easier to discover
        # the de-duplication logic get run by the Memfault backend
        time_data = pack("<I", int(time()))
        _write_block(MemfaultCoredumpBlockType.MEMORY_REGION, time_data, 0xFFFFFFFC)

        for section in self.sections:
            _write_block(MemfaultCoredumpBlockType.MEMORY_REGION, section.data, section.addr)
        if self.armv67_mpu:
            _write_block(MemfaultCoredumpBlockType.ARM_V7M_MPU, self.armv67_mpu)

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

    def __str__(self):
        return "Section(addr=0x%x, size=0x%x, name='%s', read_only=%s, data_len=%d)" % (
            self.addr,
            self.size,
            self.name,
            self.read_only,
            len(self.data),
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


def read_memory_until_error(inferior, start, size, read_size=4 * 1024):
    data = b""
    end = start + size
    try:
        for addr in range(start, end, read_size):
            data += bytes(inferior.read_memory(addr, min(read_size, end - addr)))
    except Exception as e:
        # Catch gdbserver read exceptions -- not sure what exception classes can get raised here
        print(e)
    return data


def _create_http_connection(base_uri):
    url = urlparse(base_uri)
    if url.hostname is None:
        raise RuntimeError("Invalid base URI, must be http(s)://hostname")
    if url.scheme == "http":
        conn_class = HTTPConnection
        default_port = 80
    else:
        conn_class = HTTPSConnection
        default_port = 443
    port = url.port if url.port else default_port
    return conn_class(url.hostname, port=port)


def _http(method, base_uri, path, headers=None, body=None):
    if headers is None:
        headers = {}
    conn = _create_http_connection(base_uri)
    # Convert to a string/bytes object so 'Content-Length' is set appropriately
    # Python 2.7 uses this by default but 3.6 & up were using 'chunked'
    if sys.version_info.major >= 3 and hasattr(body, "read"):
        body = body.read()

    conn.request(method, path, body=body, headers=headers)
    response = conn.getresponse()
    status = response.status
    reason = response.reason
    body = response.read()
    try:
        json_body = loads(body)
    except Exception:
        json_body = None
    conn.close()
    return status, reason, json_body


def add_basic_auth(user, password, headers=None):
    headers = dict(headers) if headers else {}
    headers["Authorization"] = "Basic {}".format(
        b2a_base64("{}:{}".format(user, password).encode("utf8")).decode("ascii").strip()
    )
    return headers


class HttpApiError(Exception):
    def __init__(self, status, reason):
        super(Exception, self).__init__("{} (HTTP {})".format(reason, status))


def _check_http_response(status, reason):
    if status < 200 or status >= 300:
        raise HttpApiError(status, reason)


def _http_api(config, method, path, headers=None, body=None, should_raise=False):
    headers = add_basic_auth(config.email, config.password, headers=headers)
    status, reason, body = _http(method, config.api_uri, path, headers, body)
    if should_raise:
        _check_http_response(status, reason)
    return status, reason, body


def http_post_coredump(coredump_file, project_key, ingress_uri):
    headers = {"Content-Type": "application/octet-stream", "Memfault-Project-Key": project_key}
    status, reason, _ = _http(
        "POST", ingress_uri, "/api/v0/upload/coredump", headers=headers, body=coredump_file
    )
    return status, reason


def http_post_chunk(chunk_data_file, project_key, chunks_uri, device_serial):
    headers = {"Content-Type": "application/octet-stream", "Memfault-Project-Key": project_key}
    status, reason, _ = _http(
        "POST",
        chunks_uri,
        "/api/v0/chunks/{}".format(device_serial),
        headers=headers,
        body=chunk_data_file,
    )
    return status, reason


def http_get_auth_me(api_uri, email, password):
    headers = add_basic_auth(email, password)
    return _http("GET", api_uri, "/auth/me", headers=headers)


def http_get_prepared_url(config):
    _, _, body = _http_api(
        config,
        "POST",
        "/api/v0/organizations/{organization}/projects/{project}/upload".format(
            organization=config.organization,
            project=config.project,
        ),
        headers={"Accept": "application/json"},
        should_raise=True,
    )
    data = body["data"]
    return data["token"], data["upload_url"]


def http_upload_file(config, file_readable):
    token, upload_url = http_get_prepared_url(config)
    url_parts = urlparse(upload_url)
    base_uri = urlunparse((url_parts[0], url_parts[1], "", "", "", ""))
    path = urlunparse(("", "", url_parts[2], url_parts[3], url_parts[4], url_parts[5]))
    status, reason, _ = _http(
        "PUT",
        base_uri,
        path,
        # NB: Prepared upload API does not expect Content-Type to be set so
        # we need to exclude the Content-Type or set it to an empty string
        headers={"Content-Type": ""},
        body=file_readable,
    )
    _check_http_response(status, reason)
    return token


def http_upload_symbol_file(config, artifact_readable, software_type, software_version):
    token = http_upload_file(config, artifact_readable)
    _http_api(
        config,
        "POST",
        "/api/v0/organizations/{organization}/projects/{project}/symbols".format(
            organization=config.organization,
            project=config.project,
        ),
        headers={"Content-Type": "application/json", "Accept": "application/json"},
        body=dumps({
            "file": {"token": token, "name": "symbols.elf"},
            "software_version": {
                "version": software_version,
                "software_type": software_type,
            },
        }),
        should_raise=True,
    )


def http_get_software_version(config, software_type, software_version):
    software_version_url = "/api/v0/organizations/{organization}/projects/{project}/software_types/{software_type}/software_versions/{software_version}".format(
        organization=config.organization,
        project=config.project,
        software_type=software_type,
        software_version=software_version,
    )

    status, _, body = _http_api(config, "GET", software_version_url)
    if status < 200 or status >= 300:
        return None
    return body["data"]


def http_get_project_key(config):
    status, reason, body = _http_api(
        config,
        "GET",
        "/api/v0/organizations/{organization}/projects/{project}/api_key".format(
            organization=config.organization, project=config.project
        ),
        headers={"Accept": "application/json"},
    )
    if status < 200 or status >= 300:
        return None, (status, reason)
    return body["data"]["api_key"], None


def get_file_hash(fn):
    with open(fn, "rb") as f:
        return md5(f.read()).hexdigest()


def has_uploaded_symbols(config, software_type, software_version):
    software_version_obj = http_get_software_version(config, software_type, software_version)
    if not software_version_obj:
        return False
    symbol_file = software_version_obj.get("symbol_file")
    if not symbol_file:
        return False
    return bool(symbol_file.get("downloadable"))


def upload_symbols_if_needed(config, elf_fn, software_type, software_version):
    has_symbols = has_uploaded_symbols(config, software_type, software_version)
    if has_symbols:
        print("Symbols have already been uploaded, skipping!")
        return
    if not has_symbols:
        print("Uploading symbols...")
        with open(elf_fn, "rb") as elf_f:
            try:
                http_upload_symbol_file(config, elf_f, software_type, software_version)
                # NOTE: upload is processed asynchronously. Give the symbol file
                # a little time to be processed. In the future, we could poll here
                # for completion
                sleep(0.3)
                print("Done!")
            except HttpApiError as e:
                print("Failed to upload symbols: {}".format(e))


# TODO: Duped from mflt.tools/gdb_memfault.py
class MemfaultGdbArgumentParseError(Exception):
    pass


class MemfaultGdbArgumentParser(argparse.ArgumentParser):
    def exit(self, status=0, message=None):  # noqa: A003
        if message:
            self._print_message(message)
        # Don't call sys.exit()
        raise MemfaultGdbArgumentParseError


def populate_config_args_and_parse_args(parser, unicode_args, config):
    parser.add_argument(
        "--email",
        help="The username (email address) of the user to use",
        default=MEMFAULT_CONFIG.email,
    )
    parser.add_argument(
        "--password",
        help="The user API key or password of the user to use",
        default=MEMFAULT_CONFIG.password,
    )
    parser.add_argument(
        "--organization",
        "-o",
        help="Default organization (slug) to use",
        default=MEMFAULT_CONFIG.organization,
    )
    parser.add_argument(
        "--project", "-p", help="Default project (slug) to use", default=MEMFAULT_CONFIG.project
    )
    parser.add_argument(
        "--ingress-uri",
        default=MEMFAULT_CONFIG.ingress_uri,
        help="Default ingress base URI to use (default: {})".format(MEMFAULT_CONFIG.ingress_uri),
    )
    parser.add_argument(
        "--api-uri",
        help="Default API base URI to use (default: {})".format(MEMFAULT_CONFIG.api_uri),
        default=MEMFAULT_CONFIG.api_uri,
    )
    args = list(filter(None, unicode_args.split(" ")))
    parsed_args = parser.parse_args(args)

    config.ingress_uri = parsed_args.ingress_uri
    config.api_uri = parsed_args.api_uri
    config.email = parsed_args.email
    config.password = parsed_args.password
    config.organization = parsed_args.organization
    config.project = parsed_args.project

    # If the user overrides the email, we need to re-auth to get the new user ID
    if parsed_args.email == MEMFAULT_CONFIG.email:
        config.user_id = MEMFAULT_CONFIG.user_id
    else:
        _, _, json_body = http_get_auth_me(config.api_uri, config.email, config.password)
        config.user_id = json_body["id"]

    return parsed_args


class MemfaultGdbCommand(gdb.Command):
    GDB_CMD = "memfault override_me"

    def __init__(self, *args, **kwargs):
        super(MemfaultGdbCommand, self).__init__(self.GDB_CMD, gdb.COMMAND_USER, *args, **kwargs)

    def invoke(self, arg, from_tty):
        # Work-around for GDB-py not printing out the backtrace upon exceptions
        analytics_props = {}
        start_time = time()
        try:
            self._invoke(arg, from_tty, analytics_props, MEMFAULT_CONFIG)
        except Exception:
            print(traceback.format_exc())
            ANALYTICS.track("Exception", {"traceback": traceback.format_exc(), "args": arg})
            raise  # Important, otherwise unit test failures may go undetected!
        analytics_props["duration_ms"] = int((time() - start_time) * 1000)
        ANALYTICS.track("Command {}".format(self.GDB_CMD), analytics_props, MEMFAULT_CONFIG.user_id)

    def _invoke(self, unicode_args, from_tty, analytics_props, config):
        pass


class Memfault(MemfaultGdbCommand):
    """Memfault GDB commands"""

    GDB_CMD = "memfault"

    def __init__(self):
        super(Memfault, self).__init__(gdb.COMPLETE_NONE, prefix=True)

    def _invoke(self, unicode_args, from_tty, analytics_props, config):
        gdb.execute("help memfault")


def settings_load():
    try:
        with open(MEMFAULT_CONFIG.json_path, "rb") as f:
            return load(f)
    except Exception:
        return {}


def settings_save(settings):
    try:  # noqa: SIM105
        # exist_ok does not exist yet in Python 2.7!
        os.makedirs(os.path.dirname(MEMFAULT_CONFIG.json_path))
    except OSError:
        pass
    with open(MEMFAULT_CONFIG.json_path, "w") as f:
        dump(settings, f, sort_keys=True)


def _infer_issues_html_url(ingress_uri, config):
    if "dev." in ingress_uri or "localhost" in ingress_uri:
        html_base_uri = ingress_uri
    elif "ingress.try" in ingress_uri:
        html_base_uri = ingress_uri.replace("ingress.try.", "try.")
    elif "ingress." in ingress_uri:
        html_base_uri = ingress_uri.replace("ingress.", "app.")
    else:
        return None
    return "{}/organizations/{}/projects/{}/issues?live".format(
        html_base_uri, config.organization, config.project
    )


def _read_register(address):
    reg_data = gdb.selected_inferior().read_memory(address, 4)
    reg_val = unpack("<I", reg_data)[0]
    return reg_val, bytes(reg_data)


def _write_register(address, value):
    reg_val = pack("<I", value)
    gdb.selected_inferior().write_memory(address, reg_val)


def _post_chunk_data(chunk_data, **kwargs):
    with TemporaryFile() as f_out:
        f_out.write(chunk_data)
        f_out.seek(0)
        return http_post_chunk(f_out, **kwargs)


class GdbMemfaultPostChunkBreakpoint(gdb.Breakpoint):
    def __init__(self, project_key, chunks_uri, verbose, device_serial, *args, **kwargs):
        gdb.Breakpoint.__init__(self, *args, **kwargs)
        self.project_key = project_key
        self.chunks_uri = chunks_uri
        self.verbose = verbose
        self.chunk_buf_size_tuple = None
        self.device_serial = device_serial
        print("Memfault GDB Chunk Handler Installed")

    def _determine_param_names(self):
        if self.chunk_buf_size_tuple is not None:
            return self.chunk_buf_size_tuple

        frame = gdb.newest_frame()

        chunk_buf_param = None
        chunk_buf_size_param = None
        for symbol in frame.block():
            if not symbol.is_argument:
                continue
            symbol_type = symbol.type.strip_typedefs().code
            if symbol_type == gdb.TYPE_CODE_PTR:
                chunk_buf_param = symbol.name
            if symbol_type == gdb.TYPE_CODE_INT:
                chunk_buf_size_param = symbol.name

        if chunk_buf_param is None or chunk_buf_size_param is None:
            return None

        print(
            "Params Found! Chunk buffer: '{}' Length: '{}'".format(
                chunk_buf_param, chunk_buf_size_param
            )
        )
        self.chunk_buf_size_tuple = (chunk_buf_param, chunk_buf_size_param)
        return self.chunk_buf_size_tuple

    def stop(self):
        params = self._determine_param_names()
        if params is None:
            print(
                """ERROR: Could not determine names of parameters holding chunk buffer and size
                Disabling Memfault GDB Chunk Handler & Halting
                """
            )
            self.enabled = False
            return True

        buf_param, buf_size_param = params

        # NB: Cast packet_buffer pointer to a uint32_t because older versions of GDB don't perform
        # the address conversion correctly
        packet_buffer_addr = int(gdb.parse_and_eval("(uint32_t){}".format(buf_param)))
        packet_buffer_len = gdb.parse_and_eval(buf_size_param)
        chunk_data = gdb.selected_inferior().read_memory(packet_buffer_addr, packet_buffer_len)

        status, reason = _post_chunk_data(
            chunk_data,
            project_key=self.project_key,
            chunks_uri=self.chunks_uri,
            device_serial=self.device_serial,
        )
        if status != 202:
            print("Chunk Post Failed with Http Status Code {}".format(status))
            print("Reason: {}".format(reason))
            print("ERROR: Disabling Memfault GDB Chunk Handler and Halting")
            self.enabled = False
            return True

        if self.verbose:
            print("Successfully posted {} bytes of chunk data".format(len(chunk_data)))

        return False


class MemfaultPostChunk(MemfaultGdbCommand):
    """Installs a hook which sends chunks to the Memfault cloud chunk endpoint automagically.
    See https://mflt.io/posting-chunks-with-gdb for more details.
    """

    GDB_CMD = "memfault install_chunk_handler"
    USER_TRANSPORT_SEND_CHUNK_HANDLER = [  # noqa: RUF012
        "memfault_data_export_chunk",
        "user_transport_send_chunk_data",
    ]
    DEFAULT_CHUNK_DEVICE_SERIAL = "GDB_TESTSERIAL"
    FILENAME_FROM_INFO_FUNC_PATTERN = re.compile(r"File\s+([^\s]+):")

    def _invoke(self, unicode_args, from_tty, analytics_props, config):
        try:
            parsed_args = self.parse_args(unicode_args)
        except MemfaultGdbArgumentParseError:
            return

        if isinstance(parsed_args.chunk_handler_func, list):
            chunk_handler_funcs = parsed_args.chunk_handler_func
        else:
            chunk_handler_funcs = [parsed_args.chunk_handler_func]

        for chunk_handler_func in chunk_handler_funcs:
            output = gdb.execute("info functions {}$".format(chunk_handler_func), to_string=True)
            lines = output.splitlines()[1:]
            if len(lines) != 0:
                break
        else:
            print(
                """Chunk handler function '{}' does not exist.

            A custom handler location can be specified with the --chunk-handler-func argument.
            """.format(chunk_handler_func)
            )
            return

        # It's possible to have multiple function declarations when the default weak symbol in
        # components/demo is overriden. This is because the gnu linker does not edit the DWARF DIE
        # generated by a compilation unit (https://sourceware.org/bugzilla/show_bug.cgi?id=25561)
        # To workaround this limitation, we'll use the <file>:<function> syntax to ensure the
        # breakpoint is set for the correct function.
        chunk_handler_func_filename = None
        for line in lines:
            file_match = re.search(self.FILENAME_FROM_INFO_FUNC_PATTERN, line)
            if not file_match:
                continue
            filename = file_match.groups()[0]
            if chunk_handler_func_filename is None or "components/demo" not in filename:
                chunk_handler_func_filename = filename

        spec_prefix = (
            ""
            if chunk_handler_func_filename is None
            else "'{}':".format(chunk_handler_func_filename)
        )

        # We always delete any pre-existing breakpoints on the function. This way
        # a re-execution of the command can always be used to re-init the setup
        #
        # NB: Wrapped in a try catch because older versions of gdb.breakpoints() eventually return None
        # instead of raising StopIteration
        try:
            for breakpoint in gdb.breakpoints():  # noqa: A001
                if chunk_handler_func in breakpoint.location:
                    print("Deleting breakpoint for '{}'".format(chunk_handler_func))
                    breakpoint.delete()
        except TypeError:
            pass
        print("Installing Memfault GDB Chunk Handler for function '{}'".format(chunk_handler_func))
        GdbMemfaultPostChunkBreakpoint(
            spec=spec_prefix + chunk_handler_func,
            project_key=parsed_args.project_key,
            chunks_uri=parsed_args.chunks_uri,
            verbose=parsed_args.verbose,
            device_serial=parsed_args.device_serial,
            internal=True,
        )

    def parse_args(self, unicode_args):
        parser = MemfaultGdbArgumentParser(description=MemfaultPostChunk.__doc__)
        parser.add_argument(
            "--project-key", "-pk", help="Memfault Project Key", required=True, default=None
        )
        parser.add_argument(
            "--chunk-handler-func",
            "-ch",
            help=(
                "Name of function that handles sending Memfault Chunks."
                "(default: search for one of {})".format(self.USER_TRANSPORT_SEND_CHUNK_HANDLER)
            ),
            default=self.USER_TRANSPORT_SEND_CHUNK_HANDLER,
        )
        parser.add_argument(
            "--chunks-uri",
            help="Default chunks base URI to use (default: {})".format(
                MEMFAULT_DEFAULT_CHUNKS_BASE_URI
            ),
            default=MEMFAULT_DEFAULT_CHUNKS_BASE_URI,
        )
        parser.add_argument(
            "--device-serial",
            "-ds",
            help="Device Serial to post chunks as (default: {})".format(
                self.DEFAULT_CHUNK_DEVICE_SERIAL
            ),
            default=self.DEFAULT_CHUNK_DEVICE_SERIAL,
        )
        parser.add_argument(
            "--verbose",
            help="Prints a summary every time a chunk is posted instead of just on failures",
            action="store_true",
        )

        args = list(filter(None, unicode_args.split(" ")))
        return parser.parse_args(args)


class MemfaultCoredump(MemfaultGdbCommand):
    """Captures a coredump from the target and uploads it to Memfault for analysis"""

    ALPHANUM_SLUG_DOTS_COLON_REGEX = r"^[-a-zA-Z0-9_\.\+:]+$"
    ALPHANUM_SLUG_DOTS_COLON_SPACES_PARENS_SLASH_COMMA_REGEX = r"^[-a-zA-Z0-9_\.\+: \(\)\[\]/,]+$"
    DEFAULT_CORE_DUMP_HARDWARE_REVISION = "DEVBOARD"
    DEFAULT_CORE_DUMP_SERIAL_NUMBER = "DEMOSERIALNUMBER"
    DEFAULT_CORE_DUMP_SOFTWARE_TYPE = "main"
    DEFAULT_CORE_DUMP_SOFTWARE_VERSION = "1.0.0"
    GDB_CMD = "memfault coredump"

    def _check_permission(self, analytics_props):
        settings = settings_load()
        key = "coredump.allow"
        allowed = settings.get(key, False)
        if allowed:
            analytics_props["permission"] = "accepted-stored"
            return True

        y = MEMFAULT_CONFIG.prompt(
            """
You are about to capture a coredump from the attached target.
This means that memory contents of the target will be captured
and sent to Memfault's web server for analysis. The currently
loaded binary file (.elf) will be sent as well, because it
contains debugging information (symbols) that are needed for
Memfault's analysis to work correctly.

Memfault will never share your data, coredumps, binary files (.elf)
or other proprietary information with other companies or anyone else.

Proceed? [y/n]
"""
        )  # This last newline is important! If it's not here, the last line is not shown on Windows!
        if "Y" not in y.upper():
            print("Aborting...")
            analytics_props["user_input"] = y
            analytics_props["permission"] = "rejected"
            return False
        analytics_props["permission"] = "accepted"

        settings[key] = True
        settings_save(settings)
        return True

    def _invoke(self, unicode_args, from_tty, analytics_props, config):
        if not self._check_permission(analytics_props):
            return

        try:
            parsed_args = self.parse_args(unicode_args, config)
        except MemfaultGdbArgumentParseError:
            return

        can_make_project_api_request = config.can_make_project_api_request()

        analytics_props["has_project_key"] = bool(parsed_args.project_key)
        analytics_props["can_make_project_api_request"] = bool(can_make_project_api_request)
        analytics_props["regions"] = len(parsed_args.region) if parsed_args.region else 0
        analytics_props["no_symbols"] = bool(parsed_args.no_symbols)
        analytics_props["api_uri"] = config.api_uri
        analytics_props["ingress_uri"] = config.ingress_uri

        if not can_make_project_api_request and not parsed_args.project_key:
            print(
                "Cannot post coredump. Please specify either --project-key or"
                " use `memfault login` and specify the project and organization.\n"
                "See `memfault login --help` for more information"
            )
            ANALYTICS.error("Missing login or key")
            return

        cd_writer, elf_fn = self.build_coredump_writer(parsed_args, analytics_props)
        if not cd_writer:
            return

        elf_hash = get_file_hash(elf_fn)

        # TODO: try calling memfault_platform_get_device_info() and use returned info if present
        # Populate the version based on hash of the .elf for now:
        software_version = cd_writer.software_version + "-md5+{}".format(elf_hash[:8])
        cd_writer.software_version = software_version

        if not parsed_args.no_symbols:
            if can_make_project_api_request:
                upload_symbols_if_needed(config, elf_fn, cd_writer.software_type, software_version)
            else:
                print("Skipping symbols upload because not logged in. Hint: use `memfault login`")

        if parsed_args.project_key:
            project_key = parsed_args.project_key
        else:
            assert can_make_project_api_request
            project_key, status_and_reason = http_get_project_key(config)
            if not project_key:
                error = "Failed to get Project Key: {}".format(status_and_reason)
                analytics_props["error"] = error
                ANALYTICS.error(error)
                print(error)
                return

        with TemporaryFile() as f_out:
            cd_writer.write(f_out)
            f_out.seek(0)
            status, reason = http_post_coredump(f_out, project_key, parsed_args.ingress_uri)

        analytics_props["http_status"] = status

        if status == 409:
            print("Coredump already exists!")
        elif status >= 200 and status < 300:
            print("Coredump uploaded successfully!")
            print("Once it has been processed, it will appear here:")
            # TODO: Print direct link to trace
            # https://memfault.myjetbrains.com/youtrack/issue/MFLT-461
            print(_infer_issues_html_url(parsed_args.ingress_uri, config))
        else:
            print("Error occurred... HTTP Status {} {}".format(status, reason))

    def parse_args(self, unicode_args, config):
        parser = MemfaultGdbArgumentParser(description=MemfaultCoredump.__doc__)
        parser.add_argument(
            "source",
            help=(
                "Source of coredump: 'live' (default) or 'storage' (target's live RAM or stored"
                " coredump)"
            ),
            default="live",
            choices=["live", "storage"],
            nargs="?",
        )
        parser.add_argument("--project-key", "-pk", help="Project Key", default=None)
        parser.add_argument(
            "--no-symbols",
            "-ns",
            help="Do not upload symbol (.elf) automatically if missing",
            default=False,
            action="store_true",
        )

        def _auto_int(x):
            try:
                return int(x, 0)
            except ValueError:
                return int(gdb.parse_and_eval(x))

        parser.add_argument(
            "--region",
            "-r",
            help=(
                "Specify memory region and size in bytes to capture. This option can be passed"
                " multiple times. Example: `-r 0x20000000 1024 0x80000000 512` will capture 1024"
                " bytes starting at 0x20000000. When no regions are passed, the command will"
                " attempt to infer what to capture based on the sections in the loaded .elf."
            ),
            type=_auto_int,
            nargs=2,
            action="append",
        )

        def _character_check(regex, name):
            def _check_inner(input):
                pattern = re.compile(regex)
                if not pattern.match(input):
                    raise argparse.ArgumentTypeError(
                        "Invalid characters in {}: {}.".format(name, input)
                    )

                return input

            return _check_inner

        parser.add_argument(
            "--device-serial",
            type=_character_check(self.ALPHANUM_SLUG_DOTS_COLON_REGEX, "device serial"),
            help=(
                "Overrides the device serial that will be reported in the core dump. (default: {})".format(
                    self.DEFAULT_CORE_DUMP_SERIAL_NUMBER
                )
            ),
            default=self.DEFAULT_CORE_DUMP_SERIAL_NUMBER,
        )
        parser.add_argument(
            "--software-type",
            type=_character_check(self.ALPHANUM_SLUG_DOTS_COLON_REGEX, "software type"),
            help=(
                "Overrides the software type that will be reported in the core dump. (default: {})".format(
                    self.DEFAULT_CORE_DUMP_SOFTWARE_TYPE
                )
            ),
            default=self.DEFAULT_CORE_DUMP_SOFTWARE_TYPE,
        )
        parser.add_argument(
            "--software-version",
            type=_character_check(
                self.ALPHANUM_SLUG_DOTS_COLON_SPACES_PARENS_SLASH_COMMA_REGEX, "software version"
            ),
            help=(
                "Overrides the software version that will be reported in the core dump."
                " (default: {})".format(self.DEFAULT_CORE_DUMP_SOFTWARE_VERSION)
            ),
            default=self.DEFAULT_CORE_DUMP_SOFTWARE_VERSION,
        )
        parser.add_argument(
            "--hardware-revision",
            type=_character_check(self.ALPHANUM_SLUG_DOTS_COLON_REGEX, "hardware revision"),
            help=(
                "Overrides the hardware revision that will be reported in the core dump."
                " (default: {})".format(self.DEFAULT_CORE_DUMP_HARDWARE_REVISION)
            ),
            default=self.DEFAULT_CORE_DUMP_HARDWARE_REVISION,
        )

        return populate_config_args_and_parse_args(parser, unicode_args, config)

    @staticmethod
    def _get_arch(current_arch, analytics_props):
        if "arm" in current_arch:
            return ArmCortexMCoredumpArch()
        if "xtensa" in current_arch:
            target = gdb.execute("monitor target current", to_string=True)
            if "esp32" in target:
                return XtensaCoredumpArch()
            analytics_props["xtensa_target"] = target
        return None

    def build_coredump_writer(self, parsed_args, analytics_props):
        # inferior.architecture() is a relatively new API, so let's use "show arch" instead:
        show_arch_output = gdb.execute("show arch", to_string=True).lower()
        current_arch_matches = re.search("currently ([^)]+)", show_arch_output)
        if current_arch_matches:
            # Using groups() here instead of fn_match[1] for python2.x compatibility
            current_arch = current_arch_matches.groups()[0]
            # Should tell us about different arm flavors:
            analytics_props["arch"] = current_arch
            arch = self._get_arch(current_arch, analytics_props)
            if arch is None:
                print("This command is currently only supported for ARM and XTENSA targets!")
                analytics_props["error"] = "Unsupported architecture"
                return None, None
        else:
            error = "show arch has unexpected output"
            analytics_props["error"] = error
            ANALYTICS.error(error, info=show_arch_output)
            return None, None

        inferior = gdb.selected_inferior()

        # Make sure thread info has been requested before doing anything else:
        gdb.execute("info threads", to_string=True)
        threads = inferior.threads()

        if not inferior:
            print("No target!? Make sure to attach to a target first.")
            analytics_props["error"] = "No target"
            return None, None

        if len(threads) == 0:
            print("No target!? Make sure to attach to a target first.")
            analytics_props["error"] = "No threads"
            return None, None

        print("One moment please, capturing memory...")

        # [MT]: How to figure out which thread/context the CPU is actually in?
        # JLink nor OpenOCD do not seem to be setting thread.is_running() correctly.
        # print("Note: for correct results, do not switch threads!")
        try:
            thread = gdb.selected_thread()
        except SystemError as e:
            # Exception can be raised if selected thread has dissappeared in the mean time
            # SystemError: could not find gdb thread object
            print(e)
            thread = None

        if not thread:
            print("Did not find any threads!?")
            analytics_props["error"] = "Failed to activate thread"
            return None, None

        info_sections_output = gdb.execute("maintenance info sections", to_string=True)
        elf_fn, sections = parse_maintenance_info_sections(info_sections_output)
        if elf_fn is None or sections is None:
            print(
                """Could not find file and sections.
This command requires that you use the 'load' command to load a binary/symbol (.elf) file first"""
            )
            error = "Failed to parse sections"
            analytics_props["error"] = error
            ANALYTICS.error(error, info=info_sections_output)
            return None, None

        has_debug_info = any(map(is_debug_info_section, sections))
        analytics_props["has_debug_info"] = has_debug_info
        if not has_debug_info:
            print("WARNING: no debug info sections found!")
            print(
                "Hints: did you compile with -g or similar flags? did you inadvertently strip the"
                " binary?"
            )

        cd_writer = MemfaultCoredumpWriter(
            arch,
            parsed_args.device_serial,
            parsed_args.software_type,
            parsed_args.software_version,
            parsed_args.hardware_revision,
        )
        cd_writer.regs = arch.get_current_registers(thread, analytics_props)

        arch.add_platform_specific_sections(cd_writer, inferior, analytics_props)

        regions = parsed_args.region
        if regions is None:
            print(
                "No capturing regions were specified; will default to capturing the first 1MB for"
                " each used RAM address range."
            )
            print(
                "Tip: optionally, you can use `--region <addr> <size>` to manually specify"
                " capturing regions and increase capturing speed."
            )

            regions = arch.guess_ram_regions(sections)

        for addr, size in regions:
            print("Capturing RAM @ 0x{:x} ({} bytes)...".format(addr, size))
            data = read_memory_until_error(inferior, addr, size)
            section = Section(addr, len(data), "RAM")
            section.data = data
            cd_writer.add_section(section)
            print("Captured RAM @ 0x{:x} ({} bytes)".format(section.addr, section.size))

        return cd_writer, elf_fn


class MemfaultLogin(MemfaultGdbCommand):
    """Authenticate the current GDB session with Memfault"""

    GDB_CMD = "memfault login"

    def _invoke(self, unicode_args, from_tty, analytics_props, config):
        try:
            parsed_args = self.parse_args(unicode_args)
        except MemfaultGdbArgumentParseError:
            return

        api_uri = parsed_args.api_uri
        ingress_uri = parsed_args.ingress_uri

        status, reason, json_body = http_get_auth_me(
            api_uri, parsed_args.email, parsed_args.password
        )

        analytics_props["http_status"] = status
        analytics_props["api_uri"] = api_uri
        analytics_props["ingress_uri"] = ingress_uri

        if status >= 200 and status < 300:
            config.api_uri = MEMFAULT_CONFIG.api_uri = api_uri
            config.email = MEMFAULT_CONFIG.email = parsed_args.email
            config.ingress_uri = MEMFAULT_CONFIG.ingress_uri = ingress_uri
            config.organization = MEMFAULT_CONFIG.organization = parsed_args.organization
            config.password = MEMFAULT_CONFIG.password = parsed_args.password
            config.project = MEMFAULT_CONFIG.project = parsed_args.project
            config.user_id = MEMFAULT_CONFIG.user_id = json_body["id"]
            print("Authenticated successfully!")
        elif status == 404:
            print("Login failed. Make sure you have entered the email and password/key correctly.")
        else:
            print("Error occurred... HTTP Status {} {}".format(status, reason))

    def parse_args(self, unicode_args):
        parser = MemfaultGdbArgumentParser(description=MemfaultLogin.__doc__)
        parser.add_argument(
            "email", help="The username (email address) of the user to authenticate"
        )
        parser.add_argument(
            "password", help="The user API key or password of the user to authenticate"
        )
        parser.add_argument(
            "--organization", "-o", help="Default organization (slug) to use", default=None
        )
        parser.add_argument("--project", "-p", help="Default project (slug) to use", default=None)
        parser.add_argument(
            "--api-uri",
            help="Default API base URI to use (default: {})".format(MEMFAULT_DEFAULT_API_BASE_URI),
            default=MEMFAULT_DEFAULT_API_BASE_URI,
        )
        parser.add_argument(
            "--ingress-uri",
            help="Default ingress base URI to use (default: {})".format(
                MEMFAULT_DEFAULT_INGRESS_BASE_URI
            ),
            default=MEMFAULT_DEFAULT_INGRESS_BASE_URI,
        )
        args = list(filter(None, unicode_args.split(" ")))
        return parser.parse_args(args)


Memfault()
MemfaultCoredump()
MemfaultLogin()
MemfaultPostChunk()


class AnalyticsTracker(Thread):
    def __init__(self):
        super(AnalyticsTracker, self).__init__()
        self._queue = Queue()

    def track(self, event_name, event_properties=None, user_id=None):
        # Put in queue to offload to background thread, to avoid slowing down the GDB commands
        self._queue.put((event_name, event_properties, user_id))

    def log(self, level, type, **kwargs):  # noqa: A002
        props = dict(**kwargs)
        props["type"] = type
        props["level"] = level
        self.track("Log", props)

    def error(self, type, info=None):  # noqa: A002
        self.log("error", type, info=info)

    def _is_analytics_disabled(self):
        settings = settings_load()
        key = "analytics.disabled"
        return settings.get(key, False)

    def _is_unittest(self):
        # Pretty gross, but because importing this script has side effects (hitting the analytics endpoint),
        # mocking gets pretty tricky. Use this check to disable analytics for unit and integration tests
        return (
            hasattr(gdb, "MEMFAULT_MOCK_IMPLEMENTATION")
            or os.environ.get("MEMFAULT_DISABLE_ANALYTICS", None) is not None
        )

    def run(self):
        # uuid.getnode() is based on the mac address, so should be stable across multiple sessions on the same machine:
        anonymous_id = md5(uuid.UUID(int=uuid.getnode()).bytes).hexdigest()
        while True:
            try:
                event_name, event_properties, user_id = self._queue.get()

                if self._is_analytics_disabled() or self._is_unittest():
                    continue

                if event_properties is None:
                    event_properties = {}
                conn = HTTPSConnection("api.segment.io")
                body = {
                    "anonymousId": anonymous_id,
                    "event": event_name,
                    "properties": event_properties,
                }
                if user_id is not None:
                    body["userId"] = "user{}".format(user_id)
                headers = {
                    "Content-Type": "application/json",
                    "Authorization": "Basic RFl3Q0E1TENRTW5Uek95ajk5MTJKRjFORTkzMU5yb0o6",
                }
                conn.request("POST", "/v1/track", body=dumps(body), headers=headers)
                response = conn.getresponse()
                response.read()

                # Throttle a bit
                sleep(0.2)

            except Exception:  # noqa: S110
                pass  # Never fail due to analytics requests erroring out


ANALYTICS = AnalyticsTracker()
ANALYTICS.daemon = True
ANALYTICS.start()


def _track_script_sourced():
    system, _, release, version, machine, processor = platform.uname()
    try:
        from platform import mac_ver

        mac_version = mac_ver()[0]
    except Exception:
        mac_version = ""

    try:
        gdb_version = gdb.execute("show version", to_string=True).strip().split("\n")[0]
    except Exception:
        gdb_version = ""

    ANALYTICS.track(
        "Script sourced",
        # TODO: MFLT-497 -- properly version this
        {
            "version": "1",
            "python": platform.python_version(),
            "uname_system": system,
            "uname_release": release,
            "uname_version": version,
            "uname_machine": machine,
            "uname_processor": processor,
            "mac_version": mac_version,
            "gdb_version": gdb_version,
        },
    )


_track_script_sourced()

print("Memfault Commands Initialized successfully.")
