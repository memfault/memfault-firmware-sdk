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
from collections import defaultdict
from enum import Enum

try:
    from elftools.elf.constants import SH_FLAGS
    from elftools.elf.elffile import ELFFile
    from elftools.elf.sections import NoteSection
except ImportError:
    raise Exception(
        """
    Script depends on pyelftools. Add it to your requirements.txt or run:
    $ pip install pyelftools
    """
    )


class SectionType(Enum):
    TEXT = 1
    DATA = 2
    BSS = 3
    UNALLOCATED = 5


class ELFFileHelper:
    def __init__(self, elf):
        """
        :param elf: An elftools.elf.elffile.ELFFile instance
        """
        self.elf = elf
        self._symtab = None

    @staticmethod
    def section_in_binary(section):
        # Only allocated sections make it into the actual binary
        sh_flags = section["sh_flags"]
        return sh_flags & SH_FLAGS.SHF_ALLOC != 0

    @staticmethod
    def build_symbol_by_name_cache(symtab, little_endian):
        # An optimized imlementation for building a cache for quick symbol info lookups
        #
        # Replacing implementation here:
        #  https://github.com/eliben/pyelftools/blob/49ffaf4/elftools/elf/sections.py#L198-L210
        #
        # Two main performance optimizations
        #   1) The "struct_parse" utility pyelftools relies on for decoding structures
        #      is extremely slow. We will use Python's struct.unpack instead here
        #   2) pyelftools passes around a file stream object while doing deserialization
        #      which means there are a ton of disk seeks that get kicked off
        #
        # Empirically, seeing about 10x performance improvement
        symtab_data = symtab.data()
        symtab_entry_size = symtab["sh_entsize"]

        symbol_name_map = defaultdict(list)

        stringtable_data = symtab.stringtable.data()

        def _get_string(start_offset):
            end_offset = stringtable_data.find(b"\x00", start_offset)
            if end_offset == -1:
                return None
            s = stringtable_data[start_offset:end_offset]
            return s.decode("utf-8", errors="replace")

        for idx in range(symtab.num_symbols()):
            entry_offset = idx * symtab_entry_size
            # The first word of a "Symbol Table Entry" is "st_name"
            # For more details, see the "Executable and Linking Format" specification
            symtab_entry_data = symtab_data[entry_offset : entry_offset + 4]

            endianess_prefix = "<" if little_endian else ">"
            st_name = struct.unpack("{}I".format(endianess_prefix), symtab_entry_data)[0]
            name = _get_string(st_name)
            symbol_name_map[name].append(idx)

        return symbol_name_map

    @property
    def symtab(self):
        # Cache the SymbolTableSection, to avoid re-parsing
        if self._symtab:
            return self._symtab
        self._symtab = self.elf.get_section_by_name(".symtab")

        # Pyelftools maintains a symbol_name to index cache (_symbol_name_map) which is extremely
        # slow to build when there are many symbols present in an ELF so we build the cache here
        # using an optimized implementation
        if self._symtab:
            self._symtab._symbol_name_map = self.build_symbol_by_name_cache(
                self._symtab, little_endian=self.elf.little_endian
            )

        return self._symtab

    def find_symbol_and_section(self, symbol_name):
        symtab = self.symtab
        if symtab is None:
            return None, None
        symbol = symtab.get_symbol_by_name(symbol_name)
        if not symbol:
            return None, None

        symbol = symbol[0]

        symbol_start = symbol["st_value"]
        symbol_size = symbol["st_size"]

        section = self.find_section_for_address_range((symbol_start, symbol_start + symbol_size))
        if section is None:
            raise BuildIdError("Could not locate a section with symbol {}".format(symbol_name))
        return (symbol, section)

    def find_section_for_address_range(self, addr_range):
        for section in self.elf.iter_sections():
            if not ELFFileHelper.section_in_binary(section):
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

    @staticmethod
    def get_symbol_offset_in_sector(symbol, section):
        return symbol["st_value"] - section["sh_addr"]

    def get_symbol_data(self, symbol, section):
        offset_in_section = self.get_symbol_offset_in_sector(symbol, section)
        symbol_size = symbol["st_size"]
        data = section.data()[offset_in_section : offset_in_section + symbol_size]
        if isinstance(data, str):  # Python 2.x
            return bytearray(data)
        return data

    def get_section_type(self, section):
        if not self.section_in_binary(section):
            return SectionType.UNALLOCATED

        sh_flags = section["sh_flags"]
        is_text = sh_flags & SH_FLAGS.SHF_EXECINSTR != 0 or sh_flags & SH_FLAGS.SHF_WRITE == 0
        if is_text:
            return SectionType.TEXT
        if section["sh_type"] != "SHT_NOBITS":
            return SectionType.DATA

        return SectionType.BSS


class MemfaultBuildIdTypes(Enum):
    # "eMemfaultBuildIdType" from "core/src/memfault_build_id_private.h"
    NONE = 1
    GNU_BUILD_ID_SHA1 = 2
    MEMFAULT_BUILD_ID_SHA1 = 3


class BuildIdError(Exception):
    pass


