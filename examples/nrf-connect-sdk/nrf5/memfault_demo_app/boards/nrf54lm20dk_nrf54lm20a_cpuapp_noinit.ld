/*
  Demonstrate storing reboot event data in a fixed location of noninit.

  The nRF54LM20 has 0x7fc00 bytes of RAM starting at 0x20000000. Reserve the
  last 0xc00 bytes in devicetree and link this section into that dedicated
  memory region so .bss can still start at the base of normal SRAM.
*/

SECTION_PROLOGUE(memfault_noinit, (NOLOAD),)
{
    KEEP(*(*.memfault_reboot_info));
    KEEP(*(*.memfault_coredump));
} GROUP_DATA_LINK_IN(LINKER_DT_NODE_REGION_NAME(DT_NODELABEL(memfault_noinit_sram)),
                     LINKER_DT_NODE_REGION_NAME(DT_NODELABEL(memfault_noinit_sram)))
