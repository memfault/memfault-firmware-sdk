//! @file
//!
//! Copyright (c) Memfault, Inc.
//! See License.txt for details
//!
//! @brief
//! Public interface for self_test component
#pragma once

#ifdef __cplusplus
extern "C" {
#endif

//! Main entry point to performing a self test
//!
//! This function will carry out all component tests and return after completing all tests
//! See output for test results and feedback on any errors detected
//!
//! @returns 0 on success or 1 if a subtest failed
int memfault_self_test_run(void);

#ifdef __cplusplus
}
#endif
