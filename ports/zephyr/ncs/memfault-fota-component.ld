#include <zephyr/linker/iterable_sections.h>

// Use a literal subalign of 4, matching Zephyr's 32-bit iterable sections
ITERABLE_SECTION_ROM(MemfaultFotaComponent, 4)
