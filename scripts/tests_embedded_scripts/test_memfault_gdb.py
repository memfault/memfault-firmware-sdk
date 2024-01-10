#
# Copyright (c) Memfault, Inc.
# See License.txt for details
#

import json
import os
import sys
from io import BufferedIOBase, BytesIO
from json import dumps
from typing import Any, List, Tuple
from unittest import mock
from unittest.mock import ANY, MagicMock

import pytest

from . import gdb_fake

gdb_fake.install_fake()
tests_dir = os.path.dirname(os.path.realpath(__file__))
scripts_dir = os.path.dirname(tests_dir)
sys.path.insert(0, scripts_dir)


from memfault_gdb import (  # noqa: E402  # pyright: ignore[reportGeneralTypeIssues]
    ArmCortexMCoredumpArch,
    MemfaultConfig,
    MemfaultCoredump,
    MemfaultCoredumpWriter,
    MemfaultLogin,
    Section,
    _post_chunk_data,
    add_basic_auth,
    has_uploaded_symbols,
    lookup_registers_from_list,
    parse_maintenance_info_sections,
    read_memory_until_error,
    settings_save,
    should_capture_section,
)


# We save the time in the coredump generated so each run of "memfault coredump" looks unique
# Let's monkeypatch time so we get a reproducible time in out unit tests
@pytest.fixture(autouse=True)
def _patch_datetime_now(mocker):
    mocker.patch("memfault_gdb.time", return_value=float(1565034658.6))


@pytest.fixture()
def http_expect_request():
    connections: List[MagicMock] = []
    expected_requests: List[Tuple[Any, ...]] = []
    actual_request_count = 0

    def _http_expect_request(url, method, assert_body_fn, req_headers, resp_status, resp_body):
        expected_requests.append((url, method, assert_body_fn, req_headers, resp_status, resp_body))

    def _create_connection_constructor(scheme):
        def _connection_constructor(hostname, port=0):
            connection = MagicMock()
            try:
                (
                    exp_url,
                    exp_method,
                    assert_body_fn,
                    exp_req_headers,
                    resp_status,
                    resp_body,
                ) = expected_requests[len(connections)]
            except IndexError as e:
                raise AssertionError("Unexpected mock HTTPConnection created!") from e

            def _request(method, path, body=None, headers=None):
                nonlocal actual_request_count
                actual_request_count += 1
                standard_port = (scheme == "http" and port != 80) or (
                    scheme == "https" and port != 443
                )
                port_str = ":{}".format(port) if standard_port else ""
                assert "{}://{}{}{}".format(scheme, hostname, port_str, path) == exp_url
                assert method == exp_method
                if assert_body_fn and assert_body_fn is not ANY:
                    body_bytes = body.read() if isinstance(body, BufferedIOBase) else body
                    assert_body_fn(body_bytes)
                assert headers == exp_req_headers

            connection.request = _request
            response = MagicMock()
            response.status = resp_status
            response.reason = ""
            response.read.return_value = (
                None if resp_body is None else dumps(resp_body).encode("utf8")
            )
            connection.getresponse.return_value = response
            connections.append(connection)
            return connection

        return _connection_constructor

    with (
        mock.patch("memfault_gdb.HTTPSConnection", _create_connection_constructor("https")),
        mock.patch("memfault_gdb.HTTPConnection", _create_connection_constructor("http")),
    ):
        yield _http_expect_request

    assert actual_request_count == len(expected_requests)


TEST_EMAIL = "martijn@memfault.com"
TEST_PASSWORD = "open_sesame"
TEST_AUTH_HEADERS = {"Authorization": "Basic bWFydGlqbkBtZW1mYXVsdC5jb206b3Blbl9zZXNhbWU="}
TEST_ORG = "acme-inc"
TEST_PROJECT = "smart-sink"
TEST_HW_VERSION = "DEVBOARD"
TEST_PROJECT_KEY = "7f083342d30444b2b3bed65357f24a19"


