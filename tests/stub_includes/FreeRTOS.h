//! @file
//!
//! Copyright (c) Memfault, Inc.
//! See License.txt for details
#pragma once

#ifdef TEST_FREERTOS_METRICS
  #define configGENERATE_RUN_TIME_STATS 1
  #define configNUM_CORES 1
  #define configUSE_TRACE_FACILITY 1
#endif
