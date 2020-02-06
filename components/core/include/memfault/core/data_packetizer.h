#pragma once

//! @file
//!
//! Copyright (c) Memfault, Inc.
//! See License.txt for details
//!
//! @brief
//! API for packetizing the data stored by the Memfault SDK (such as coredumps)
//! so that the data can be transported up to the Memfault cloud
//!
//! For a step-by-step walkthrough of the API, check out the documentation:
//!   https://mflt.io/data-to-cloud

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

//! Fills buffer with a chunk when there is data available
//!
//! NOTE: This is the simplest way to interact with the packetizer. The API call returns a single
//! "chunk" to be forwarded out over the transport topology to the Memfault cloud. For more
//! advanced control over chunking, the lower level APIs exposed below in this module can be used.
//!
//! @param[out] buf The buffer to copy data to be sent into
//! @param[in,out] buf_len The size of the buffer to copy data into. On return, populated
//! with the amount of data, in bytes, that was copied into the buffer. If a buffer with a length
//! less than MEMFAULT_PACKETIZER_MIN_BUF_LEN is passed, no data will be copied and the size returned
//! will be 0.
//!
//! @return true if the buffer was filled, false otherwise
bool memfault_packetizer_get_chunk(void *buf, size_t *buf_len);

typedef enum {
  //! Indicates there is no more data to be sent at this time
  kMemfaultPacketizerStatus_NoMoreData = 0,

  //! Indicates that an entire chunk has been returned. By default, every call to
  //! memfault_packetizer_get_next() that returns data will be a complete "Chunk"
  kMemfaultPacketizerStatus_EndOfChunk,

  //! Indicates there is more data to be received for the chunk. This will _only_ be returned
  //! as a value if multi packet chunking has been enabled by a call to
  //! memfault_packetizer_enable_multi_packet_chunks()
  kMemfaultPacketizerStatus_MoreDataForChunk,
} eMemfaultPacketizerStatus;

//! The _absolute_ minimum a buffer passed into memfault_packetizer_get_next() can be in order to
//! receive data. However, it's recommended you use a buffer size that matches the MTU of your
//! transport path.
#define MEMFAULT_PACKETIZER_MIN_BUF_LEN 9

//! @return true if there is data available to send, false otherwise
//!
//! This can be used to check if there is any data to send prior to opening a connection over the
//! underlying transport to the internet
bool memfault_packetizer_data_available(void);

typedef struct {
  //! When false, memfault_packetizer_get_next() will always return a single "chunk"
  //! when data is available that can be pushed directly to the Memfault cloud
  //!
  //! When true, memfault_packetizer_get_next() may have to be called multiple times to return a
  //! single chunk. This can be used as an optimization for system which support sending or
  //! re-assembling larger messages over their transport.
  //!
  //! @note You can find a reference example in the reference example using the WICED http stack
  //! (wiced/libraries/memfault/platform_reference_impl/memfault_platform_http_client.c)
  //! @note In this mode, it's the API users responsibility to make sure they push the chunk data
  //! only when a kMemfaultPacketizerStatus_EndOfChunk is received
  bool enable_multi_packet_chunk;
} sPacketizerConfig;

typedef struct {
  //! true if the packetizer has partially sent an underlying message. Calls to
  //! memfault_packetizer_get_next() will continue to return the next packets of the message to
  //! send. false if no parts of a message have been sent yet.
  bool send_in_progress;

  //! The size of the message when sent as a single chunk. This can be useful when
  //! using transports which require the entire size of the binary blob be known up front
  //! (i.e the "Content-Length" in an http request)
  uint32_t single_chunk_message_length;
} sPacketizerMetadata;

//! @return true if there is data available to send, false otherwise.
bool memfault_packetizer_begin(const sPacketizerConfig *cfg, sPacketizerMetadata *metadata_out);

//! Fills the provided buffer with data to be sent.
//!
//! @note It's expected that memfault_packetizer_begin() is called prior to invoking this call for
//! the first time and each time kMemfaultPacketizerStatus_EndOfChunk is returned. If it is not called
//! this routine will return kMemfaultPacketizerStatus_NoMoreData
//! @note It's expected that this API will be called periodically (i.e when a connection to the internet
//! becomes available) to drain data collected by Memfault up to the cloud.
//! @note To completely drain the data "memfault_packetizer_get_next()" must be called until the
//! function returns kMemfaultPacketizerStatus_NoMoreData. It is _not_ required that all the data be
//! drained at once. It is perfectly fine to drain data a little bit at a time each time a
//! connection to the internet becomes available on your device.
//!
//! @note It is expected that the customer has a mechanism to send the packets returned from
//! this API up to the cloud _reliably_ and _in-order_.
//! @note The api is not threadsafe. The expectation is that a user will drain data from a single thread
//! or otherwise wrap the call with a mutex
//!
//! @param[out] buf The buffer to copy data to be sent into
//! @param[in,out] buf_len The size of the buffer to copy data into. On return, populated
//! with the amount of data, in bytes, that was copied into the buffer. If a buffer with a length
//! less than MEMFAULT_PACKETIZER_MIN_BUF_LEN is passed, no data will be copied and the size returned
//! will be 0.
//!
//! @return The status of the packetization. See comments in enum for more details.
eMemfaultPacketizerStatus memfault_packetizer_get_next(void *buf, size_t *buf_len);

//! Abort any in-progress message packetizations
//!
//! For example, if packets being sent got dropped or failed to send, it would make sense to abort
//! the transaction and re-send the data
//!
//! @note This will cause any partially written pieces of data to be re-transmitted in their
//! entirety (i.e coredump)
void memfault_packetizer_abort(void);

#ifdef __cplusplus
}
#endif
