SECTION_PROLOGUE(.note.gnu.build-id,,)
{
  __start_gnu_build_id_start = .;
  KEEP(*(.note.gnu.build-id))

/* For ESP32s, GROUP_LINK_IN() results in the placement of the build id at
 * a bad address but GROUP_DATA_LINK_IN() gives it a reasonable one.
 * GROUP_DATA_LINK_IN() is newer to Zephyr so it may not catch all cases;
 * #ifdef on the ESP32 family for now.
 */
#if defined(CONFIG_SOC_FAMILY_ESP32)
} GROUP_DATA_LINK_IN(RODATA_REGION, ROMABLE_REGION)
#else
} GROUP_LINK_IN(ROMABLE_REGION)
#endif
