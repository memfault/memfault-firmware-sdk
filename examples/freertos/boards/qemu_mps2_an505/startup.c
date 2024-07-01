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

// See application note AN505, and the CMSDK UART doc:
// https://developer.arm.com/documentation/ddi0479/b/APB-Components/UART/Programmers-model
#define UART0_ADDR ((UART_t *)(0x40200000))
#define UART_DR(baseaddr) (*(unsigned int *)(baseaddr))

#define UART_STATE_TXFULL (1 << 0)
#define UART_CTRL_TX_EN (1 << 0)
#define UART_CTRL_RX_EN (1 << 1)
#define UART_STATE_RXRDY (2 << 0)

void uart_init(void) {
  UART0_ADDR->CTRL |= UART_CTRL_TX_EN;
  UART0_ADDR->CTRL |= UART_CTRL_RX_EN;
}

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
  // use assembly to set the msr      msplim,__stack
  __asm volatile("msr msplim, %0" ::"r"(__stack));

// enable the FPU
#define SCB_CPACR_ (*(volatile uint32_t *)0xE000ED88)
  SCB_CPACR_ |= ((3UL << 10 * 2) | (3UL << 11 * 2));  // set CP10 and CP11 Full Access

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
extern void SVC_Handler(void);
extern void PendSV_Handler(void);
extern void SysTick_Handler(void);
extern void NMI_Handler(void);
extern void HardFault_Handler(void);
extern void MemManage_Handler(void);
extern void BusFault_Handler(void);
extern void UsageFault_Handler(void);

// A minimal vector table for a Cortex M.
__attribute__((section(".isr_vector"))) void (*const g_pfnVectors[])(void) = {
  (void *)(&__stack),     // initial stack pointer
  Reset_Handler,          /*     Reset Handler */
  NMI_Handler,            /* -14 NMI Handler */
  HardFault_Handler,      /* -13 Hard Fault Handler */
  MemManage_Handler,      /* -12 MPU Fault Handler */
  BusFault_Handler,       /* -11 Bus Fault Handler */
  UsageFault_Handler,     /* -10 Usage Fault Handler */
  0,                      /*  -9 Secure Fault Handler */
  0,                      /*     Reserved */
  0,                      /*     Reserved */
  0,                      /*     Reserved */
  SVC_Handler,            /*  -5 SVCall Handler */
  0 /*DebugMon_Handler*/, /*  -4 Debug Monitor Handler */
  0,                      /*     Reserved */
  PendSV_Handler,         /*  -2 PendSV Handler */
  SysTick_Handler,        /*  -1 SysTick Handler */
};
