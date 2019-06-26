/*
 * Defines __MfltCoredumpRamStart and __MfltCoredumpRamEnd that point to beginning and end of the SRAM region.
 */

SECTIONS
{
  .mflt_coredump_symbols :
  {
    __MfltCoredumpRamStart = ORIGIN(SRAM);
    __MfltCoredumpRamEnd = ORIGIN(SRAM) + LENGTH(SRAM);
  }
}
