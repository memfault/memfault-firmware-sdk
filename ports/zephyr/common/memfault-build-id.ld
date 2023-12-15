SECTION_PROLOGUE(.note.gnu.build-id,,)
{
  __start_gnu_build_id_start = .;
  KEEP(*(.note.gnu.build-id))
} GROUP_ROM_LINK_IN(RODATA_REGION, ROMABLE_REGION)
