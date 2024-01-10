#
# Copyright (c) Memfault, Inc.
# See License.txt for details
#

# pyright: reportTypeCommentUsage=false
# This file needs to be Python 2.7 compatible (i.e. type annotations must remain in comments).
# Note: The snippet should be kept in sync with the "Example Usage" section of the README
"""Inspects provided ELF for a Build ID and when missing adds one if possible.

If a pre-existing Build ID is found (either a GNU Build ID or a Memfault Build ID),
no action is taken.

If no Build ID is found, this script will generate a unique ID by computing a SHA1 over the
contents that will be in the final binary. Once computed, the build ID will be "patched" into a
read-only struct defined in memfault-firmware-sdk/components/core/src/memfault_build_id.c to hold
the info.

If the --crc <symbol_holding_crc32> argument is used, instead of populating the Memfault Build ID
structure, the symbol specified will be updated with a CRC32 computed over the contents that will
be in the final binary.

If the --sha1 <symbol_holding_sha> argument is used, instead of populating the Memfault Build ID
structure, the symbol specified will be updated directly with Memfault SHA1 using the same strategy
discussed above. The only expectation in this mode is that a global symbol has been defined as follow:

const uint8_t g_your_symbol_build_id[20] = { 0x1, };

For further reading about Build Ids in general see:
  https://mflt.io//symbol-file-build-ids
"""

import argparse
import hashlib
import struct
import sys
import zlib
from collections import defaultdict
from enum import Enum

try:
    from typing import IO, Any, Dict, List, Optional, Tuple
except ImportError:
    if sys.version_info >= (3, 5):
        raise

try:
    from elftools.elf.constants import SH_FLAGS
    from elftools.elf.elffile import ELFFile
    from elftools.elf.sections import NoteSection, Section, Symbol, SymbolTableSection
except ImportError:
    raise ImportError(  # noqa: B904  (no raise-from in Python 2.7)
        """
    Script depends on pyelftools. Add it to your requirements.txt or run:
    $ pip install pyelftools
    """
    )


if sys.version_info < (3,):

    def hexlify(data):
        # type: (str) -> str
        return data.encode("hex")

else:

    def hexlify(data):
        # type: (bytes) -> str
        return data.hex()


SHA1_BUILD_ID_SIZE_BYTES = 20


class SectionType(Enum):
    TEXT = 1
    DATA = 2
    BSS = 3
    UNALLOCATED = 5


