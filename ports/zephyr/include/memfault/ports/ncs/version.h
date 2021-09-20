#pragma once

//! @file
//!
//! Copyright (c) Memfault, Inc.
//! See License.txt for details
//!
//! @brief A little convenience header to assist in checks which can be run at compile time for
//! backward compatibility based on NCS version.

#ifdef __cplusplus
extern "C" {
#endif

//! NCS Version was introduced in nRF Connect SDK >= 1.4
#if __has_include("ncs_version.h")

#include "ncs_version.h"

//! modem/bsdlib.h was introduced in nRF Connect SDK 1.3
#elif __has_include("modem/bsdlib.h")

#define NCS_VERSION_MAJOR 1
#define NCS_VERSION_MINOR 3
#define NCS_PATCHLEVEL 0

#else

//! The lowest version the memfault-firmware-sdk has been ported to
#define NCS_VERSION_MAJOR 1
#define NCS_VERSION_MINOR 2
#define NCS_PATCHLEVEL 0

#endif

//! Returns true if current nRF Connect Version is greater than the one specified
//!
//! Three checks:
//!  - First check if major version is greater than the one specified
//!  - Next check if the major version matches and the minor version is greater
//!  - Finally check if we are on a development build that is greater than the version.  After a
//!    release is shipped, a PATCHLEVEL of 99 is used to indicate this. For example, a version of
//!    "1.7.99" means development for version "1.8.0".
#define MEMFAULT_NCS_VERSION_GT(major, minor) \
  ((NCS_VERSION_MAJOR > (major)) ||                                     \
   ((NCS_VERSION_MAJOR == (major)) && (NCS_VERSION_MINOR > (minor))) || \
   ((NCS_VERSION_MAJOR == (major)) && ((NCS_VERSION_MINOR + 1) > (minor)) && (NCS_PATCHLEVEL == 99)))

#ifdef __cplusplus
}
#endif
