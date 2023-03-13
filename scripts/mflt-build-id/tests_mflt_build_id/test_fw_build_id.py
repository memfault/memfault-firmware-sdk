#
# Copyright (c) Memfault, Inc.
# See License.txt for details
#

import filecmp
import os
import shutil
import sys

import pytest

from .elf_fixtures import ELF_FIXTURES_DIR

test_dir = os.path.dirname(os.path.realpath(__file__))
script_dir = os.path.dirname(test_dir)
sys.path.append(script_dir)


from mflt_build_id import (  # noqa: E402 # isort:skip
    BuildIdInspectorAndPatcher,
    MemfaultBuildIdTypes,
)


@pytest.fixture()
def copy_file(tmp_path):
    """Copies a file into the tests tmp path"""
    idx = [0]

    def _copy_file(src):
        # NB: Python 2.7 does not support `nonlocal`
        tmp_name = str(tmp_path / "file_{}.bin".format(idx[0]))
        idx[0] += 1
        shutil.copy2(src, tmp_name)
        return tmp_name

    return _copy_file


def test_gnu_build_id_in_use(capsys, copy_file):
    elf_fixture_filename = os.path.join(ELF_FIXTURES_DIR, "gnu_id_present_and_used.elf")

    with open(copy_file(elf_fixture_filename), mode="rb") as elf_copy_file:
        b = BuildIdInspectorAndPatcher(elf_copy_file)
        b.check_or_update_build_id()

        assert filecmp.cmp(elf_copy_file.name, elf_fixture_filename)

    out, _ = capsys.readouterr()
    lines = out.splitlines()
    assert lines == ["Found GNU Build ID: 3d6f95306bbc5fda4183728b3829f88b30f7aa1c"]


def test_gnu_build_id_present_but_not_used(capsys, copy_file):
    elf_fixture_filename = os.path.join(ELF_FIXTURES_DIR, "gnu_id_present_and_not_used.elf")
    result_fixture_filename = os.path.join(ELF_FIXTURES_DIR, "memfault_id_used_gnu_id_present.elf")

    with open(copy_file(elf_fixture_filename), mode="rb") as elf_copy_file:
        b = BuildIdInspectorAndPatcher(elf_copy_file)
        b.check_or_update_build_id()

        assert filecmp.cmp(elf_copy_file.name, result_fixture_filename)

    out, _ = capsys.readouterr()
    lines = out.splitlines()
    assert lines == [
        "WARNING: Located a GNU build id but it's not being used by the Memfault SDK",
        "Added Memfault Generated Build ID to ELF: 028aa9800f0524c9b064711925df5ec403df6b16",
        "Found Memfault Build Id: 028aa9800f0524c9b064711925df5ec403df6b16",
    ]


def test_memfault_id_unpopulated(capsys, copy_file):
    elf_fixture_filename = os.path.join(
        ELF_FIXTURES_DIR, "memfault_build_id_present_and_unpopulated.elf"
    )
    result_fixture_filename = os.path.join(
        ELF_FIXTURES_DIR, "memfault_build_id_present_and_populated.elf"
    )

    with open(copy_file(elf_fixture_filename), mode="rb") as elf_copy_file:
        b = BuildIdInspectorAndPatcher(elf_copy_file)
        b.check_or_update_build_id()

        assert filecmp.cmp(elf_copy_file.name, result_fixture_filename)

    out, _ = capsys.readouterr()
    lines = out.splitlines()
    assert lines == [
        "Added Memfault Generated Build ID to ELF: 16e0fe39af176cfa4cf961321ccf5193c2590451",
        "Found Memfault Build Id: 16e0fe39af176cfa4cf961321ccf5193c2590451",
    ]


