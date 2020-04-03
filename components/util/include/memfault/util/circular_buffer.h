#pragma once

//! @file
//!
//! Copyright (c) Memfault, Inc.
//! See License.txt for details
//!
//! @brief
//! A simple circular buffer implementation.
//!
//! Note: the implementation does not have any locking. If the user is accessing the buffer
//! from multiple contexts, it is their responsibility to lock things

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

//! Structure tracking circular buffer state. In header for convenient static allocation but it
//! should never be accessed directly!
typedef struct {
  size_t read_offset;
  size_t read_size;
  size_t total_space;
  uint8_t *storage;
} sMfltCircularBuffer;

//! Called to initialize circular buffer context
//!
//! @param circular_buffer Allocated context for circular buffer tracking
//! @param storage_buf storage area that will be used by circular buffer
//! @param storage_len Size of storage area
//!
//! @return true if successfully configured, else false
bool memfault_circular_buffer_init(sMfltCircularBuffer *circular_buf, void *storage_buf,
                                   size_t storage_len);

//! Read the requested number of bytes
//!
//! @param circular_buffer The buffer to read from
//! @param offset The offset within the buffer to start reading at (must be less than @ref
//!   memfault_circular_buffer_get_read_size())
//! @param data The buffer to copy read data into
//! @param data_len The amount of data to read and the size of space available in data
//!
//! @return true if the requested data_len was read, false otherwise (i.e trying to read more bytes
//! than are available or past the end of available bytes)
bool memfault_circular_buffer_read(sMfltCircularBuffer *circular_buf, size_t offset,
                                   void *data, size_t data_len);

//! Populates the read_ptr with a set of contigous bytes which can be read.
//!
//! This can be useful to avoid the need of doing double buffering or implementing something like a
//! circular buffer array
//!
//! @param circular_buf The buffer to read from
//! @param offset The offset to start reading from
//! @param data Populated with the pointer to read from
//! @param data_len The length which can be read
//!
//! @return true if a read pointer was successfully populated
bool memfault_circular_buffer_get_read_pointer(sMfltCircularBuffer *circular_buf, size_t offset,
                                               uint8_t **read_ptr, size_t *read_ptr_len);

//! Flush the requested number of bytes from the circular buffer
//!
//! @param circular_buffer The buffer to clear bytes from
//! @param data_len The number of bytes to consume (must be less then @ref
//! memfault_circular_buffer_get_read_size)
//!
//! @return true if the bytes were consumed, false otherwise (i.e trying to consume more bytes than
//!   exist)
bool memfault_circular_buffer_consume(sMfltCircularBuffer *circular_buf, size_t consume_len);


//! Same as "memfault_circular_buffer_consume" but flush the requested number of bytes from
//! the _end_ of the circular buffer
//!
//! This can be useful for lazily aborting a write that spans multiple write calls
//! where the entire size of the write is not known upfront
bool memfault_circular_buffer_consume_from_end(sMfltCircularBuffer *circular_buf,
                                               size_t consume_len);

//! Copy data into the circular buffer
//!
//! @param circular_buffer The buffer to clear bytes from
//! @param data The buffer to copy
//! @param data_len Length of buffer to copy
//!
//! @return true if there was enough space and the _entire_ buffer was copied, false otherwise.
//! Note if there is not enough space, _no_ data will be written.
bool memfault_circular_buffer_write(sMfltCircularBuffer *circular_buf, const void *data,
                                    size_t data_len);

//! Copy data into the circular buffer starting at the provided offset from the end
//!
//! @param circular_buffer The buffer to clear bytes from
//! @param offset_from_end Where to begin the write. For example if 10 bytes were written to the buffer
//!  and offset_from_end is 1, a write will begin at offset 9 within the buffer. offset_from_end must
//! be less than or equal to the current amount of bytes currently stored in the buffer
//! @param data The buffer to copy
//! @param data_len Length of buffer to copy
//!
//! @return true if there was enough space and the _entire_ buffer was copied, false otherwise
bool memfault_circular_buffer_write_at_offset(
    sMfltCircularBuffer *circular_buf, size_t offset_from_end, const void *data, size_t data_len);

//! @return Amount of bytes available to read
size_t memfault_circular_buffer_get_read_size(const sMfltCircularBuffer *circular_buf);

//! @return Amount of bytes available for writing
size_t memfault_circular_buffer_get_write_size(const sMfltCircularBuffer *circular_buf);

#ifdef __cplusplus
}
#endif
