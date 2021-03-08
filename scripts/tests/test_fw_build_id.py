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

test_dir = os.path.dirname(os.path.realpath(__file__))
script_dir = os.path.dirname(test_dir)
sys.path.append(script_dir)
ELF_FIXTURES_DIR = os.path.join(test_dir, "elf_fixtures")

from fw_build_id import BuildIdInspectorAndPatcher  # noqa isort:skip


@contextlib.contextmanager
def temporary_filename(file_to_copy=None):
    f = tempfile.NamedTemporaryFile(delete=False)
    try:
        tmp_name = f.name
        f.close()

        if file_to_copy is not None:
            shutil.copy2(file_to_copy, tmp_name)

        yield tmp_name
    finally:
        os.unlink(tmp_name)


def test_gnu_build_id_in_use(capsys, snapshot):
    elf_fixture_filename = os.path.join(ELF_FIXTURES_DIR, "gnu_id_present_and_used.elf")

    with temporary_filename(elf_fixture_filename) as elf_copy_filename:
        b = BuildIdInspectorAndPatcher(elf_copy_filename)
        b.check_or_update_build_id()

        assert filecmp.cmp(elf_copy_filename, elf_fixture_filename)

    out, _ = capsys.readouterr()
    lines = out.splitlines()
    snapshot.assert_match(lines)


def test_gnu_build_id_present_but_not_used(capsys, snapshot):
    elf_fixture_filename = os.path.join(ELF_FIXTURES_DIR, "gnu_id_present_and_not_used.elf")
    result_fixture_filename = os.path.join(ELF_FIXTURES_DIR, "memfault_id_used_gnu_id_present.elf")

    with temporary_filename(elf_fixture_filename) as elf_copy_filename:
        b = BuildIdInspectorAndPatcher(elf_copy_filename)
        b.check_or_update_build_id()

        assert filecmp.cmp(elf_copy_filename, result_fixture_filename)

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

    with temporary_filename(elf_fixture_filename) as elf_copy_filename:
        b = BuildIdInspectorAndPatcher(elf_copy_filename)
        b.check_or_update_build_id()

        assert filecmp.cmp(elf_copy_filename, result_fixture_filename)

    out, _ = capsys.readouterr()
    lines = out.splitlines()
    snapshot.assert_match(lines)


def test_memfault_id_populated(capsys, snapshot):
    elf_fixture_filename = os.path.join(
        ELF_FIXTURES_DIR, "memfault_build_id_present_and_populated.elf"
    )

    with temporary_filename(elf_fixture_filename) as elf_copy_filename:
        b = BuildIdInspectorAndPatcher(elf_copy_filename)
        b.check_or_update_build_id()

        assert filecmp.cmp(elf_copy_filename, elf_fixture_filename)

    out, _ = capsys.readouterr()
    lines = out.splitlines()
    snapshot.assert_match(lines)


def test_no_memfault_sdk_present():
    elf_fixture_filename = os.path.join(ELF_FIXTURES_DIR, "no_memfault_symbols.elf")
    with pytest.raises(
        Exception, match="Could not locate 'g_memfault_build_id' symbol in provided ELF"
    ):
        b = BuildIdInspectorAndPatcher(elf_fixture_filename)
        b.check_or_update_build_id()


def test_no_build_id_on_dump():
    elf_fixture_filename = os.path.join(
        ELF_FIXTURES_DIR, "memfault_build_id_present_and_unpopulated.elf"
    )
    with pytest.raises(Exception, match="No Build ID Found"):
        b = BuildIdInspectorAndPatcher(elf_fixture_filename)
        b.dump_build_info(num_chars=1)


def test_build_id_dump(capsys, snapshot):
    elf_fixture_filename = os.path.join(
        ELF_FIXTURES_DIR, "memfault_build_id_present_and_populated.elf"
    )
    with temporary_filename(elf_fixture_filename) as elf_copy_filename:
        b = BuildIdInspectorAndPatcher(elf_copy_filename)
        b.dump_build_info(num_chars=1)
        b.dump_build_info(num_chars=2)
        b.dump_build_info(num_chars=30)

        assert filecmp.cmp(elf_copy_filename, elf_fixture_filename)

    out, _ = capsys.readouterr()
    lines = out.splitlines()
    snapshot.assert_match(lines)
