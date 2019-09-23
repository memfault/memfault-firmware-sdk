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
    raise Exception(error_str)


# Note: not using `requests` but using the built-in http.client instead, so
# there will be no additional dependencies other than Python itself.
try:
    from httplib import HTTPSConnection, HTTPConnection
    from urlparse import urlparse
    from Queue import Queue
except ImportError:
    from http.client import HTTPSConnection, HTTPConnection
    from urllib.parse import urlparse
    from queue import Queue


MEMFAULT_DEFAULT_INGRESS_BASE_URI = "https://ingress.memfault.com"
MEMFAULT_DEFAULT_API_BASE_URI = "https://api.memfault.com"

MEMFAULT_TRY_INGRESS_BASE_URI = "https://ingress.try.memfault.com"
MEMFAULT_TRY_API_BASE_URI = "https://api.try.memfault.com"


def _get_input():
    try:
        # Python 2.x
        return raw_input
    except:
        # In Python 3.x, raw_input was renamed to input
        # NOTE: Python 2.x also had an input() function which eval'd the input...!
        return input


class MemfaultConfig(object):
    ingress_uri = MEMFAULT_DEFAULT_INGRESS_BASE_URI
    api_uri = MEMFAULT_DEFAULT_API_BASE_URI
    email = None
    password = None
    organization = None
    project = None

    # Added `json_path` and `input` as attributes on the config to aid unit testing:
    json_path = expanduser("~/.memfault/gdb.json")

    input = _get_input()

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
        return exc_handler == curr_pc
    except:
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

    FAULT_START_PROMPT = """
We see you are trying out Memfault from a Fault handler. That's great!
For the best results and to mirror the behavior of our firmware SDK,
please run "memfault coredump" at exception entry before other code has run
in the exception handler
"""
    GDB_HOW_TO_FAULT_PROMPT = """
It's easy to halt at exception entry by installing a breakpoint from gdb.
For example,
(gdb) breakpoint HardFault_Handler
"""

    exc_return = _get_register_value(register_list, "lr")
    if exc_return >> 28 != 0xF:
        print("{} {}".format(FAULT_START_PROMPT, GDB_HOW_TO_FAULT_PROMPT))
        raise Exception("LR no longer set to EXC_RETURN value")

    # DCRS - armv8m only - only relevant when chaining secure and non-secure exceptions
    # so pretty unlikely to be hit in a try test scenario
    if exc_return & (1 << 5) == 0:
        raise Exception("DCRS exception unwinding unimplemented")

    if not _pc_in_vector_table(register_list, exception_number, analytics_props):
        analytics_props["displayed_fault_prompt"] = True
        # The pc is not at the start of the exception handler. Some firmware implementations
        # will redirect the vector table to a software vector table. If that's the case, it's
        # hard to detect programmatically, let's check in with the user
        y = _get_input()(
            "{}\nAre you currently at the start of an exception handler [y/n]?".format(
                FAULT_START_PROMPT
            )
        )
        if y.upper() != "Y":
            print(GDB_HOW_TO_FAULT_PROMPT)
            raise Exception("User did not confirm being at beginning of exception")
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


