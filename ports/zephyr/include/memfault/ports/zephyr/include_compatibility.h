#pragma once

//! @file
//!
//! Copyright (c) Memfault, Inc.
//! See License.txt for details

#ifdef __cplusplus
extern "C" {
#endif

#include "memfault/ports/zephyr/version.h"

/*
 * Workaround for Zephyr include path changes
 * NB: This macro and it's usage should be removed after we remove support for < Zephyr v3.2
 * NB: The clang-format blocks are required because clang-format inserts spaces which muck up the
 * token concatenation
 *
 * Background: Zephyr changed its include path in version v3.2 from no subpath within include/ to
 * adding a Zephyr namespace directory include/zephyr/ There was a brief period v3.2 - v3.3 when a
 * Kconfig existed to bridge support before complete deprecation. We instead opted to fixup the
 * paths within our CMake code. However this can lead to problems if customers have headers with
 * the same name as the Zephyr headers (hence the reason Zephyr changed the paths in the first
 * place). We still support Zephyr versions prior to v3.2 so we need a way to use the old header
 * paths.
 *
 * Implementation: Using this SO post as reference/inspiration
 * (https://stackoverflow.com/questions/32066204/construct-path-for-include-directive-with-macro)
 * the macros below build up a token that can be used within a #include directive. This macro
 * should be used with the exact strings provided to your typical #include
 * <zephyr/path/to/header.h> statements.
 */

// clang-format off
// Used to create a token from the input
#define MEMFAULT_IDENT(x) x
// Encloses contents with <> using preprocessor token concatenation
#define MEMFAULT_ENCLOSE_ARROW_BRACKETS(contents) \
  MEMFAULT_IDENT(<)MEMFAULT_IDENT(contents)MEMFAULT_IDENT(>)

#if MEMFAULT_ZEPHYR_VERSION_GT(3, 1) && !defined(CONFIG_LEGACY_INCLUDE_PATH)
  // Concatenates contents with zephyr/ to form a single token
  #define PREPEND_ZEPHYR(contents) MEMFAULT_IDENT(zephyr/)MEMFAULT_IDENT(contents)

  // Creates a token which prepends zephyr/ to path and encloses in <>
  #define MEMFAULT_ZEPHYR_INCLUDE(header_path) \
    MEMFAULT_ENCLOSE_ARROW_BRACKETS(PREPEND_ZEPHYR(header_path))
#else
  #define MEMFAULT_ZEPHYR_INCLUDE(header_path) MEMFAULT_ENCLOSE_ARROW_BRACKETS(header_path)
#endif
// clang-format on

#ifdef __cplusplus
}
#endif
