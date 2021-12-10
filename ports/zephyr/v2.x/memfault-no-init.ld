/**
 * Memfault stores some state across reboots. These regions must be placed in a region of RAM that
 * does not get initialized on bootup
 */

     SECTION_PROLOGUE (.memfault_noinit, (NOLOAD),)
    {
        __shared_ram_start = .;
        KEEP(*(SORT_BY_NAME(".memfault_noinit*")))
        __shared_ram_end = .;
    } > DTCM