# The order in which we collect ARM registers in a crash
ARM_CORTEX_M_REGISTER_COLLECTION = (
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

# GDB allows the remote server to provide register names via the target description which can
# be exchanged between client and server:
#   https://sourceware.org/gdb/onlinedocs/gdb/Target-Description-Format.html#Target-Description-Format
#
# Different implementations of the gdb server use different names for these custom register sets
# This dictionary holds known mappings between the names we will use and the ones in different GDBDebugContextFacade
# server implementations
#
# NOTE: If the only difference is capitalization, that will be automagically resolved below
ARM_CORTEX_M_ALT_NAMES = {"xpsr": ("cpsr",)}

# FIXME: De-duplicate with code from rtos_register_stacking.py
def concat_registers_dict_to_bytes(regs):
    result = b""
    for reg_name in ARM_CORTEX_M_REGISTER_COLLECTION:
        assert reg_name.lower() == reg_name
        if reg_name not in regs:
            result += b"\x00\x00\x00\x00"
            continue
        result += regs[reg_name]
    return result


def _is_expected_reg(reg_name):
    return reg_name in ARM_CORTEX_M_REGISTER_COLLECTION


def _add_reg_collection_error_analytic(analytics_props, reg_name, error):
    if not _is_expected_reg(reg_name):
        return

    REG_COLLECTION_ERROR = "reg_collection_error"
    if REG_COLLECTION_ERROR not in analytics_props:
        analytics_props[REG_COLLECTION_ERROR] = {}

    analytics_props[REG_COLLECTION_ERROR][reg_name] = error


def _try_read_register(frame, lookup_name, register_list, analytics_props, result_name=None):
    # `info reg` will print all registers, even though they are not part of the core.
    # If that's the case, doing frame.read_register() will raise a gdb.error.
    try:
        value = frame.read_register(lookup_name)
        value_str = str(value)
        if value_str != "<unavailable>":
            name_to_use = lookup_name if result_name is None else result_name
            register_list[name_to_use] = register_value_to_bytes(value)
        else:
            _add_reg_collection_error_analytic(analytics_props, lookup_name, "<unavailable> value")
    except:
        _add_reg_collection_error_analytic(analytics_props, lookup_name, traceback.format_exc())
        pass


def lookup_registers_from_list(info_reg_all_list, analytics_props):
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

        alt_reg_names = ARM_CORTEX_M_ALT_NAMES.get(reg, [])
        for alt_reg_name in alt_reg_names:
            if alt_reg_name in found_registers:
                return alt_reg_name

        return None

    alt_reg_names = []
    for expected_reg in ARM_CORTEX_M_REGISTER_COLLECTION:
        if expected_reg in register_names:
            continue

        alt_reg_name = _search_list_for_alt_name(expected_reg, register_names)
        if alt_reg_name:
            alt_reg_names.append((alt_reg_name, expected_reg))
            continue

        _add_reg_collection_error_analytic(
            analytics_props, expected_reg, "Not found in register set"
        )

    # Iterate over all register names and pull the value out of the frame
    register_list = {}

    # Remove register_names we don't care about before actually looking up values
    register_names = filter(_is_expected_reg, register_names)

    for reg_name in register_names:
        _try_read_register(frame, reg_name, register_list, analytics_props)

    for lookup_reg_name, result_reg_name in alt_reg_names:
        _try_read_register(frame, lookup_reg_name, register_list, analytics_props, result_reg_name)

    # if we can't patch the registers, we'll just fallback to the active state
    try:
        check_and_patch_reglist_for_fault(register_list, analytics_props)
    except Exception as e:
        analytics_props["fault_register_recover_error"] = {"traceback": traceback.format_exc()}
        pass

    return register_list


# FIXME: De-duplicate with code from trace_builder_coredump.py
def get_current_registers(gdb_thread, analytics_props):
    gdb_thread.switch()

    # GDB Doesn't have a convenient way to know all of the registers in Python, so this is the
    # best way. Call this, rip out the first element in each row...that's the register name
    #
    # NOTE: We use the "all" argument below because on some versions of gdb "msp, psp, etc" are not considered part of them
    # core set. This will also dump all the fpu registers which we don't collect but thats fine
    info_reg_all_list = gdb.execute("info reg all", to_string=True)
    return lookup_registers_from_list(info_reg_all_list, analytics_props)


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
    ARM_V7M_MPU = 9
    SOFTWARE_VERSION = 10
    SOFTWARE_TYPE = 11


class MemfaultCoredumpWriter(object):
    def __init__(self):
        self.device_serial = "DEMOSERIALNUMBER"
        self.software_version = "1.0.0"
        self.software_type = "main"
        self.hardware_revision = "DEVBOARD"
        self.trace_reason = 5  # Debugger Halted
        self.machine_type = 40  # ARM
        self.regs = {}
        self.sections = []
        self.armv67_mpu = None

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
            MemfaultCoredumpBlockType.CURRENT_REGISTERS, concat_registers_dict_to_bytes(self.regs)
        )
        _write_block(MemfaultCoredumpBlockType.DEVICE_SERIAL, self.device_serial.encode("utf8"))
        _write_block(
            MemfaultCoredumpBlockType.SOFTWARE_VERSION, self.software_version.encode("utf8")
        )
        _write_block(MemfaultCoredumpBlockType.SOFTWARE_TYPE, self.software_type.encode("utf8"))
        _write_block(
            MemfaultCoredumpBlockType.HARDWARE_REVISION, self.hardware_revision.encode("utf8")
        )
        _write_block(MemfaultCoredumpBlockType.MACHINE_TYPE, pack("<I", self.machine_type))
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


