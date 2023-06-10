#pragma once

#include "CppUTestExt/MockSupport.h"

extern "C" {
#include "memfault/metrics/metrics.h"
}

class MemfaultMetricIdsComparator : public MockNamedValueComparator {
 public:
  virtual bool isEqual(const void *object1, const void *object2) {
    const MemfaultMetricId *a = (const MemfaultMetricId *)object1;
    const MemfaultMetricId *b = (const MemfaultMetricId *)object2;

    return a->_impl == b->_impl;
  }
  virtual SimpleString valueToString(const void *object) {
    const MemfaultMetricId *o = (const MemfaultMetricId *)object;
    return StringFromFormat("MemfaultMetricId: _impl: %d", o->_impl);
  }
};
