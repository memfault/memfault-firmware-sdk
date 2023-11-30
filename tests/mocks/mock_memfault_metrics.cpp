#include "CppUTestExt/MockSupport.h"
#include "memfault/metrics/metrics.h"

int memfault_metrics_heartbeat_set_unsigned(MemfaultMetricId key, uint32_t unsigned_value) {
  return mock()
    .actualCall(__func__)
    .withParameterOfType("MemfaultMetricId", "key", &key)
    .withParameter("unsigned_value", unsigned_value)
    .returnIntValue();
}

int memfault_metrics_heartbeat_set_signed(MemfaultMetricId key, int32_t signed_value) {
  return mock()
    .actualCall(__func__)
    .withParameterOfType("MemfaultMetricId", "key", &key)
    .withParameter("signed_value", signed_value)
    .returnIntValue();
}

int memfault_metrics_heartbeat_timer_start(MemfaultMetricId key) {
  return mock()
    .actualCall(__func__)
    .withParameterOfType("MemfaultMetricId", "key", &key)
    .returnIntValue();
}

int memfault_metrics_heartbeat_timer_stop(MemfaultMetricId key) {
  return mock()
    .actualCall(__func__)
    .withParameterOfType("MemfaultMetricId", "key", &key)
    .returnIntValue();
}

int memfault_metrics_heartbeat_add(MemfaultMetricId key, int32_t amount) {
  return mock()
    .actualCall(__func__)
    .withParameterOfType("MemfaultMetricId", "key", &key)
    .withParameter("amount", amount)
    .returnIntValue();
}