@pytest.fixture(autouse=True)
def test_config(mocker, tmp_path):
    """Updates MEMFAULT_CONFIG to contain TEST_... values"""
    rv = MemfaultConfig()
    mocker.patch("memfault_gdb.MEMFAULT_CONFIG", rv)
    rv.prompt = lambda prompt: "Y"
    rv.json_path = str(tmp_path / "settings.yml")
    return rv


@pytest.fixture()
def test_config_with_login(test_config):
    test_config.email = TEST_EMAIL
    test_config.password = TEST_PASSWORD
    test_config.organization = TEST_ORG
    test_config.project = TEST_PROJECT
    return test_config


TEST_MAINTENANCE_INFO_SECTIONS_FIXTURE_FMT = """Exec file:
    `{}', file type elf32-littlearm.
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


@pytest.fixture()
def fake_elf(tmp_path):
    file = tmp_path / "fake.elf"
    file.write_bytes(b"ELF")
    return file


@pytest.fixture()
def maintenance_info_sections_fixture(fake_elf):
    return TEST_MAINTENANCE_INFO_SECTIONS_FIXTURE_FMT.format(fake_elf)


# Data from a GDB Server with register names that differ from the final names we use
@pytest.fixture()
def info_reg_all_fixture(fake_elf):
    return """
r0             0x0	0
r1             0x1	1
r2             0xe000ed00	-536810240
r3             0x20003fe8	536887272
r4             0x20000294	536871572
r5             0x0	0
r6             0x0	0
r7             0x0	0
r8             0x0	0
r9             0x0	0
r10            0x0	0
r11            0x0	0
r12            0x0	0
sp             0x200046c8	0x200046c8 <os_idle_thread_stack+480>
lr             0x8002649	134227529
pc             0x8007122	0x8007122 <hal_sleep+18>
cpsr           0x61000000	1627389952
PRIMASK        0x0	0
BASExPRI        0x0	0
FAULTMASK      0x0	0
CONTROL        0x0	0
MSP            0x2001ffc0	537001920
PSP            0x200046c8	536889032
"""


@pytest.fixture()
def _settings_coredump_allowed(test_config):
    settings_save({"coredump.allow": True})


def test_parse_maintenance_info_sections_no_file():
    fn, sections = parse_maintenance_info_sections(
        """Remote serial target in gdb-specific protocol:
Debugging a target over a serial line.
"""
    )
    assert fn is None
    assert sections is None


def test_parse_maintenance_info_sections_with_file(maintenance_info_sections_fixture, fake_elf):
    fn, sections = parse_maintenance_info_sections(maintenance_info_sections_fixture)
    assert fn == str(fake_elf)
    assert sections
    assert len(sections) == 35
    assert sections[0] == Section(0x26000, 0x6B784 - 0x26000, ".text", read_only=True)
    assert sections[20] == Section(0x20005C30, 0x2000B850 - 0x20005C30, ".bss", read_only=False)


def test_read_current_registers(mocker, info_reg_all_fixture):
    frame = MagicMock()
    frame.read_register.return_value = gdb_fake.Value(1, type=gdb_fake.Type(sizeof=4))
    mocker.patch.object(gdb_fake, "newest_frame", return_value=frame)

    dummy_dict: dict = {}
    arch = ArmCortexMCoredumpArch()
    register_list = lookup_registers_from_list(arch, info_reg_all_fixture, dummy_dict)
    for reg in arch.register_collection_list:
        # A mock read_register should have taken place so the value should be non-zero
        assert len(register_list[reg]) == 4
        assert register_list[reg][0] != 0, reg


def test_should_capture_section():
    # Never capture .text, even when NOT marked READONLY:
    assert not should_capture_section(Section(0, 10, ".text", read_only=False))

    # Never capture .debug_...
    assert not should_capture_section(Section(0, 10, ".debug_info", read_only=True))

    # Never capture 0 length sections
    assert not should_capture_section(Section(0, 0, ".dram0.data", read_only=False))

    # Always capture sections with .bss, .heap, .data or .stack in the name, even when marked READONLY:
    assert should_capture_section(Section(0, 10, ".foo.bss", read_only=True))
    assert should_capture_section(Section(0, 10, ".stack_dummy", read_only=True))
    assert should_capture_section(Section(0, 10, ".heap_psram", read_only=True))
    assert should_capture_section(Section(0, 10, ".dram0.data", read_only=True))


def test_armv7_get_used_ram_base_addresses():
    sections = (
        # Sections must meet the guess criteria:
        # 1. within an allowed region
        # 2. not read-only OR contain `.data`/`.bss` etc in the name
        Section(0x21000000, 10, "", read_only=False),
        Section(0x30000000, 10, "", read_only=True),
        Section(0x3F000000, 10, ".data.etc", read_only=True),
        Section(0x40000000, 10, "", read_only=True),
        Section(0x70000000, 10, "", read_only=True),
        Section(0x90000000, 10, "", read_only=True),
        Section(0x00000000, 10, "", read_only=True),
        Section(0xE0000000, 10, "", read_only=True),
    )

    collection_size_arm = 1 * 1024 * 1024  # 1MB
    assert [
        (0x21000000, collection_size_arm),
        (0x3F000000, collection_size_arm),
    ] == ArmCortexMCoredumpArch().guess_ram_regions(sections)


def test_read_memory_until_error_no_error():
    read_size = 1024
    size = 1024 * 1024
    inferior = MagicMock()
    inferior.read_memory.return_value = b"A" * read_size
    data = read_memory_until_error(inferior, 0x20000000, size, read_size=read_size)
    assert data == b"A" * size


def test_read_memory_until_error_after_10k():
    read_size = 1024
    size = 1024 * 1024
    inferior = MagicMock()
    total_size = 0

    def _read_memory_raise_after_10k(_addr, _size):
        nonlocal total_size
        total_size += _size
        if total_size > _size * 10:
            raise ValueError
        return b"A" * _size

    inferior.read_memory.side_effect = _read_memory_raise_after_10k
    data = read_memory_until_error(inferior, 0x20000000, size, read_size=read_size)
    assert data == b"A" * read_size * 10


def test_coredump_writer(snapshot):
    arch = ArmCortexMCoredumpArch()
    device_serial = "device_serial"
    software_type = "main"
    software_version = "1.2.3"
    hardware_revision = "gdb-proto"
    cd_writer = MemfaultCoredumpWriter(
        arch, device_serial, software_type, software_version, hardware_revision
    )
    cd_writer.trace_reason = 5
    cd_writer.regs = [  # pyright: ignore[reportGeneralTypeIssues]
        {
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
            "r10": 4 * b"\x0a",
            "r11": 4 * b"\x0b",
            "r12": 4 * b"\x0c",
            "sp": 4 * b"\x0d",
            "lr": 4 * b"\x0e",
            "pc": 4 * b"\x0f",
            "xpsr": 4 * b"\x10",
            "msp": 4 * b"\x11",
            "psp": 4 * b"\x12",
            "primask": 4 * b"\x13",
            "control": 4 * b"\x14",
        },
    ]
    section = Section(4, 32, ".test", "")  # pyright: ignore[reportGeneralTypeIssues]
    section.data = b"hello world"
    cd_writer.add_section(section)

    f_out = BytesIO()
    cd_writer.write(f_out)
    f_out.seek(0)
    snapshot.assert_match(f_out.read().hex())


def test_http_basic_auth():
    headers = add_basic_auth("martijn@memfault.com", "open_sesame", {"Foo": "Bar"})
    assert headers == {
        "Foo": "Bar",
        "Authorization": "Basic bWFydGlqbkBtZW1mYXVsdC5jb206b3Blbl9zZXNhbWU=",
    }


@pytest.fixture()
def _gdb_for_coredump(mocker, maintenance_info_sections_fixture, _settings_coredump_allowed):
    frame = MagicMock()
    mocker.patch.object(gdb_fake, "newest_frame", return_value=frame)

    def _gdb_execute(cmd, to_string):
        if cmd == "maintenance info sections":
            return maintenance_info_sections_fixture
        if cmd == "info all-registers":
            return "r0\t0\n"
        if cmd == "info threads":
            return ""
        if cmd.startswith("show arch"):
            return "The target architecture is set automatically (currently armv7m)"
        raise NotImplementedError(f"Command: {cmd}")

    mocker.patch.object(gdb_fake, "execute", side_effect=_gdb_execute)

    selected_thread = MagicMock()
    mocker.patch.object(gdb_fake, "selected_thread", return_value=selected_thread)

    inferior = MagicMock()
    inferior.threads.return_value = [selected_thread]
    mocker.patch.object(gdb_fake, "selected_inferior", return_value=inferior)

    def _read_memory(addr, size):
        return b"A" * size  # Write 41 'A' for reads to memory regions

    inferior.read_memory = _read_memory


@pytest.mark.usefixtures("_gdb_for_coredump")
def test_coredump_command_no_target(capsys):
    """Test coredump command shows error when no target is connected"""
    inferior = gdb_fake.selected_inferior()
    assert inferior
    inferior.threads.return_value = []

    cmd = MemfaultCoredump()
    cmd.invoke("--project-key {}".format(TEST_PROJECT_KEY), True)

    stdout = capsys.readouterr().out
    assert "No target" in stdout


def test_coredump_command_non_arm(mocker, capsys):
    """Test coredump command shows error when no ARM or XTENSA target"""

    def _gdb_execute(cmd, to_string):
        if cmd.startswith("show arch"):
            return "The target architecture is set automatically (currently riscv)"
        return ""

    mocker.patch.object(gdb_fake, "execute", side_effect=_gdb_execute)

    cmd = MemfaultCoredump()
    cmd.invoke("--project-key {}".format(TEST_PROJECT_KEY), True)

    stdout = capsys.readouterr().out
    assert "This command is currently only supported for ARM and XTENSA targets!" in stdout


@pytest.mark.usefixtures("_gdb_for_coredump")
def test_coredump_command_with_project_key(http_expect_request, test_config):
    """Test coredump command project key"""

    # Expect to use ingress_uri from config if not provided explicitely to the command invocation
    test_config.ingress_uri = "https://custom-ingress.memfault.com"

    http_expect_request(
        "https://custom-ingress.memfault.com/api/v0/upload/coredump",
        "POST",
        ANY,  # Not checking the request body at the moment
        {"Content-Type": "application/octet-stream", "Memfault-Project-Key": TEST_PROJECT_KEY},
        200,
        {},
    )

    cmd = MemfaultCoredump()
    cmd.invoke("--project-key {}".format(TEST_PROJECT_KEY), True)


def test_coredump_command_not_allowing(
    http_expect_request,
    capsys,
    test_config,
):
    """Test coredump command and not accepting prompt"""

    test_config.prompt = lambda prompt: "N"

    cmd = MemfaultCoredump()
    cmd.invoke("--project-key {}".format(TEST_PROJECT_KEY), True)

    stdout = capsys.readouterr().out
    assert "Aborting" in stdout


@pytest.mark.usefixtures("_gdb_for_coredump")
def test_coredump_all_overrides(http_expect_request, test_config, mocker):
    """Test coredump command with all overrides set"""

    # Verify coredump is uploaded
    test_config.ingress_uri = "https://custom-ingress.memfault.com"
    http_expect_request(
        "https://custom-ingress.memfault.com/api/v0/upload/coredump",
        "POST",
        ANY,
        {"Content-Type": "application/octet-stream", "Memfault-Project-Key": TEST_PROJECT_KEY},
        200,
        {},
    )

    writer = MagicMock()
    mocker.patch("memfault_gdb.MemfaultCoredumpWriter", writer)

    hardware_revision = "TESTREVISION"
    software_version = "TESTVERSION"
    software_type = "TESTTYPE"
    device_serial = "TESTSERIAL"

    cmd = MemfaultCoredump()
    cmd.invoke(
        "--project-key {} --hardware-revision {} --software-version {} --software-type {}"
        " --device-serial {}".format(
            TEST_PROJECT_KEY, hardware_revision, software_version, software_type, device_serial
        ),
        True,
    )

    # Verify that all args were successfully extracted and passed to writer
    writer.assert_called_once_with(
        ANY, device_serial, software_type, software_version, hardware_revision
    )


@pytest.mark.parametrize(
    ("cmd_option", "cmd_type"),
    [
        ("--hardware-revision", "hardware revision"),
        ("--software-version", "software version"),
        ("--software-type", "software type"),
        ("--device-serial", "device serial"),
    ],
)
def test_coredump_override_invalid_input(capsys, cmd_option, cmd_type):
    """Test coredump command with invalid input for each override"""

    invalid_input = 'inv"lid'

    cmd = MemfaultCoredump()
    cmd.invoke("--project-key {} {} {}".format(TEST_PROJECT_KEY, cmd_option, invalid_input), True)

    # Verify an error is thrown for the invalid input
    stderr = capsys.readouterr().err
    assert "Invalid characters in {}: {}".format(cmd_type, invalid_input) in stderr


@pytest.mark.parametrize(
    ("expected_result", "extra_cmd_args", "expected_coredump_fn"),
    [
        (None, "", "fixture2.bin"),
        (None, "-r 0x600000 8 -r 0x800000 4", "fixture3.bin"),
    ],
)
@pytest.mark.usefixtures("_gdb_for_coredump")
def test_coredump_command_with_login_no_existing_release_or_symbols(
    test_config_with_login,
    http_expect_request,
    test_config,
    expected_result,
    extra_cmd_args,
    expected_coredump_fn,
    capsys,
    snapshot,
):
    """Test coredump command without project key, but with logged in user and default org & project"""

    # Check whether Release and symbols exists -- No (404)
    http_expect_request(
        "https://api.memfault.com/api/v0/organizations/acme-inc/projects/smart-sink/software_types/main/software_versions/1.0.0-md5+b61b2d6c",
        "GET",
        ANY,
        ANY,
        404,
        None,
    )

    # Upload Symbols
    token = "token-foo"
    upload_url = "https://memfault-test-east1.s3.amazonaws.com/symbols/foo"
    http_expect_request(
        "https://api.memfault.com/api/v0/organizations/acme-inc/projects/smart-sink/upload",
        "POST",
        ANY,
        ANY,
        200,
        {
            "data": {
                "token": token,
                "upload_url": upload_url,
            }
        },
    )

    http_expect_request(
        upload_url,
        "PUT",
        ANY,
        ANY,
        200,
        None,
    )

    http_expect_request(
        "https://api.memfault.com/api/v0/organizations/acme-inc/projects/smart-sink/symbols",
        "POST",
        lambda body_bytes: json.loads(body_bytes)
        == {
            "file": {"token": token, "name": "symbols.elf"},
            "software_version": {
                "version": "1.0.0-md5+b61b2d6c",
                "software_type": "main",
            },
        },
        ANY,
        200,
        {"data": {}},
    )

    # Get Project-Key:
    http_expect_request(
        "https://api.memfault.com/api/v0/organizations/acme-inc/projects/smart-sink/api_key",
        "GET",
        ANY,
        ANY,
        200,
        {"data": {"api_key": TEST_PROJECT_KEY}},
    )

    # Upload coredump:
    def _check_request_body(body_bytes):
        # truncated in the interest of avoiding bloat in snapshot:
        snapshot.assert_match(body_bytes[:500].hex())

    http_expect_request(
        "https://ingress.memfault.com/api/v0/upload/coredump",
        "POST",
        _check_request_body,
        {"Content-Type": "application/octet-stream", "Memfault-Project-Key": TEST_PROJECT_KEY},
        200,
        {},
    )

    cmd = MemfaultCoredump()
    cmd.invoke(extra_cmd_args, True)

    stdout = capsys.readouterr().out
    assert (
        """Coredump uploaded successfully!