def armv7_get_used_ram_base_addresses(elf_sections):
    capturable_elf_sections = list(filter(should_capture_section, elf_sections))

    def _is_ram(base_addr):
        # See Table B3-1 ARMv7-M address map in "ARMv7-M Architecture Reference Manual"
        return base_addr in (0x20000000, 0x60000000, 0x80000000)

    return set(
        filter(_is_ram, map(lambda section: section.addr & 0xE0000000, capturable_elf_sections))
    )


def read_memory_until_error(inferior, start, size, read_size=4 * 1024):
    data = b""
    end = start + size
    try:
        for addr in range(start, end, read_size):
            data += bytes(inferior.read_memory(addr, min(read_size, end - addr)))
    except Exception as e:  # Catch gdbserver read exceptions -- not sure what exception classes can get raised here
        print(e)
    finally:
        return data


def _create_http_connection(base_uri):
    url = urlparse(base_uri)
    if url.hostname is None:
        raise Exception("Invalid base URI, must be http(s)://hostname")
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
    if hasattr(body, "read"):
        body = body.read()

    conn.request(method, path, body=body, headers=headers)
    response = conn.getresponse()
    status = response.status
    reason = response.reason
    body = response.read()
    try:
        json_body = loads(body)
    except:
        json_body = None
    conn.close()
    return status, reason, json_body


def add_basic_auth(user, password, headers=None):
    headers = dict(headers) if headers else {}
    headers["Authorization"] = "Basic {}".format(
        b2a_base64("{}:{}".format(user, password).encode("utf8")).decode("ascii").strip()
    )
    return headers


def _http_api(config, method, path, headers=None, body=None):
    headers = add_basic_auth(config.email, config.password, headers=headers)
    return _http(method, config.api_uri, path, headers, body)


def http_post_coredump(coredump_file, project_key, ingress_uri):
    headers = {"Content-Type": "application/octet-stream", "Memfault-Project-Key": project_key}
    status, reason, _ = _http(
        "POST", ingress_uri, "/api/v0/upload/coredump", headers=headers, body=coredump_file
    )
    return status, reason


def http_get_auth_me(api_uri, email, password):
    headers = add_basic_auth(email, password)
    return _http("GET", api_uri, "/auth/me", headers=headers)


def http_put_artifact(
    config, artifact_readable, software_type, software_version, artifact_type="symbols"
):
    status, reason, body = _http_api(
        config,
        "PUT",
        "/api/v0/organizations/{organization}/projects/{project}/software_types/{software_type}/software_versions/{software_version}/artifacts/{artifact_type}".format(
            organization=config.organization,
            project=config.project,
            software_type=software_type,
            software_version=software_version,
            artifact_type=artifact_type,
        ),
        headers={"Content-Type": "application/octet-stream"},
        body=artifact_readable,
    )
    if status < 200 or status >= 300:
        return False, (status, reason)
    return True, None


def http_get_software_version(config, software_type, software_version):
    SOFTWARE_VERSION_URL = "/api/v0/organizations/{organization}/projects/{project}/software_types/{software_type}/software_versions/{software_version}".format(
        organization=config.organization,
        project=config.project,
        software_type=software_type,
        software_version=software_version,
    )

    status, reason, body = _http_api(config, "GET", SOFTWARE_VERSION_URL)
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
    artifacts = software_version_obj["artifacts"]
    return any(map(lambda artifact: artifact["type"] == "symbols", artifacts))


