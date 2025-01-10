/*
  Using the Memfault RAM-backed coredump storage on the nRF54 series requires
  placing the noinit section at an arbitrarily higher location in RAM, to
  prevent the bootloader from overwriting it.
*/

SECTION_PROLOGUE (memfault_noinit, 0x20020000 (NOLOAD),)
{
    KEEP(*(*.memfault_reboot_info));
    KEEP(*(*.memfault_coredump));
} GROUP_DATA_LINK_IN(RAMABLE_REGION, RAMABLE_REGION)