Once it has been processed, it will appear here:
https://app.memfault.com/organizations/acme-inc/projects/smart-sink/issues?live"""
        in stdout
    )


def test_login_command_simple(http_expect_request, test_config):
    http_expect_request(
        "https://api.memfault.com/auth/me", "GET", None, TEST_AUTH_HEADERS, 200, {"id": 123}
    )

    login = MemfaultLogin()
    login.invoke("{} {}".format(TEST_EMAIL, TEST_PASSWORD), True)

    assert test_config.email == TEST_EMAIL
    assert test_config.password == TEST_PASSWORD
    assert test_config.organization is None
    assert test_config.project is None
    assert test_config.user_id == 123


def test_login_command_with_all_options(http_expect_request, test_config):
    test_api_uri = "http://dev-api.memfault.com:8000"
    test_ingress_uri = "http://dev-ingress.memfault.com"

    http_expect_request(
        "http://dev-api.memfault.com:8000/auth/me", "GET", None, TEST_AUTH_HEADERS, 200, {"id": 123}
    )

    login = MemfaultLogin()
    login.invoke(
        "{} {} -o {} -p {} --api-uri {} --ingress-uri {}".format(
            TEST_EMAIL,
            TEST_PASSWORD,
            TEST_ORG,
            TEST_PROJECT,
            test_api_uri,
            test_ingress_uri,
        ),
        True,
    )

    assert test_config.email == TEST_EMAIL
    assert test_config.password == TEST_PASSWORD
    assert test_config.organization == TEST_ORG
    assert test_config.project == TEST_PROJECT
    assert test_config.api_uri == test_api_uri
    assert test_config.ingress_uri == test_ingress_uri


