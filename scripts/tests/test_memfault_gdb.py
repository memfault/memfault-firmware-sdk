#
# Copyright (c) 2019-Present Memfault, Inc.
# See License.txt for details
#
import os
import sys
from unittest import mock
from unittest.mock import MagicMock

from io import BytesIO

script_dir = os.path.dirname(os.path.realpath(__file__))
root_dir = os.path.dirname(os.path.dirname(script_dir))
sys.path.append(root_dir)


def test_parse_maintenance_info_sections_no_file():
    with mock.patch.dict("sys.modules", gdb=MagicMock()):
        from memfault_gdb import parse_maintenance_info_sections

    fn, sections = parse_maintenance_info_sections(
        """Remote serial target in gdb-specific protocol:
Debugging a target over a serial line.
"""
    )
    assert fn == None
    assert sections == None


def test_parse_maintenance_info_sections_with_file():
    with mock.patch.dict("sys.modules", gdb=MagicMock()):
        from memfault_gdb import parse_maintenance_info_sections, Section

    fixture = """Exec file:
    `/Users/mthe/memfault/memfault/sdk/embedded/platforms/nrf5/memfault_test_app/build/mf_test_app_nrf52840_s140.out', file type elf32-littlearm.
 [0]     0x26000->0x6b784 at 0x00006000: .text ALLOC LOAD READONLY CODE HAS_CONTENTS
 [1]     0x2003d800->0x2003e878 at 0x0005d800: .rtt ALLOC
 [2]     0x6b784->0x6b7a8 at 0x0004b784: .gnu_build_id ALLOC LOAD READONLY DATA HAS_CONTENTS
 [3]     0x2003fe00->0x2003fe14 at 0x0005fe00: .fw_install_info ALLOC
 [4]     0x2003ff00->0x20040000 at 0x0005ff00: .noinit ALLOC
 [5]     0x6b7a8->0x6b7c8 at 0x0004b7a8: .sdh_soc_observers ALLOC LOAD READONLY DATA HAS_CONTENTS
 [6]     0x6b7c8->0x6b850 at 0x0004b7c8: .sdh_ble_observers ALLOC LOAD READONLY DATA HAS_CONTENTS
 [7]     0x6b850->0x6b860 at 0x0004b850: .sdh_stack_observers ALLOC LOAD READONLY DATA HAS_CONTENTS
 [8]     0x6b860->0x6b868 at 0x0004b860: .sdh_req_observers ALLOC LOAD READONLY DATA HAS_CONTENTS
 [9]     0x6b868->0x6b880 at 0x0004b868: .sdh_state_observers ALLOC LOAD READONLY DATA HAS_CONTENTS
 [10]     0x6b880->0x6b8a8 at 0x0004b880: .nrf_queue ALLOC LOAD READONLY DATA HAS_CONTENTS
 [11]     0x6b8a8->0x6b8d0 at 0x0004b8a8: .nrf_balloc ALLOC LOAD READONLY DATA HAS_CONTENTS
 [12]     0x6b8d0->0x6b8f8 at 0x0004b8d0: .cli_command ALLOC LOAD READONLY DATA HAS_CONTENTS
 [13]     0x6b8f8->0x6b908 at 0x0004b8f8: .crypto_data ALLOC LOAD READONLY DATA HAS_CONTENTS
 [14]     0x6b908->0x6b9e0 at 0x0004b908: .log_const_data ALLOC LOAD READONLY DATA HAS_CONTENTS
 [15]     0x6b9e0->0x6ba00 at 0x0004b9e0: .log_backends ALLOC LOAD READONLY DATA HAS_CONTENTS
 [16]     0x6ba00->0x6ba08 at 0x0004ba00: .ARM.exidx ALLOC LOAD READONLY DATA HAS_CONTENTS
 [17]     0x200057b8->0x20005bf0 at 0x000557b8: .data ALLOC LOAD DATA HAS_CONTENTS
 [18]     0x20005bf0->0x20005c04 at 0x00055bf0: .cli_sorted_cmd_ptrs ALLOC LOAD DATA HAS_CONTENTS
 [19]     0x20005c04->0x20005c2c at 0x00055c04: .fs_data ALLOC LOAD DATA HAS_CONTENTS
 [20]     0x20005c30->0x2000b850 at 0x00055c2c: .bss ALLOC
 [21]     0x2000b850->0x2000d850 at 0x00055c30: .heap READONLY HAS_CONTENTS
 [22]     0x2000b850->0x2000d850 at 0x00057c30: .stack_dummy READONLY HAS_CONTENTS
 [23]     0x0000->0x0030 at 0x00059c30: .ARM.attributes READONLY HAS_CONTENTS
 [24]     0x0000->0x00f4 at 0x00059c60: .comment READONLY HAS_CONTENTS
 [25]     0x0000->0xf3d30 at 0x00059d54: .debug_info READONLY HAS_CONTENTS
 [26]     0x0000->0x1eeec at 0x0014da84: .debug_abbrev READONLY HAS_CONTENTS
 [27]     0x0000->0x59416 at 0x0016c970: .debug_loc READONLY HAS_CONTENTS
 [28]     0x0000->0x2c10 at 0x001c5d88: .debug_aranges READONLY HAS_CONTENTS
 [29]     0x0000->0x121a8 at 0x001c8998: .debug_ranges READONLY HAS_CONTENTS
 [30]     0x0000->0x37f3a at 0x001dab40: .debug_macro READONLY HAS_CONTENTS
 [31]     0x0000->0x708cb at 0x00212a7a: .debug_line READONLY HAS_CONTENTS
 [32]     0x0000->0xcee28 at 0x00283345: .debug_str READONLY HAS_CONTENTS
 [33]     0x0000->0x91f0 at 0x00352170: .debug_frame READONLY HAS_CONTENTS
 [34]     0x0000->0x00df at 0x0035b360: .stabstr READONLY HAS_CONTENTS
"""
    fn, sections = parse_maintenance_info_sections(fixture)
    assert (
        fn
        == "/Users/mthe/memfault/memfault/sdk/embedded/platforms/nrf5/memfault_test_app/build/mf_test_app_nrf52840_s140.out"
    )
    assert len(sections) == 35
    assert sections[0] == Section(0x26000, 0x6B784 - 0x26000, ".text", read_only=True)
    assert sections[20] == Section(0x20005C30, 0x2000B850 - 0x20005C30, ".bss", read_only=False)


