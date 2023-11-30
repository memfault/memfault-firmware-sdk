//! @file
//!
//! Copyright (c) Memfault, Inc.
//! See License.txt for details

#include "lwip/stats.h"
#include "memfault/metrics/metrics.h"
#include "memfault/ports/lwip/metrics.h"

// Macro used to calculate appropriate diff for the protocol stats, update the metric values, and
// reset static values.
//
// Instead of resetting the counter values to 0, a difference is calculated to prevent concurrency
// issues. We take the previous counter value and subtract from the current value. This way only
// reads are performed on the stat counters in this context. The difference calculations rely on two
// things:
// 1. The counters are monotonically increasing
// 2. The values will never wrap more than once before a heartbeat
#define SET_PROTOCOL_METRICS(proto_struct, proto_name, proto_metric_prefix)       \
  do {                                                                            \
    uint32_t tx_diff = proto_struct.xmit - s_##proto_name##_tx_prev;              \
    uint32_t rx_diff = proto_struct.recv - s_##proto_name##_rx_prev;              \
    uint32_t drop_diff = proto_struct.drop - s_##proto_name##_drop_prev;          \
                                                                                  \
    MEMFAULT_METRIC_SET_UNSIGNED(proto_metric_prefix##_tx_count, tx_diff);     \
    MEMFAULT_METRIC_SET_UNSIGNED(proto_metric_prefix##_rx_count, rx_diff);     \
    MEMFAULT_METRIC_SET_UNSIGNED(proto_metric_prefix##_drop_count, drop_diff); \
                                                                                  \
    s_##proto_name##_tx_prev += tx_diff;                                          \
    s_##proto_name##_rx_prev += rx_diff;                                          \
    s_##proto_name##_drop_prev += drop_diff;                                      \
  } while (0)

void memfault_lwip_heartbeat_collect_data(void) {
#if TCP_STATS
  static uint32_t s_tcp_tx_prev = 0;
  static uint32_t s_tcp_rx_prev = 0;
  static uint32_t s_tcp_drop_prev = 0;
  SET_PROTOCOL_METRICS(lwip_stats.tcp, tcp, tcp);
#endif  // TCP_STATS

#if UDP_STATS
  static uint32_t s_udp_tx_prev = 0;
  static uint32_t s_udp_rx_prev = 0;
  static uint32_t s_udp_drop_prev = 0;
  SET_PROTOCOL_METRICS(lwip_stats.udp, udp, udp);
#endif  // UDP_STATS
}