@pytest.mark.parametrize(
    ("expected_result", "resp_status", "resp_body"),
    [
        (
            True,
            200,
            {"data": {"symbol_file": {"downloadable": True}}},
        ),
        (
            False,
            200,
            {"data": {}},
        ),
        (False, 404, None),
    ],
)
def test_has_uploaded_symbols(
    http_expect_request, expected_result, resp_status, resp_body, test_config_with_login
):
    test_software_type = "main"
    test_software_version = "1.0.0"
    http_expect_request(
        "https://api.memfault.com/api/v0/organizations/{}/projects/{}/software_types/{}/software_versions/{}".format(
            test_config_with_login.organization,
            test_config_with_login.project,
            test_software_type,
            test_software_version,
        ),
        "GET",
        None,
        TEST_AUTH_HEADERS,
        resp_status,
        resp_body,
    )

    assert (
        has_uploaded_symbols(test_config_with_login, test_software_type, test_software_version)
        == expected_result
    )


def test_post_chunk_data(http_expect_request):
    base_uri = "https://example.chunks.memfault.com"
    chunk_data = bytearray([1, 2, 3, 4])
    device_serial = "GDB_TESTSERIAL"

    def _check_request_body(body_bytes):
        assert body_bytes.hex() == chunk_data.hex(), body_bytes

    http_expect_request(
        "{}/api/v0/chunks/GDB_TESTSERIAL".format(base_uri),
        "POST",
        _check_request_body,
        {"Content-Type": "application/octet-stream", "Memfault-Project-Key": TEST_PROJECT_KEY},
        202,
        None,
    )

    _post_chunk_data(
        chunk_data, project_key=TEST_PROJECT_KEY, chunks_uri=base_uri, device_serial=device_serial
    )
