#include "CppUTestExt/MockSupport.h"
#include "memfault/metrics/reliability.h"

void memfault_metrics_reliability_collect(void) {
  mock().actualCall("memfault_metrics_reliability_collect");
}