def upload_symbols_if_needed(config, elf_fn, software_type, software_version):
    has_symbols = has_uploaded_symbols(config, software_type, software_version)
    if has_symbols:
        print("Symbols have already been uploaded, skipping!")
        return
    if not has_symbols:
        print("Uploading symbols...")
        with open(elf_fn, "rb") as elf_f:
            success, status_and_reason = http_put_artifact(
                config, elf_f, software_type, software_version
            )
            if not success:
                print("Failed to upload symbols: {}".format(status_and_reason))
            else:
                print("Done!")


# FIXME: Duped from tools/gdb_memfault.py
class MemfaultGdbArgumentParseError(Exception):
    pass


class MemfaultGdbArgumentParser(argparse.ArgumentParser):
    def exit(self, status=0, message=None):
        if message:
            self._print_message(message)
        # Don't call sys.exit()
        raise MemfaultGdbArgumentParseError()


def add_config_args_and_parse_args(parser, unicode_args):
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

    config = MemfaultConfig()
    config.ingress_uri = parsed_args.ingress_uri
    config.api_uri = parsed_args.api_uri
    config.email = parsed_args.email
    config.password = parsed_args.password
    config.organization = parsed_args.organization
    config.project = parsed_args.project

    return parsed_args, config


class MemfaultGdbCommand(gdb.Command):
    GDB_CMD = "memfault override_me"

    def __init__(self, *args, **kwargs):
        super(MemfaultGdbCommand, self).__init__(self.GDB_CMD, gdb.COMMAND_USER, *args, **kwargs)

    def invoke(self, arg, from_tty):
        # Work-around for GDB-py not printing out the backtrace upon exceptions
        analytics_props = {}
        start_time = time()
        try:
            self._invoke(arg, from_tty, analytics_props)
        except Exception as e:
            print(traceback.format_exc())
            ANALYTICS.track("Exception", {"traceback": traceback.format_exc(), "args": arg})
            raise e  # Important, otherwise unit test failures may go undetected!
        analytics_props["duration_ms"] = int((time() - start_time) * 1000)
        ANALYTICS.track("Command {}".format(self.GDB_CMD), analytics_props)

    def _invoke(self, arg, from_tty, analytics_props):
        pass


class Memfault(MemfaultGdbCommand):
    """Memfault GDB commands"""

    GDB_CMD = "memfault"

    def __init__(self):
        super(Memfault, self).__init__(gdb.COMPLETE_NONE, prefix=True)

    def _invoke(self, arg, from_tty, analytics_props):
        gdb.execute("help memfault")


def settings_load():
    try:
        with open(MEMFAULT_CONFIG.json_path, "rb") as f:
            return load(f)
    except:
        return {}


def settings_save(settings):
    try:
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


def _try_collect_mpu_settings():
    cpuid = 0xE000ED00
    reg_val, _ = _read_register(cpuid)
    partno = (reg_val >> 4) & 0xFFF

    CORTEX_M_CPUIDS = {
        0xC20: "M0",
        0xC21: "M1",
        0xC23: "M3",
        0xC24: "M4",
        0xC27: "M7",
        0xC60: "M0+",
    }

    if partno not in CORTEX_M_CPUIDS:
        return None

    print("Cortex-{} detected".format(CORTEX_M_CPUIDS[partno]))
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
    for i in range(0, num_regions):
        _write_register(mpu_rnr, i)
        _, data = _read_register(mpu_rbar)
        result += data
        _, data = _read_register(mpu_rasr)
        result += data

    return result


