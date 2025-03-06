//! @file

#include <stdbool.h>

#include "CppUTestExt/MockSupport.h"
#include "memfault/core/platform/system_time.h"

bool memfault_platform_time_get_current(sMemfaultCurrentTime *time) {
  return mock().actualCall(__func__).withOutputParameter("time", time).returnBoolValue();
}
