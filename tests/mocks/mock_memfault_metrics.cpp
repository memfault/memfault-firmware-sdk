#include "CppUTestExt/MockSupport.h"
#include "memfault/metrics/metrics.h"

int memfault_metrics_heartbeat_set_unsigned(MemfaultMetricId key, uint32_t unsigned_value) {
  return mock()
    .actualCall("memfault_metrics_heartbeat_set_unsigned")
    .withParameterOfType("MemfaultMetricId", "key", &key)
    .withParameter("unsigned_value", unsigned_value)
    .returnIntValue();
}

int memfault_metrics_heartbeat_set_signed(MemfaultMetricId key, int32_t signed_value) {
  return mock()
    .actualCall("memfault_metrics_heartbeat_set_signed")
    .withParameterOfType("MemfaultMetricId", "key", &key)
    .withParameter("signed_value", signed_value)
    .returnIntValue();
}
