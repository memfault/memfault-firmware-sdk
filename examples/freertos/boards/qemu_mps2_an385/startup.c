//! @file
//!
//! Copyright (c) Memfault, Inc.
//! See License.txt for details

#include <stdint.h>
#include <sys/types.h>

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
      buf[i] = UART0_ADDR->DATA;
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
extern uint32_t __stack;

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
  prv_cinit();

  uart_init();

  // Call the application's entry point.
  (void)main();

  // shouldn't return
  while (1) {
  };
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
  (void *)(&__stack),  // initial stack pointer
  Reset_Handler,
  NMI_Handler,
  HardFault_Handler,
  MemoryManagement_Handler,
  BusFault_Handler,
  UsageFault_Handler,
  0,
  0,
  0,
  0,
  vPortSVCHandler,     /* SVC_Handler              -5 */
  0,                   /* DebugMon_Handler         -4 */
  0,                   /* reserved */
  xPortPendSVHandler,  /* PendSV handler    -2 */
  xPortSysTickHandler, /* SysTick_Handler   -1 */
};
