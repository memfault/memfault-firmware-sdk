#pragma once

#include <stdbool.h>
#include <stddef.h>

//! Tests whether a coredump is present that has been captured
//! using esp-idf/components/esp32/core_dump.c and gets the size of the core dump data.
//! @param[out] size_out After returning will be updated to the size of the core dump data.
//! @return true if an esp-idf formatted core dump was found or false otherwise.
bool memfault_coredump_get_esp_idf_formatted_coredump_size(size_t *size_out);
