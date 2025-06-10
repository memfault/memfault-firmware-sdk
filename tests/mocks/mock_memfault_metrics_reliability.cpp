#include "CppUTestExt/MockSupport.h"
#include "memfault/metrics/reliability.h"

void memfault_metrics_reliability_boot(sMemfaultMetricsReliabilityCtx *ctx) {
  mock().actualCall("memfault_metrics_reliability_boot").withParameter("ctx", ctx);
}

sMemfaultMetricsReliabilityCtx *memfault_metrics_reliability_get_ctx(void) {
  mock().actualCall("memfault_metrics_reliability_get_ctx");
  // Return a pointer to a static context for testing purposes
  static sMemfaultMetricsReliabilityCtx ctx;
  return &ctx;
}

void memfault_metrics_reliability_collect(void) {
  mock().actualCall("memfault_metrics_reliability_collect");
}
