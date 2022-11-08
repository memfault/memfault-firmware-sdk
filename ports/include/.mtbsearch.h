//! @file
//!
//! Copyright (c) Memfault, Inc.
//! See License.txt for details
//!
//! This file is required so that this path is still included when mtbsearch generates cyqbuild.mk
//!
//! For issues debugging the include path check the generate output in the build/ directory
//! of a MTB application.
//!
//! Here's snippet for searching for memfault related include paths, run in the same directory as
//! inclist.rsp: `cat inclist.rsp | tr " " "\n" | rg -e memfault-firmware-sdk`
#ifdef __cplusplus
extern "C" {
#endif

#ifdef __cplusplus
}
#endif