def test_should_capture_section():
    with mock.patch.dict("sys.modules", gdb=MagicMock()):
        from memfault_gdb import Section, should_capture_section
    # Never capture .text, even when NOT marked READONLY:
    assert False == should_capture_section(Section(0, 0, ".text", read_only=False))

    # Never capture .debug_...
    assert False == should_capture_section(Section(0, 0, ".debug_info", read_only=True))

    # Always capture sections with .bss, .heap, .data or .stack in the name, even when marked READONLY:
    assert True == should_capture_section(Section(0, 0, ".foo.bss", read_only=True))
    assert True == should_capture_section(Section(0, 0, ".stack_dummy", read_only=True))
    assert True == should_capture_section(Section(0, 0, ".heap_psram", read_only=True))
    assert True == should_capture_section(Section(0, 0, ".dram0.data", read_only=True))


def test_coredump_writer():
    with mock.patch.dict("sys.modules", gdb=MagicMock()):
        from memfault_gdb import Section, MemfaultCoredumpWriter

    cd_writer = MemfaultCoredumpWriter()
    cd_writer.device_serial = "device_serial"
    cd_writer.firmware_version = "1.2.3"
    cd_writer.hardware_revision = "gdb-proto"
    cd_writer.trace_reason = 5
    cd_writer.regs = {
        "r0": 4 * b"\x00",
        "r1": 4 * b"\x01",
        "r2": 4 * b"\x02",
        "r3": 4 * b"\x03",
        "r4": 4 * b"\x04",
        "r5": 4 * b"\x05",
        "r6": 4 * b"\x06",
        "r7": 4 * b"\x07",
        "r8": 4 * b"\x08",
        "r9": 4 * b"\x09",
        "r10": 4 * b"\x0A",
        "r11": 4 * b"\x0B",
        "r12": 4 * b"\x0C",
        "sp": 4 * b"\x0D",
        "lr": 4 * b"\x0E",
        "pc": 4 * b"\x0F",
        "xpsr": 4 * b"\x10",
    }
    section = Section(4, 32, ".test", "")
    section.data = b"hello world"
    cd_writer.add_section(section)

    f_out = BytesIO()
    cd_writer.write(f_out)
    f_out.seek(0)
    fixture_bin = open(os.path.join(script_dir, "fixture.bin"), "rb").read()
    assert f_out.read() == fixture_bin