class ELFFileHelper:
    def __init__(self, elf):
        # type: (ELFFile) -> None
        """
        :param elf: An elftools.elf.elffile.ELFFile instance
        """
        self.elf = elf
        self._symtab = None  # type: Optional[SymbolTableSection]

    @staticmethod
    def section_in_binary(section):
        # type: (Section) -> bool
        # Only allocated sections make it into the actual binary
        sh_flags = section["sh_flags"]
        return sh_flags & SH_FLAGS.SHF_ALLOC != 0

    @staticmethod
    def build_symbol_by_name_cache(symtab, little_endian):
        # type: (SymbolTableSection, bool) -> Dict[Optional[str], List[int]]
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
            # type: (int) -> Optional[str]
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
        # type: () -> Optional[SymbolTableSection]
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
        # type: (str) -> Tuple[Optional[Symbol], Optional[Section]]
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
        return symbol, section

    def find_section_for_address_range(self, addr_range):
        # type: (Tuple[int, int]) -> Optional[NoteSection]
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
        # type: (Symbol, Section) -> int
        return symbol["st_value"] - section["sh_addr"]

    def get_symbol_data(self, symbol, section):
        # type: (Any, Section) -> bytes
        offset_in_section = self.get_symbol_offset_in_sector(symbol, section)
        symbol_size = symbol["st_size"]
        return section.data()[offset_in_section : offset_in_section + symbol_size]

    def get_section_type(self, section):
        # type: (Section) -> SectionType
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
    def __init__(self, elf_file, elf=None, elf_helper=None):
        # type: (IO[bytes], Optional[ELFFile], Optional[ELFFileHelper]) -> None
        """
        :param elf_file: file object with the ELF to inspect and/or patch
        :param elf: optional, already instantiated ELFFile
        :param elf_helper: optional, already instantiated ELFFileHelper
        """
        self.elf_file = elf_file
        self.elf = (elf_helper.elf if elf_helper else elf) or ELFFile(elf_file)
        self._helper = elf_helper or ELFFileHelper(self.elf)

    def _generate_build_id(self, sha1_symbol_section=None, sha1_symbol_section_offset=0):
        # type: (Optional[Section], int) -> hashlib._Hash
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
                data = section.data()
                if sha1_symbol_section and sha1_symbol_section["sh_addr"] == section["sh_addr"]:
                    build_id.update(data[:sha1_symbol_section_offset])
                    # NB: When a Memfault Build ID is generated as part of a memfault-firmware-sdk
                    # integration we SHA1 over the compile time state of
                    # "g_memfault_sdk_derived_build_id"... simulate the same behavior when updating
                    # a SHA1 symbol directly.
                    build_id.update(bytearray([1] + [0] * 19))
                    build_id.update(data[sha1_symbol_section_offset + SHA1_BUILD_ID_SIZE_BYTES :])
                else:
                    build_id.update(data)

        return build_id

    def _get_build_id(self):
        # type: () -> Optional[str]
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

    def check_or_update_sha1_build_id(self, sha1_sym_name, dump_only):
        symbol, section = self._helper.find_symbol_and_section(sha1_sym_name)
        if symbol is None or section is None:
            raise BuildIdError("Could not locate '{}' symbol in provided ELF".format(sha1_sym_name))

        if symbol["st_size"] != SHA1_BUILD_ID_SIZE_BYTES:
            raise BuildIdError(
                "A build ID should be {} bytes in size".format(SHA1_BUILD_ID_SIZE_BYTES)
            )

        current_sha1_bytes = bytearray(self._helper.get_symbol_data(symbol, section))

        symbol_offset = self._helper.get_symbol_offset_in_sector(symbol, section)

        sha1_hash = self._generate_build_id(
            sha1_symbol_section=section, sha1_symbol_section_offset=symbol_offset
        )
        sha1_hash_bytes = sha1_hash.digest()

        if current_sha1_bytes == sha1_hash_bytes:
            print(
                "Memfault Generated SHA1 Build ID at '{}': {}".format(
                    sha1_sym_name, sha1_hash.hexdigest()
                )
            )
            return sha1_hash

        if dump_only:
            print("Memfault Build ID at '{}' is not written".format(sha1_sym_name))
            return sha1_hash

        with open(self.elf_file.name, "r+b") as fh:
            derived_id_patch_offset = section[
                "sh_offset"
            ] + self._helper.get_symbol_offset_in_sector(symbol, section)

            fh.seek(derived_id_patch_offset)
            fh.write(sha1_hash_bytes)

        print("Added Memfault Generated SHA1 Build ID to ELF: {}".format(sha1_hash.hexdigest()))

        # Return the sha1 computed in case someone wants to run this as a library
        return sha1_hash

    def _write_and_return_build_info(self, dump_only):
        # type: (bool) -> Tuple[MemfaultBuildIdTypes, Optional[str], Optional[int]]
        sdk_build_id_sym_name = "g_memfault_build_id"
        symbol, section = self._helper.find_symbol_and_section(sdk_build_id_sym_name)
        if symbol is None or section is None:
            raise BuildIdError(
                "Could not locate '{}' symbol in provided ELF".format(sdk_build_id_sym_name)
            )

        gnu_build_id = self._get_build_id()

        # Maps to sMemfaultBuildIdStorage from "core/src/memfault_build_id_private.h"
        data = bytearray(self._helper.get_symbol_data(symbol, section))

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
        if sdk_build_id is None or sdk_build_id_section is None:
            raise BuildIdError(
                "Could not locate '{}' symbol in provided elf".format(derived_sym_name)
            )

        if build_id_type == MemfaultBuildIdTypes.MEMFAULT_BUILD_ID_SHA1.value:
            build_id_bytes = self._helper.get_symbol_data(sdk_build_id, sdk_build_id_section)
            return MemfaultBuildIdTypes.MEMFAULT_BUILD_ID_SHA1, hexlify(build_id_bytes), short_len

        if gnu_build_id is not None:
            print("WARNING: Located a GNU build id but it's not being used by the Memfault SDK")

        if build_id_type != MemfaultBuildIdTypes.NONE.value:
            raise BuildIdError("Unrecognized Build Id Type '{}'".format(build_id_type))

        if dump_only:
            return MemfaultBuildIdTypes.NONE, None, None

        build_id_hash = self._generate_build_id()

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
            fh.write(build_id_hash.digest())

        build_id = build_id_hash.hexdigest()
        print("Added Memfault Generated Build ID to ELF: {}".format(build_id))
        return MemfaultBuildIdTypes.MEMFAULT_BUILD_ID_SHA1, build_id, short_len

    def check_or_update_build_id(self):
        # type: () -> None
        build_type, build_id, _ = self._write_and_return_build_info(dump_only=False)
        if build_type == MemfaultBuildIdTypes.GNU_BUILD_ID_SHA1:
            print("Found GNU Build ID: {}".format(build_id))
        elif build_type == MemfaultBuildIdTypes.MEMFAULT_BUILD_ID_SHA1:
            print("Found Memfault Build Id: {}".format(build_id))

    def dump_build_info(self, num_chars):
        # type: (int) -> None
        build_type, build_id, _ = self._write_and_return_build_info(dump_only=True)
        if build_type is None or build_id is None:
            raise BuildIdError("No Build ID Found")

        print(build_id[:num_chars])

    def _generate_crc32_build_id(
        self, crc32_symbol_section, crc32_symbol_section_offset, crc32_symbol_length=4
    ):
        # type: (Section, int, int) -> int
        crc32 = 0

        for section in self.elf.iter_sections():
            if not self._helper.section_in_binary(section):
                continue

            sec_start = section["sh_addr"]
            crc32 = zlib.crc32(struct.pack("<I", sec_start), crc32) & 0xFFFFFFFF

            sec_type = self._helper.get_section_type(section)
            if sec_type == SectionType.BSS:
                # All zeros so just include the length of the section in our sha1
                length = section["sh_size"]
                crc32 = zlib.crc32(struct.pack("<I", length), crc32) & 0xFFFFFFFF
            else:
                data = section.data()

                # Note: We don't include the crc32 symbol location in the CRC32 so that re-running
                # this script on an ELF yields a stable result
                if section["sh_addr"] == crc32_symbol_section["sh_addr"]:
                    crc32 = zlib.crc32(data[:crc32_symbol_section_offset], crc32) & 0xFFFFFFFF
                    crc32 = (
                        zlib.crc32(data[crc32_symbol_section_offset + crc32_symbol_length :], crc32)
                        & 0xFFFFFFFF
                    )
                else:
                    crc32 = zlib.crc32(data, crc32) & 0xFFFFFFFF

        return crc32

    def check_or_update_crc_build_id(self, crc_symbol_name, dump_only=False):
        # type: (str, bool) -> None
        symbol, section = self._helper.find_symbol_and_section(crc_symbol_name)
        if symbol is None or section is None:
            raise BuildIdError(
                "Could not locate '{}' CRC symbol in provided ELF".format(crc_symbol_name)
            )

        sec_type = self._helper.get_section_type(section)
        if sec_type not in {SectionType.TEXT, SectionType.DATA}:
            raise BuildIdError(
                "CRC symbol '{}' in invalid Section '{}'".format(crc_symbol_name, sec_type)
            )

        section_offset = section["sh_offset"]
        symbol_offset = self._helper.get_symbol_offset_in_sector(symbol, section)
        symbol_length = 4

        crc_build_id = self._generate_crc32_build_id(section, symbol_offset, symbol_length)

        endianess_prefix = "<" if self.elf.little_endian else ">"
        current_crc_build_id = struct.unpack(
            "{}I".format(endianess_prefix),
            section.data()[symbol_offset : symbol_offset + symbol_length],
        )[0]

        if current_crc_build_id == crc_build_id:
            print(
                "CRC32 Generated Build ID at '{}' to ELF already written: {}".format(
                    crc_symbol_name, hex(crc_build_id)
                )
            )
            return

        if dump_only:
            print("CRC32 Build ID at '{}' is not written".format(crc_symbol_name))
            return

        crc_build_id_bytes = struct.pack("{}I".format(endianess_prefix), crc_build_id)

        with open(self.elf_file.name, "r+b") as fh:
            derived_id_patch_offset = section_offset + symbol_offset

            fh.seek(derived_id_patch_offset)
            fh.write(crc_build_id_bytes)

        print(
            "Added CRC32 Generated Build ID at '{}' to ELF: {}".format(
                crc_symbol_name, hex(crc_build_id)
            )
        )

    def get_build_info(self):
        # type: () -> Tuple[Optional[MemfaultBuildIdTypes], Optional[str], Optional[int]]
        try:
            return self._write_and_return_build_info(dump_only=True)
        except BuildIdError:
            return None, None, None


def main():
    # type: () -> None
    parser = argparse.ArgumentParser(
        description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter
    )
    parser.add_argument("elf", action="store")
    parser.add_argument("--dump", nargs="?", const=7, type=int)
    parser.add_argument("--crc", action="store")
    parser.add_argument("--sha1", action="store")

    args = parser.parse_args()

    if args.sha1 and args.crc:
        raise RuntimeError("Only one of 'crc' or 'sha1' can be specified")

    with open(args.elf, "rb") as elf_file:
        b = BuildIdInspectorAndPatcher(elf_file=elf_file)

        if args.sha1:
            b.check_or_update_sha1_build_id(args.sha1, dump_only=args.dump)
        elif args.crc:
            b.check_or_update_crc_build_id(args.sha1, dump_only=args.dump)
        elif args.dump is None:
            if args.crc is None:
                b.check_or_update_build_id()
        else:
            b.dump_build_info(args.dump)


if __name__ == "__main__":
    main()
