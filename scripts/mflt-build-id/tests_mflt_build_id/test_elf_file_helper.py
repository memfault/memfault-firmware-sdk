#
# Copyright (c) Memfault, Inc.
# See License.txt for details
#

import os
import sys

import elftools.elf.sections
from elftools.elf.elffile import ELFFile

from .elf_fixtures import ELF_FIXTURES_DIR

test_dir = os.path.dirname(os.path.realpath(__file__))
script_dir = os.path.dirname(test_dir)
sys.path.append(script_dir)

from mflt_build_id import ELFFileHelper  # noqa: E402 # isort:skip


class TestELFFileHelper:
    def test_basic(self):
        elf_fixture_filename = os.path.join(ELF_FIXTURES_DIR, "gnu_id_present_and_used.elf")

        with open(elf_fixture_filename, "rb") as f:
            helper = ELFFileHelper(ELFFile(f))

            symbol, section = helper.find_symbol_and_section("g_memfault_build_id")
            assert isinstance(symbol, elftools.elf.sections.Symbol)
            assert isinstance(section, elftools.elf.sections.Section)
            assert symbol.name == "g_memfault_build_id"

            data = helper.get_symbol_data(symbol, section)
            assert isinstance(data, bytes)
