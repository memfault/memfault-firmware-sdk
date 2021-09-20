#
# Copyright (c) Memfault, Inc.
# See License.txt for details
#

import contextlib
import filecmp
import os
import shutil
import sys
import tempfile

import pytest

from .elf_fixtures import ELF_FIXTURES_DIR

test_dir = os.path.dirname(os.path.realpath(__file__))
script_dir = os.path.dirname(test_dir)
sys.path.append(script_dir)


from mflt_build_id import BuildIdInspectorAndPatcher, MemfaultBuildIdTypes  # noqa isort:skip


@contextlib.contextmanager
def open_copy(file_to_copy, *args, **kwargs):
    f = tempfile.NamedTemporaryFile(delete=False)
    try:
        tmp_name = f.name
        f.close()

        if file_to_copy is not None:
            shutil.copy2(file_to_copy, tmp_name)

        with open(tmp_name, *args, **kwargs) as file:
            yield file
    finally:
        os.unlink(tmp_name)


def test_gnu_build_id_in_use(capsys, snapshot):
    elf_fixture_filename = os.path.join(ELF_FIXTURES_DIR, "gnu_id_present_and_used.elf")

    with open_copy(elf_fixture_filename, mode="rb") as elf_copy_file:
        b = BuildIdInspectorAndPatcher(elf_copy_file)
        b.check_or_update_build_id()

        assert filecmp.cmp(elf_copy_file.name, elf_fixture_filename)

    out, _ = capsys.readouterr()
    lines = out.splitlines()
    snapshot.assert_match(lines)


def test_gnu_build_id_present_but_not_used(capsys, snapshot):
    elf_fixture_filename = os.path.join(ELF_FIXTURES_DIR, "gnu_id_present_and_not_used.elf")
    result_fixture_filename = os.path.join(ELF_FIXTURES_DIR, "memfault_id_used_gnu_id_present.elf")

    with open_copy(elf_fixture_filename, mode="rb") as elf_copy_file:
        b = BuildIdInspectorAndPatcher(elf_copy_file)
        b.check_or_update_build_id()

        assert filecmp.cmp(elf_copy_file.name, result_fixture_filename)

    out, _ = capsys.readouterr()
    lines = out.splitlines()
    snapshot.assert_match(lines)


def test_memfault_id_unpopulated(capsys, snapshot):
    elf_fixture_filename = os.path.join(
        ELF_FIXTURES_DIR, "memfault_build_id_present_and_unpopulated.elf"
    )
    result_fixture_filename = os.path.join(
        ELF_FIXTURES_DIR, "memfault_build_id_present_and_populated.elf"
    )

    with open_copy(elf_fixture_filename, mode="rb") as elf_copy_file:
        b = BuildIdInspectorAndPatcher(elf_copy_file)
        b.check_or_update_build_id()

        assert filecmp.cmp(elf_copy_file.name, result_fixture_filename)

    out, _ = capsys.readouterr()
    lines = out.splitlines()
    snapshot.assert_match(lines)


def test_memfault_id_populated(capsys, snapshot):
    elf_fixture_filename = os.path.join(
        ELF_FIXTURES_DIR, "memfault_build_id_present_and_populated.elf"
    )

    with open_copy(elf_fixture_filename, mode="rb") as elf_copy_file:
        b = BuildIdInspectorAndPatcher(elf_copy_file)
        b.check_or_update_build_id()

        assert filecmp.cmp(elf_copy_file.name, elf_fixture_filename)

    out, _ = capsys.readouterr()
    lines = out.splitlines()
    snapshot.assert_match(lines)


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


def test_build_id_dump(capsys, snapshot):
    elf_fixture_filename = os.path.join(
        ELF_FIXTURES_DIR, "memfault_build_id_present_and_populated.elf"
    )
    with open_copy(elf_fixture_filename, mode="rb") as elf_copy_file:
        b = BuildIdInspectorAndPatcher(elf_copy_file)
        b.dump_build_info(num_chars=1)
        b.dump_build_info(num_chars=2)
        b.dump_build_info(num_chars=30)

        assert filecmp.cmp(elf_copy_file.name, elf_fixture_filename)

    out, _ = capsys.readouterr()
    lines = out.splitlines()
    snapshot.assert_match(lines)


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
def test_get_build_info(fixture, expected_result):
    elf_fixture_filename = os.path.join(ELF_FIXTURES_DIR, fixture)
    with open_copy(elf_fixture_filename, mode="rb") as elf_copy_file:
        b = BuildIdInspectorAndPatcher(elf_copy_file)
        assert b.get_build_info() == expected_result

        assert filecmp.cmp(elf_copy_file.name, elf_fixture_filename)
