#
# Copyright (c) Memfault, Inc.
# See License.txt for details
#

"""Inspects provided ELF for a Build ID and when missing adds one if possible.

If a pre-existing Build ID is found (either a GNU Build ID or a Memfault Build ID),
no action is taken.

If no Build ID is found, this script will generate a unique ID by computing a SHA1 over the
contents that will be in the final binary. Once computed, the build ID will be "patched"
into a read-only struct defined in memfault/core/memfault_build_id.c to hold the info.
"""

import argparse
import hashlib
import struct
from enum import Enum

try:
    from elftools.elf.constants import SH_FLAGS
    from elftools.elf.elffile import ELFFile
    from elftools.elf.sections import NoteSection
except ImportError as e:
    raise Exception(
        """
    Script depends on pyelftools. Add it to your requirements.txt or run:
    $ pip install pyelftools
    """
    ) from e


class SectionType(Enum):
    TEXT = 1
    DATA = 2
    BSS = 3
    UNALLOCATED = 5


class MemfaultBuildIdTypes(Enum):
    # "eMemfaultBuildIdType" from "core/src/memfault_build_id_private.h"
    NONE = 1
    GNU_BUILD_ID_SHA1 = 2
    MEMFAULT_BUILD_ID_SHA1 = 3


class BuildIdInspectorAndPatcher:
    def __init__(self, elf_file):
        self.elf_file = elf_file
        self.elf = ELFFile(open(elf_file, "rb"))

    @staticmethod
    def _section_in_binary(section):
        # Only allocated sections make it into the actual binary
        sh_flags = section["sh_flags"]
        return sh_flags & SH_FLAGS.SHF_ALLOC != 0

    def _get_section_type(self, section):
        if not self._section_in_binary(section):
            return SectionType.UNALLOCATED

        sh_flags = section["sh_flags"]
        is_text = sh_flags & SH_FLAGS.SHF_EXECINSTR != 0 or sh_flags & SH_FLAGS.SHF_WRITE == 0
        if is_text:
            return SectionType.TEXT
        if section["sh_type"] != "SHT_NOBITS":
            return SectionType.DATA

        return SectionType.BSS

    def _find_section_for_address_range(self, addr_range):
        for section in self.elf.iter_sections():
            if not self._section_in_binary(section):
                continue

            sec_start = section["sh_addr"]
            sh_size = section["sh_size"]
            sec_end = sec_start + sh_size

            addr_start, addr_end = addr_range

            range_in_section = (sec_start <= addr_start < sec_end) and (
                sec_start <= addr_end <= sec_end
            )

            if not range_in_section:
                continue

            return section
        return None

    def _find_symbol_and_section(self, symbol_name):
        symtab = self.elf.get_section_by_name(".symtab")
        symbol = symtab.get_symbol_by_name(symbol_name)
        if not symbol:
            return None, None

        symbol = symbol[0]

        symbol_start = symbol["st_value"]
        symbol_size = symbol["st_size"]

        section = self._find_section_for_address_range((symbol_start, symbol_start + symbol_size))
        if section is None:
            raise Exception("Could not locate a section with symbol {}".format(symbol_name))
        return (symbol, section)

    def _generate_build_id(self):
        build_id = hashlib.sha1()
        for section in self.elf.iter_sections():
            if not self._section_in_binary(section):
                continue

            sec_start = section["sh_addr"]
            build_id.update(struct.pack("<I", sec_start))

            sec_type = self._get_section_type(section)
            if sec_type == SectionType.BSS:
                # All zeros so just include the length of the section in our sha1
                length = section["sh_size"]
                build_id.update(struct.pack("<I", length))
            else:
                build_id.update(section.data())

        return build_id

    @staticmethod
    def _get_symbol_offset_in_sector(symbol, section):
        return symbol["st_value"] - section["sh_addr"]

    def _get_symbol_data(self, symbol, section):
        offset_in_section = self._get_symbol_offset_in_sector(symbol, section)
        symbol_size = symbol["st_size"]
        data = section.data()[offset_in_section : offset_in_section + symbol_size]
        if isinstance(data, str):
            return bytearray(data)
        return data

    def _get_build_id(self):
        def _get_note_sections(elf):
            for section in elf.iter_sections():
                if not isinstance(section, NoteSection):
                    continue
                for note in section.iter_notes():
                    yield note

        for note in _get_note_sections(self.elf):
            if note.n_type == "NT_GNU_BUILD_ID":
                return note.n_desc

        return None

    def check_or_update_build_id(self):
        sdk_build_id_sym_name = "g_memfault_build_id"
        symbol, section = self._find_symbol_and_section(sdk_build_id_sym_name)
        if symbol is None:
            raise Exception(
                "Could not locate '{}' symbol in provided ELF".format(sdk_build_id_sym_name)
            )

        gnu_build_id = self._get_build_id()

        # sMemfaultBuildIdStorage
        data = self._get_symbol_data(symbol, section)
        build_id_type = data[0]
        if build_id_type == MemfaultBuildIdTypes.GNU_BUILD_ID_SHA1.value:
            if gnu_build_id is None:
                raise Exception(
                    "Couldn't locate GNU Build ID but 'MEMFAULT_USE_GNU_BUILD_ID' is in use"
                )

            print("Found GNU Build ID: {}".format(gnu_build_id))
            return

        derived_sym_name = "g_memfault_sdk_derived_build_id"
        sdk_build_id, sdk_build_id_section = self._find_symbol_and_section(derived_sym_name)
        if sdk_build_id is None:
            raise Exception("Could not locate '{}' symbol in provided elf".format(derived_sym_name))

        data = self._get_symbol_data(sdk_build_id, sdk_build_id_section)

        if build_id_type == MemfaultBuildIdTypes.MEMFAULT_BUILD_ID_SHA1.value:
            build_id = data.hex() if isinstance(data, bytes) else bytes(data).encode("hex")
            print("Found Memfault Build Id: {}".format(build_id))
            return

        if gnu_build_id is not None:
            print("WARNING: Located a GNU build id but it's not being used by the Memfault SDK")

        if build_id_type != MemfaultBuildIdTypes.NONE.value:
            raise Exception("Unrecognized Build Id Type '{}'".format(build_id_type))

        build_id = self._generate_build_id()

        with open(self.elf_file, "r+b") as fh:
            build_id_type_patch_offset = section["sh_offset"] + self._get_symbol_offset_in_sector(
                symbol, section
            )
            fh.seek(build_id_type_patch_offset)
            fh.write(bytes([MemfaultBuildIdTypes.MEMFAULT_BUILD_ID_SHA1.value]))

            derived_id_patch_offset = sdk_build_id_section[
                "sh_offset"
            ] + self._get_symbol_offset_in_sector(sdk_build_id, sdk_build_id_section)

            fh.seek(derived_id_patch_offset)
            fh.write(build_id.digest())
        print("Added Memfault Generated Build ID to ELF: {}".format(build_id.hexdigest()))


if __name__ == "__main__":
    parser = argparse.ArgumentParser(
        description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter
    )
    parser.add_argument("elf", action="store")
    args = parser.parse_args()

    b = BuildIdInspectorAndPatcher(elf_file=args.elf)
    b.check_or_update_build_id()
