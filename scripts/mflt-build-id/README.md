# Memfault Build ID Tool

This package contains the `mflt_build_id` CLI tool.

The purpose of the tool is simplify reading or writing
[Build IDs](https://interrupt.memfault.com/blog/gnu-build-id-for-firmware) in a
firmware image irrespective of the compiler or build system being used in a
project.

## Example Usage

```
$ mflt_build_id --help
usage: mflt_build_id [-h] [--dump [DUMP]] [--crc CRC] [--sha1 SHA1] elf

Inspects provided ELF for a Build ID and when missing adds one if possible.

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

positional arguments:
  elf

options:
  -h, --help     show this help message and exit
  --dump [DUMP]
  --crc CRC
  --sha1 SHA1
```