def test_memfault_sha1_unpopulated(capsys, copy_file):
    elf_fixture_filename = os.path.join(
        ELF_FIXTURES_DIR, "memfault_build_id_present_and_unpopulated.elf"
    )

    with open(copy_file(elf_fixture_filename), mode="rb") as elf_copy_file:
        # attempt to dump build id before it has been written
        b = BuildIdInspectorAndPatcher(elf_copy_file)
        sha1 = b.check_or_update_sha1_build_id("g_memfault_sdk_derived_build_id", dump_only=True)
        assert sha1.hexdigest() == "16e0fe39af176cfa4cf961321ccf5193c2590451"

        # actually write the build id
        b = BuildIdInspectorAndPatcher(elf_copy_file)
        sha1 = b.check_or_update_sha1_build_id("g_memfault_sdk_derived_build_id", dump_only=False)
        assert sha1.hexdigest() == "16e0fe39af176cfa4cf961321ccf5193c2590451"

        # We've updated the file -- force a reload of anything that is cached
        elf_copy_file.flush()

        # confirm that patching the SHA1 build id is idempotent
        b = BuildIdInspectorAndPatcher(elf_copy_file)
        sha1 = b.check_or_update_sha1_build_id("g_memfault_sdk_derived_build_id", dump_only=False)
        assert sha1.hexdigest() == "16e0fe39af176cfa4cf961321ccf5193c2590451"

        # confirm once build id is written we dump the info correctly
        b = BuildIdInspectorAndPatcher(elf_copy_file)
        sha1 = b.check_or_update_sha1_build_id("g_memfault_sdk_derived_build_id", dump_only=True)
        assert sha1.hexdigest() == "16e0fe39af176cfa4cf961321ccf5193c2590451"

    out, _ = capsys.readouterr()
    lines = out.splitlines()

    assert lines == [
        "Memfault Build ID at 'g_memfault_sdk_derived_build_id' is not written",
        "Added Memfault Generated SHA1 Build ID to ELF: 16e0fe39af176cfa4cf961321ccf5193c2590451",
        "Memfault Generated SHA1 Build ID at 'g_memfault_sdk_derived_build_id': 16e0fe39af176cfa4cf961321ccf5193c2590451",
        "Memfault Generated SHA1 Build ID at 'g_memfault_sdk_derived_build_id': 16e0fe39af176cfa4cf961321ccf5193c2590451",
    ]


def test_memfault_sha1_wrong_symbol(capsys, copy_file):
    elf_fixture_filename = os.path.join(
        ELF_FIXTURES_DIR, "memfault_build_id_present_and_unpopulated.elf"
    )

    with open(copy_file(elf_fixture_filename), mode="rb") as elf_copy_file:
        b = BuildIdInspectorAndPatcher(elf_copy_file)

        # test symbol does not exist case
        with pytest.raises(
            Exception, match="Could not locate 'g_wrong_symbol_name' symbol in provided ELF"
        ):
            b.check_or_update_sha1_build_id("g_wrong_symbol_name", dump_only=False)

        # test symbol exists but it's the wrong size
        with pytest.raises(Exception, match="A build ID should be 20 bytes in size"):
            b.check_or_update_sha1_build_id("g_memfault_build_id", dump_only=False)


def test_memfault_id_populated(capsys, copy_file):
    elf_fixture_filename = os.path.join(
        ELF_FIXTURES_DIR, "memfault_build_id_present_and_populated.elf"
    )

    with open(copy_file(elf_fixture_filename), mode="rb") as elf_copy_file:
        b = BuildIdInspectorAndPatcher(elf_copy_file)
        b.check_or_update_build_id()

        assert filecmp.cmp(elf_copy_file.name, elf_fixture_filename)

    out, _ = capsys.readouterr()
    lines = out.splitlines()
    assert lines == ["Found Memfault Build Id: 16e0fe39af176cfa4cf961321ccf5193c2590451"]


def test_no_memfault_sdk_present():
    elf_fixture_filename = os.path.join(ELF_FIXTURES_DIR, "no_memfault_symbols.elf")
    with open(elf_fixture_filename, "rb") as elf_fixture_file:
        b = BuildIdInspectorAndPatcher(elf_fixture_file)
        with pytest.raises(
            Exception, match="Could not locate 'g_memfault_build_id' symbol in provided ELF"
        ):
            b.check_or_update_build_id()


def test_no_build_id_on_dump():
    elf_fixture_filename = os.path.join(
        ELF_FIXTURES_DIR, "memfault_build_id_present_and_unpopulated.elf"
    )
    with open(elf_fixture_filename, "rb") as elf_fixture_file:
        b = BuildIdInspectorAndPatcher(elf_fixture_file)
        with pytest.raises(Exception, match="No Build ID Found"):
            b.dump_build_info(num_chars=1)


def test_build_id_dump(capsys, copy_file):
    elf_fixture_filename = os.path.join(
        ELF_FIXTURES_DIR, "memfault_build_id_present_and_populated.elf"
    )
    with open(copy_file(elf_fixture_filename), mode="rb") as elf_copy_file:
        b = BuildIdInspectorAndPatcher(elf_copy_file)
        b.dump_build_info(num_chars=1)
        b.dump_build_info(num_chars=2)
        b.dump_build_info(num_chars=30)

        assert filecmp.cmp(elf_copy_file.name, elf_fixture_filename)

    out, _ = capsys.readouterr()
    lines = out.splitlines()
    assert lines == ["1", "16", "16e0fe39af176cfa4cf961321ccf51"]