class BuildIdInspectorAndPatcher:
    def __init__(self, elf_file, elf=None):
        """
        :param elf_file: file object with the ELF to inspect and/or patch
        :param elf: optional, already instantiated ELFFile
        """
        self.elf_file = elf_file
        self.elf = elf or ELFFile(elf_file)
        self._helper = ELFFileHelper(self.elf)

    def _generate_build_id(self):
        build_id = hashlib.sha1()
        for section in self.elf.iter_sections():
            if not self._helper.section_in_binary(section):
                continue

            sec_start = section["sh_addr"]
            build_id.update(struct.pack("<I", sec_start))

            sec_type = self._helper.get_section_type(section)
            if sec_type == SectionType.BSS:
                # All zeros so just include the length of the section in our sha1
                length = section["sh_size"]
                build_id.update(struct.pack("<I", length))
            else:
                build_id.update(section.data())

        return build_id

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

    def _write_and_return_build_info(self, dump_only):
        sdk_build_id_sym_name = "g_memfault_build_id"
        symbol, section = self._helper.find_symbol_and_section(sdk_build_id_sym_name)
        if symbol is None:
            raise BuildIdError(
                "Could not locate '{}' symbol in provided ELF".format(sdk_build_id_sym_name)
            )

        gnu_build_id = self._get_build_id()

        # Maps to sMemfaultBuildIdStorage from "core/src/memfault_build_id_private.h"
        data = self._helper.get_symbol_data(symbol, section)

        # FW SDK's <= 0.20.1 did not encode the configured short length in the
        # "sMemfaultBuildIdStorage". In this situation the byte was a zero-initialized padding
        # byte. In this scenario we report "None" to signify we do not know the short len
        short_len = data[2] or None

        build_id_type = data[0]
        if build_id_type == MemfaultBuildIdTypes.GNU_BUILD_ID_SHA1.value:
            if gnu_build_id is None:
                raise BuildIdError(
                    "Couldn't locate GNU Build ID but 'MEMFAULT_USE_GNU_BUILD_ID' is in use"
                )

            return MemfaultBuildIdTypes.GNU_BUILD_ID_SHA1, gnu_build_id, short_len

        derived_sym_name = "g_memfault_sdk_derived_build_id"
        sdk_build_id, sdk_build_id_section = self._helper.find_symbol_and_section(derived_sym_name)
        if sdk_build_id is None:
            raise BuildIdError(
                "Could not locate '{}' symbol in provided elf".format(derived_sym_name)
            )

        data = self._helper.get_symbol_data(sdk_build_id, sdk_build_id_section)

        if build_id_type == MemfaultBuildIdTypes.MEMFAULT_BUILD_ID_SHA1.value:
            build_id = data.hex() if isinstance(data, bytes) else bytes(data).encode("hex")
            return MemfaultBuildIdTypes.MEMFAULT_BUILD_ID_SHA1, build_id, short_len

        if gnu_build_id is not None:
            print("WARNING: Located a GNU build id but it's not being used by the Memfault SDK")

        if build_id_type != MemfaultBuildIdTypes.NONE.value:
            raise BuildIdError("Unrecognized Build Id Type '{}'".format(build_id_type))

        if dump_only:
            return MemfaultBuildIdTypes.NONE, None, None

        build_id = self._generate_build_id()

        with open(self.elf_file.name, "r+b") as fh:
            build_id_type_patch_offset = section[
                "sh_offset"
            ] + self._helper.get_symbol_offset_in_sector(symbol, section)
            fh.seek(build_id_type_patch_offset)

            fh.write(struct.pack("B", MemfaultBuildIdTypes.MEMFAULT_BUILD_ID_SHA1.value))

            derived_id_patch_offset = sdk_build_id_section[
                "sh_offset"
            ] + self._helper.get_symbol_offset_in_sector(sdk_build_id, sdk_build_id_section)

            fh.seek(derived_id_patch_offset)
            fh.write(build_id.digest())
        build_id = build_id.hexdigest()
        print("Added Memfault Generated Build ID to ELF: {}".format(build_id))
        return MemfaultBuildIdTypes.MEMFAULT_BUILD_ID_SHA1, build_id, short_len

    def check_or_update_build_id(self):
        build_type, build_id, _ = self._write_and_return_build_info(dump_only=False)
        if build_type == MemfaultBuildIdTypes.GNU_BUILD_ID_SHA1:
            print("Found GNU Build ID: {}".format(build_id))
        elif build_type == MemfaultBuildIdTypes.MEMFAULT_BUILD_ID_SHA1:
            print("Found Memfault Build Id: {}".format(build_id))

    def dump_build_info(self, num_chars):
        build_type, build_id, _ = self._write_and_return_build_info(dump_only=True)
        if build_type is None or build_id is None:
            raise BuildIdError("No Build ID Found")

        print(build_id[:num_chars])

    def get_build_info(self):
        try:
            return self._write_and_return_build_info(dump_only=True)
        except BuildIdError:
            return None, None, None


def main():
    parser = argparse.ArgumentParser(
        description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter
    )
    parser.add_argument("elf", action="store")
    parser.add_argument("--dump", nargs="?", const=7, type=int)
    args = parser.parse_args()
    with open(args.elf, "rb") as elf_file:
        b = BuildIdInspectorAndPatcher(elf_file=elf_file)
        if args.dump is None:
            b.check_or_update_build_id()
        else:
            b.dump_build_info(args.dump)


if __name__ == "__main__":
    main()
