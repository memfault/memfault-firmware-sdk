//! @file
//!
//! Copyright (c) Memfault, Inc.
//! See License.txt for details
#pragma once

#define TCP_STATS 1
#define UDP_STATS 1

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

struct mock_proto {
  uint32_t xmit;
  uint32_t recv;
  uint32_t drop;
};

struct mock_stats {
  struct mock_proto tcp;
  struct mock_proto udp;
};

extern struct mock_stats lwip_stats;

#ifdef __cplusplus
}
#endif
