#pragma once

//! @file
//!
//! @brief
//! Math helpers

#define MEMFAULT_ARRAY_SIZE(arr) (sizeof(arr) / sizeof(arr[0]))

#define MEMFAULT_MIN(a, b)   (((a) < (b)) ? (a) : (b))
#define MEMFAULT_MAX(a , b)  (((a) > (b)) ? (a) : (b))
#define MEMFAULT_FLOOR(a, align) (((a) / (align)) * (align))