@pytest.mark.parametrize(
    ("fixture", "expected_result"),
    [
        (
            "memfault_build_id_present_and_populated.elf",
            (
                MemfaultBuildIdTypes.MEMFAULT_BUILD_ID_SHA1,
                "16e0fe39af176cfa4cf961321ccf5193c2590451",
                None,
            ),
        ),
        (
            "no_memfault_symbols.elf",
            # BuildIdException is caught and None, None is returned:
            (None, None, None),
        ),
        (
            "memfault_build_id_with_short_len.elf",
            (
                MemfaultBuildIdTypes.MEMFAULT_BUILD_ID_SHA1,
                "ef960811c7e83525457dc76722bbbb38f362769c",
                7,
            ),
        ),
        (
            "gnu_id_with_short_len.elf",
            (
                MemfaultBuildIdTypes.GNU_BUILD_ID_SHA1,
                "152121637a37ec8f8b7c3a29b8708b705e3350cb",
                7,
            ),
        ),
        (
            "gnu_id_present_and_not_used.elf",
            (
                MemfaultBuildIdTypes.NONE,
                None,
                None,
            ),
        ),
        (
            "no_symtab_no_text_no_data.elf",
            (
                None,
                None,
                None,
            ),
        ),
    ],
)
def test_get_build_info(fixture, expected_result, copy_file):
    elf_fixture_filename = os.path.join(ELF_FIXTURES_DIR, fixture)
    with open(copy_file(elf_fixture_filename), mode="rb") as elf_copy_file:
        b = BuildIdInspectorAndPatcher(elf_copy_file)
        assert b.get_build_info() == expected_result

        assert filecmp.cmp(elf_copy_file.name, elf_fixture_filename)


def test_crc_build_id_unpopulated(capsys, copy_file):
    elf_fixture_filename = os.path.join(ELF_FIXTURES_DIR, "crc32_build_id_unpopulated.elf")
    result_fixture_filename = os.path.join(ELF_FIXTURES_DIR, "crc32_build_id_populated.elf")

    with open(copy_file(elf_fixture_filename), mode="rb") as elf_copy_file:
        b = BuildIdInspectorAndPatcher(elf_copy_file)
        b.check_or_update_crc_build_id("g_example_crc32_build_id")

        assert filecmp.cmp(elf_copy_file.name, result_fixture_filename)

    out, _ = capsys.readouterr()
    lines = out.splitlines()
    assert lines == [
        "Added CRC32 Generated Build ID at 'g_example_crc32_build_id' to ELF: 0x8ac1e1ca"
    ]


def test_crc_build_id_unpopulated_dump_only(capsys, copy_file):
    elf_fixture_filename = os.path.join(ELF_FIXTURES_DIR, "crc32_build_id_unpopulated.elf")

    with open(copy_file(elf_fixture_filename), mode="rb") as elf_copy_file:
        b = BuildIdInspectorAndPatcher(elf_copy_file)
        b.check_or_update_crc_build_id("g_example_crc32_build_id", dump_only=True)

        # in dump only mode, nothing should change even if there isn't a build id written
        assert filecmp.cmp(elf_copy_file.name, elf_fixture_filename)

    out, _ = capsys.readouterr()
    lines = out.splitlines()
    assert lines == ["CRC32 Build ID at 'g_example_crc32_build_id' is not written"]


def test_crc_build_id_populated(capsys, copy_file):
    elf_fixture_filename = os.path.join(ELF_FIXTURES_DIR, "crc32_build_id_populated.elf")

    with open(copy_file(elf_fixture_filename), mode="rb") as elf_copy_file:
        b = BuildIdInspectorAndPatcher(elf_copy_file)
        b.check_or_update_crc_build_id("g_example_crc32_build_id")

        assert filecmp.cmp(elf_copy_file.name, elf_fixture_filename)

    out, _ = capsys.readouterr()
    lines = out.splitlines()
    assert lines == [
        "CRC32 Generated Build ID at 'g_example_crc32_build_id' to ELF already written: 0x8ac1e1ca"
    ]


def test_crc_build_id_in_bss():
    elf_fixture_filename = os.path.join(ELF_FIXTURES_DIR, "crc32_build_id_unpopulated.elf")
    with open(elf_fixture_filename, "rb") as elf_fixture_file:
        b = BuildIdInspectorAndPatcher(elf_fixture_file)
        with pytest.raises(
            Exception,
            match="CRC symbol 'g_example_crc32_bss_build_id' in invalid Section 'SectionType.BSS'",
        ):
            b.check_or_update_crc_build_id("g_example_crc32_bss_build_id")


def test_crc_build_id_symbol_dne():
    elf_fixture_filename = os.path.join(ELF_FIXTURES_DIR, "crc32_build_id_unpopulated.elf")
    with open(elf_fixture_filename, "rb") as elf_fixture_file:
        b = BuildIdInspectorAndPatcher(elf_fixture_file)
        with pytest.raises(
            Exception,
            match="Could not locate 'g_nonexistent_bss_build_id' CRC symbol in provided ELF",
        ):
            b.check_or_update_crc_build_id("g_nonexistent_bss_build_id")
