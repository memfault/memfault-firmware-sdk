//! @file
//!
//! Copyright (c) Memfault, Inc.
//! See LICENSE for details

#include <stdint.h>
#include <sys/types.h>

#include "memfault/components.h"

typedef struct UART_t {
  volatile uint32_t DATA;
  volatile uint32_t STATE;
  volatile uint32_t CTRL;
  volatile uint32_t INTSTATUS;
  volatile uint32_t BAUDDIV;
} UART_t;

#define UART0_ADDR ((UART_t *)(0x40004000))
#define UART_DR(baseaddr) (*(unsigned int *)(baseaddr))

#define UART_STATE_TXFULL (1 << 0)
#define UART_CTRL_TX_EN (1 << 0)
#define UART_CTRL_RX_EN (1 << 1)
// snagged the below def from
// https://github.com/ARMmbed/mbed-os/blob/master/targets/TARGET_ARM_FM/TARGET_FVP_MPS2/serial_api.c#L356
#define UART_STATE_RXRDY (2 << 0)

/**
 * @brief initializes the UART emulated hardware
 */
void uart_init(void) {
  UART0_ADDR->BAUDDIV = 16;
  UART0_ADDR->CTRL = UART_CTRL_TX_EN | UART_CTRL_RX_EN;
}

/**
 * @brief not used anywhere in the code
 * @todo  implement if necessary
 *
 */
int _read(__attribute__((unused)) int file, __attribute__((unused)) char *buf,
          __attribute__((unused)) int len) {
  for (int i = 0; i < len; i++) {
    if (UART0_ADDR->STATE & UART_STATE_RXRDY) {
      char data = UART0_ADDR->DATA;
      // for some reason, BACKSPACE=0x7f, DEL=0x7e, so remap those
      data = (data == 0x7f) ? '\b' : data;
      data = (data == 0x7e) ? 0x7f : data;
      buf[i] = data;
    } else {
      return i;
    }
  }
  return len;
}

/**
 * @brief  Write bytes to the UART channel to be displayed on the command line
 *         with qemu
 * @param [in] file  ignored
 * @param [in] buf   buffer to send
 * @param [in] len   length of the buffer
 * @returns the number of bytes written
 */
int _write(__attribute__((unused)) int file, __attribute__((unused)) char *buf, int len) {
  int todo;

  for (todo = 0; todo < len; todo++) {
    UART_DR(UART0_ADDR) = *buf++;
  }

  return len;
}

extern int main(void);

// Following symbols are defined by the linker.
// Start address for the initialization values of the .data section.
extern uint32_t _sidata;
// Start address for the .data section
extern uint32_t _sdata;
// End address for the .data section
extern uint32_t _edata;
// Start address for the .bss section
extern uint32_t __bss_start__;
// End address for the .bss section
extern uint32_t __bss_end__;
// End address for stack
extern void __stack(void);

// Prevent inlining to avoid persisting any stack allocations
__attribute__((noinline)) static void prv_cinit(void) {
  // Initialize data and bss
  // Copy the data segment initializers from flash to SRAM
  for (uint32_t *dst = &_sdata, *src = &_sidata; dst < &_edata;) {
    *(dst++) = *(src++);
  }

  // Zero fill the bss segment.
  for (uint32_t *dst = &__bss_start__; (uintptr_t)dst < (uintptr_t)&__bss_end__;) {
    *(dst++) = 0;
  }
}

__attribute__((noreturn)) void Reset_Handler(void) {
// enable mem/bus/usage faults
#define SCBSHCSR_ (*(uint32_t *)0xE000ED24)
  SCBSHCSR_ |= (1UL << 16U) | (1UL << 17U) | (1UL << 18U);

  prv_cinit();

  uart_init();

  // Call the application's entry point.
  (void)main();

  // shouldn't return
  while (1) { };
}

// These are defined in the FreeRTOS port
extern void vPortSVCHandler(void);
extern void xPortPendSVHandler(void);
extern void xPortSysTickHandler(void);
extern void NMI_Handler(void);
extern void HardFault_Handler(void);
extern void MemoryManagement_Handler(void);
extern void BusFault_Handler(void);
extern void UsageFault_Handler(void);

// A minimal vector table for a Cortex M.
__attribute__((section(".isr_vector"))) void (*const g_pfnVectors[])(void) = {
  __stack,                        // initial stack pointer
  Reset_Handler,                  // 1
  NMI_Handler,                    // 2
  HardFault_Handler,              // 3
  MemoryManagement_Handler,       // 4
  BusFault_Handler,               // 5
  UsageFault_Handler,             // 6
  0,                              // 7, reserved
  0,                              // 8, reserved
  0,                              // 9, reserved
  0,                              // 10, reserved
  vPortSVCHandler,                // 11 SVC_Handler              -5
  0,                              // 12 DebugMon_Handler         -4
  0,                              // 13 reserved */
  xPortPendSVHandler,             // 14 PendSV handler    -2
  xPortSysTickHandler,            // 15 SysTick_Handler   -1
  0,                              // 16 UART 0
  0,                              // 17 UART 0
  0,                              // 18 UART 1
  0,                              // 19 UART 1
  0,                              // 20 UART 2
  0,                              // 21 UART 2
  0,                              // 22 GPIO 0
  0,                              // 23 GPIO 1
  MEMFAULT_EXC_HANDLER_WATCHDOG,  // 24 Timer 0
};
