// Clang-format really hates this file, so disable it
// clang-format off
#include "CppUTest/MemoryLeakDetectorMallocMacros.h"
#include "CppUTest/MemoryLeakDetectorNewMacros.h"
#include "CppUTest/TestHarness.h"
#include "CppUTestExt/MockSupport.h"

extern "C" {
#define printf error_yolo
#if defined(printf)
#include "memfault/core/compiler.h"
#endif
}

MEMFAULT_PRINTF_LIKE_FUNC(1,2) void memfault_printf(const char *fmt, ...);

void memfault_printf(const char *fmt, ...) {
  // grab va_list and forward to vprintf
  va_list args;
  va_start(args, fmt);
  vprintf(fmt, args);
  va_end(args);
}


TEST_GROUP(MfltPrintfAttribute){
  void setup() {
    mock().strictOrder();
  }
  void teardown() {
    mock().checkExpectations();
    mock().clear();
  }
};

TEST(MfltPrintfAttribute, Test_PrintfImplementation) {
  // just call it. the real check is the compilation check though.
  memfault_printf("hello world\n");
}
