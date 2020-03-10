#pragma once

//! @file
//!
//! Copyright (c) Memfault, Inc.
//! See License.txt for details
//!
//! @brief
//! esp-idf port specific functions related to http

#include <stddef.h>

#define MEMFAULT_HTTP_CLIENT_MIN_BUFFER_SIZE 1024

//! Called to get a buffer to use for POSTing data to the Memfault cloud
//!
//! @note The default implementation just calls malloc but is defined
//! as weak so a end user can easily override the implementation
//!
//! @param buffer_size[out] Filled with the size of the buffer allocated. The expectation is that
//! the buffer will be >= MEMFAULT_HTTP_CLIENT_MIN_BUFFER_SIZE. The larger the buffer, the less
//! amount of POST requests that will be needed to publish data
//!
//! @return Allocated buffer or NULL on failure
void *memfault_http_client_allocate_chunk_buffer(size_t *buffer_size);


//! Called to release the buffer that was being to POST data to the Memfault cloud
//!
//! @note The default implementation just calls malloc but defined
//! as weak so an end user can easily override the implementation
void memfault_http_client_release_chunk_buffer(void *buffer);
