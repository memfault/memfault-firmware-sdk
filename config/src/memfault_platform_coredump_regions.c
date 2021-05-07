//! @file memfault_port_coredump_regions.c

#include "memfault/config.h"
#include "memfault/core/compiler.h"
#include "memfault/core/math.h"
#include "memfault/components.h"
#include "memfault/panics/platform/coredump.h"

#include "sdk_defs.h"

#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#if defined(HPY_MEMFAULT_RAM_COREDUMP_ENABLED)
//! holds the RAM regions to collect in a coredump
__RETAINED_UNINIT MEMFAULT_ALIGNED(8)
static uint8_t s_ram_backed_coredump_region[HPY_MEMFAULT_COREDUMP_SIZE];
#endif //#if defined(HPY_MEMFAULT_RAM_COREDUMP_ENABLED)

//! Truncates the region if it's outside the bounds of RAM
size_t memfault_platform_sanitize_address_range(void *start_addr, size_t desired_size)
{
    // This defines the valid RAM range and is pulled from sdk_defs.h.
    const uint32_t ram_start = MEMORY_SYSRAM_BASE;
    const uint32_t ram_end = MEMORY_SYSRAM_END;

    if ((uint32_t)start_addr >= ram_start && (uint32_t)start_addr < ram_end) {
        return MEMFAULT_MIN(desired_size, ram_end - (uint32_t )start_addr);
    }
    return 0;
}

//! The RAM regions to collect in a coredump capture
const sMfltCoredumpRegion *memfault_platform_coredump_get_regions(
    const sCoredumpCrashInfo *crash_info, size_t *num_regions)
{
    static sMfltCoredumpRegion s_coredump_regions[1];

    s_coredump_regions[0] =
        MEMFAULT_COREDUMP_MEMORY_REGION_INIT(
            (void *)MEMORY_SYSRAM_BASE,
            (uint32_t) memfault_platform_sanitize_address_range((void *)MEMORY_SYSRAM_BASE, HPY_MEMFAULT_COREDUMP_SIZE));
    *num_regions = MEMFAULT_ARRAY_SIZE(s_coredump_regions);
    return &s_coredump_regions[0];
}

#if defined(HPY_MEMFAULT_RAM_COREDUMP_ENABLED)
void memfault_platform_coredump_storage_get_info(sMfltCoredumpStorageInfo *info)
{
    *info = (sMfltCoredumpStorageInfo ) { .size = HPY_MEMFAULT_COREDUMP_SIZE,
                                          .sector_size = HPY_MEMFAULT_COREDUMP_SIZE, };
}

bool memfault_platform_coredump_storage_read(uint32_t offset, void *data, size_t read_len)
{
    if ((offset + read_len) > HPY_MEMFAULT_COREDUMP_SIZE) {
        return false;
    }

    const uint8_t *read_ptr = &s_ram_backed_coredump_region[offset];
    memcpy(data, read_ptr, read_len);
    return true;
}

bool memfault_platform_coredump_storage_erase(uint32_t offset, size_t erase_size)
{
    if ((offset + erase_size) > HPY_MEMFAULT_COREDUMP_SIZE) {
        return false;
    }

    uint8_t *erase_ptr = &s_ram_backed_coredump_region[offset];
    memset(erase_ptr, 0x0, erase_size);
    return true;
}

bool memfault_platform_coredump_storage_write(uint32_t offset, const void *data, size_t data_len)
{
    if ((offset + data_len) > HPY_MEMFAULT_COREDUMP_SIZE) {
        return false;
    }

    uint8_t *write_ptr = &s_ram_backed_coredump_region[offset];
    memcpy(write_ptr, data, data_len);
    return true;
}

void memfault_platform_coredump_storage_clear(void)
{
    memfault_platform_coredump_storage_erase(0, HPY_MEMFAULT_COREDUMP_SIZE);
}

#endif //#if defined(HPY_MEMFAULT_RAM_COREDUMP_ENABLED)