class MemfaultCoredump(MemfaultGdbCommand):
    """Captures a coredump from the target and uploads it to Memfault for analysis"""

    GDB_CMD = "memfault coredump"

    def _check_permission(self, analytics_props):
        settings = settings_load()
        key = "coredump.allow"
        allowed = settings.get(key, False)
        if allowed:
            analytics_props["permission"] = "accepted-stored"
            return True

        y = MEMFAULT_CONFIG.input(
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
"""  # This last newline is important! If it's not here, the last line is not shown on Windows!
        )
        if y.upper() != "Y":
            print("Aborting...")
            analytics_props["permission"] = "rejected"
            return False
        analytics_props["permission"] = "accepted"

        settings[key] = True
        settings_save(settings)
        return True

    def _invoke(self, unicode_args, from_tty, analytics_props):
        if not self._check_permission(analytics_props):
            return

        try:
            parsed_args, config = self.parse_args(unicode_args)
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

        cd_writer, elf_fn = self.build_coredump_writer(parsed_args.region, analytics_props)
        if not cd_writer:
            return

        elf_hash = get_file_hash(elf_fn)

        # TODO: try calling memfault_platform_get_device_info() and use returned info if present
        # Populate the version based on hash of the .elf for now:
        software_version = "1.0.0-md5+{}".format(elf_hash[:8])
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
            # FIXME: Print direct link to trace
            # https://memfault.myjetbrains.com/youtrack/issue/MFLT-461
            print(_infer_issues_html_url(parsed_args.ingress_uri, config))
        else:
            print("Error occurred... HTTP Status {} {}".format(status, reason))

    def parse_args(self, unicode_args):
        parser = MemfaultGdbArgumentParser(description=MemfaultCoredump.__doc__)
        parser.add_argument(
            "source",
            help="Source of coredump: 'live' (default) or 'storage' (target's live RAM or stored coredump)",
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
            return int(x, 0)

        parser.add_argument(
            "--region",
            "-r",
            help="Specify memory region and size in bytes to capture. This option can be passed multiple times."
            " Example: `-r 0x20000000 1024 0x80000000 512` will capture 1024 bytes starting at 0x20000000."
            " When no regions are passed, the command will attempt to infer what to"
            " capture based on the sections in the loaded .elf.",
            type=_auto_int,
            nargs=2,
            action="append",
        )
        return add_config_args_and_parse_args(parser, unicode_args)

    def build_coredump_writer(self, regions, analytics_props):
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
        except:
            # Exception can be raised if selected thread has dissappeared in the mean time
            # SystemError: could not find gdb thread object
            thread = None
        finally:
            if not thread:
                print("Did not find any threads!?")
                analytics_props["error"] = "Failed to activate thread"
                return None, None

        # inferior.architecture() is a relatively new API, so let's use "show arch" instead:
        show_arch_output = gdb.execute("show arch", to_string=True).lower()
        current_arch_matches = re.search("currently ([^)]+)", show_arch_output)
        if current_arch_matches:
            # Using groups() here instead of fn_match[1] for python2.x compatibility
            current_arch = current_arch_matches.groups()[0]
            # Should tell us about different arm flavors:
            analytics_props["arch"] = current_arch
            if "arm" not in current_arch:
                print("This command is currently only supported for ARM targets!")
                analytics_props["error"] = "Unsupported architecture"
                return None, None
        else:
            error = "show arch has unexpected output"
            analytics_props["error"] = error
            ANALYTICS.error(error, info=show_arch_output)
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
                "Hints: did you compile with -g or similar flags? did you inadvertently strip the binary?"
            )

        cd_writer = MemfaultCoredumpWriter()
        cd_writer.regs = get_current_registers(thread, analytics_props)

        try:
            scb_base = 0xE000ED00
            scb_top = 0xE000ED8F
            section = Section(scb_base, scb_top - scb_base, "ARMv7-M System Control and ID blocks")
            section.data = inferior.read_memory(section.addr, section.size)
            cd_writer.add_section(section)
            analytics_props["scb_ok"] = True
        except:
            analytics_props["scb_collection_error"] = {"traceback": traceback.format_exc()}
            pass

        try:
            cd_writer.armv67_mpu = _try_collect_mpu_settings()
            print("Collected MPU config")
        except:
            analytics_props["mpu_collection_error"] = {"traceback": traceback.format_exc()}
            pass

        if regions is None:

            def _guessed_ram_regions():
                base_addresses = armv7_get_used_ram_base_addresses(sections)
                for base in base_addresses:
                    yield base, 1024 * 1024  # Capture up to 1MB

            regions = _guessed_ram_regions()
            print(
                "No capturing regions were specified; will default to capturing the first 1MB for each used RAM address range."
            )
            print(
                "Tip: optionally, you can use `--region <addr> <size>` to manually specify capturing regions and increase capturing speed."
            )

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

    def _invoke(self, unicode_args, from_tty, analytics_props):
        try:
            parsed_args = self.parse_args(unicode_args)
        except MemfaultGdbArgumentParseError:
            return

        if parsed_args.try_me:
            api_uri = MEMFAULT_TRY_API_BASE_URI
            ingress_uri = MEMFAULT_TRY_INGRESS_BASE_URI
        else:
            api_uri = parsed_args.api_uri
            ingress_uri = parsed_args.ingress_uri

        status, reason, json_body = http_get_auth_me(
            api_uri, parsed_args.email, parsed_args.password
        )

        analytics_props["http_status"] = status
        analytics_props["api_uri"] = api_uri
        analytics_props["ingress_uri"] = ingress_uri

        if status >= 200 and status < 300:
            MEMFAULT_CONFIG.api_uri = api_uri
            MEMFAULT_CONFIG.email = parsed_args.email
            MEMFAULT_CONFIG.ingress_uri = ingress_uri
            MEMFAULT_CONFIG.organization = parsed_args.organization
            MEMFAULT_CONFIG.password = parsed_args.password
            MEMFAULT_CONFIG.project = parsed_args.project
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
        parser.add_argument(
            "--try-me", help="Use try.memfault.com base URIs", default=False, action="store_true"
        )
        args = list(filter(None, unicode_args.split(" ")))
        return parser.parse_args(args)


Memfault()
MemfaultCoredump()
MemfaultLogin()


class AnalyticsTracker(Thread):
    def __init__(self):
        super(AnalyticsTracker, self).__init__()
        self._queue = Queue()

    def track(self, event_name, event_properties=None):
        # Put in queue to offload to background thread, to avoid slowing down the GDB commands
        self._queue.put((event_name, event_properties))

    def log(self, level, type, **kwargs):
        props = dict(**kwargs)
        props["type"] = type
        props["level"] = level
        self.track("Log", props)

    def error(self, type, info=None):
        self.log("error", type, info=info)

    def _is_analytics_disabled(self):
        settings = settings_load()
        key = "analytics.disabled"
        return settings.get(key, False)

    def _is_unittest(self):
        # Pretty gross, but because importing this script has side effects (hitting the analytics endpoint),
        # mocking gets pretty tricky. Copping out...:
        return "PYTEST_CURRENT_TEST" in os.environ

    def run(self):
        # uuid.getnode() is based on the mac address, so should be stable across multiple sessions on the same machine:
        anonymousId = md5(uuid.UUID(int=uuid.getnode()).bytes).hexdigest()
        while True:
            try:
                event_name, event_properties = self._queue.get()

                if self._is_analytics_disabled() or self._is_unittest():
                    continue

                if event_properties is None:
                    event_properties = {}
                conn = HTTPSConnection("api.segment.io")
                body = {
                    "anonymousId": anonymousId,
                    "event": event_name,
                    "properties": event_properties,
                }
                headers = {
                    "Content-Type": "application/json",
                    "Authorization": "Basic RFl3Q0E1TENRTW5Uek95ajk5MTJKRjFORTkzMU5yb0o6",
                }
                conn.request("POST", "/v1/track", body=dumps(body), headers=headers)
                response = conn.getresponse()
                response.read()

                # Throttle a bit
                sleep(0.2)

            except Exception as e:
                pass  # Never fail due to analytics requests erroring out


ANALYTICS = AnalyticsTracker()
ANALYTICS.daemon = True
ANALYTICS.start()


def _track_script_sourced():
    system, _, release, version, machine, processor = platform.uname()
    try:
        from platform import mac_ver

        mac_version = mac_ver()[0]
    except:
        mac_version = ""

    try:
        gdb_version = gdb.execute("show version", to_string=True).strip().split("\n")[0]
    except:
        gdb_version = ""

    ANALYTICS.track(
        "Script sourced",
        # FIXME: MFLT-497 -- properly version this
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
