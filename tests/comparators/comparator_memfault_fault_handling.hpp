//! @file
//!
//! Comparators for fault_handling.h types.

#pragma once

#include "CppUTestExt/MockSupport.h"

extern "C" {
#include "memfault/panics/fault_handling.h"
}

class Mflt_sMemfaultAssertInfo_Comparator : public MockNamedValueComparator {
 public:
  virtual bool isEqual(const void *object1, const void *object2) {
    const sMemfaultAssertInfo *a = (const sMemfaultAssertInfo *)object1;
    const sMemfaultAssertInfo *b = (const sMemfaultAssertInfo *)object2;

    return (a->extra == b->extra) && (a->assert_reason == b->assert_reason);
  }
  virtual SimpleString valueToString(const void *object) {
    const sMemfaultAssertInfo *o = (const sMemfaultAssertInfo *)object;
    return StringFromFormat("sMemfaultAssertInfo:") +
           StringFromFormat(" extra: 0x%08" PRIx32, o->extra) +
           StringFromFormat(" assert_reason: %d", o->assert_reason);
  }
};
