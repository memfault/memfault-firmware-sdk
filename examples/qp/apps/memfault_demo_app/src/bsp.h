#pragma once

//! @file
//!
//! Copyright (c) Memfault, Inc.
//! See License.txt for details
//!
//! @brief Minimal BSP for Memfault demo app with QP on stm32f4xx

#include <stdbool.h>

#define BSP_TICKS_PER_SEC (100)

//! Initializes the BSP.
void bsp_init(void);

//! Synchronously sends a character.
int bsp_send_char_over_uart(char c);

//! Synchronously receives a character.
bool bsp_read_char_from_uart(char *out_char);
