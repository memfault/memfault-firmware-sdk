/*
  Custom section to be included in the linker command file to support recovery of all FreeRTOS tasks when
  a coredump is collected.
*/

SECTIONS
{
    .mflt_bss (NOLOAD) :
    {
        __memfault_capture_bss_start = .;
         *tasks.o(.bss COMMON .bss*)
         *timers*.o(.bss COMMON .bss*)
        __memfault_capture_bss_end = .;
    }
}
INSERT BEFORE .bss;
