#include <stdint.h>

// Jump through some hoops to trick the compiler into doing an unaligned 64 bit access
static uint8_t s_test_buffer[16];
void *g_memfault_unaligned_buffer = &s_test_buffer[1];
