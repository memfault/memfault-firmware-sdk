#pragma once

//! @file
//!
//! Copyright (c) Memfault, Inc.
//! See License.txt for details
//!
//! Sample CDR implementation. See https://mflt.io/custom-data-recordings for
//! more information

#include "memfault/components.h"

//! Defined by the module, must be registered with the Memfault SDK on system
//! initialization.
extern const sMemfaultCdrSourceImpl g_custom_data_recording_source;
