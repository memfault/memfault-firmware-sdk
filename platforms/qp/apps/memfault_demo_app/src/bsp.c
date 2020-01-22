//! @file
//!
//! Copyright (c) Memfault, Inc.
//! See License.txt for details

#include "bsp.h"

#include "qf_port.h"
#include "stm32f4xx.h"
#include "stm32f4xx_gpio.h"
#include "stm32f4xx_rcc.h"
#include "stm32f4xx_usart.h"

#include "memfault/demo/shell.h"

void bsp_init(void) {
  SystemCoreClockUpdate();

  // Use the automatic FPU state preservation and the FPU lazy stacking.
  FPU->FPCCR |= (1U << FPU_FPCCR_ASPEN_Pos) | (1U << FPU_FPCCR_LSPEN_Pos);

  // Enable USART2:
  RCC_APB1PeriphClockCmd(RCC_APB1Periph_USART2, ENABLE);
  RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOA, ENABLE);

  GPIO_Init(GPIOA, &(GPIO_InitTypeDef){
    .GPIO_Pin = GPIO_Pin_2 | GPIO_Pin_3,
    .GPIO_Mode = GPIO_Mode_AF,
    .GPIO_Speed = GPIO_Speed_50MHz,
    .GPIO_OType = GPIO_OType_PP,
    .GPIO_PuPd = GPIO_PuPd_UP,
  });

  GPIO_PinAFConfig(GPIOA, GPIO_PinSource2, GPIO_AF_USART2); /* TX = PA2 */
  GPIO_PinAFConfig(GPIOA, GPIO_PinSource3, GPIO_AF_USART2); /* RX = PA3 */

  USART_Init(USART2, &(USART_InitTypeDef) {
    .USART_BaudRate = 115200,
    .USART_WordLength = USART_WordLength_8b,
    .USART_StopBits = USART_StopBits_1,
    .USART_Parity = USART_Parity_No,
    .USART_HardwareFlowControl = USART_HardwareFlowControl_None,
    .USART_Mode = USART_Mode_Tx | USART_Mode_Rx,
  });
  USART_Cmd(USART2, ENABLE);

  SysTick_Config(SystemCoreClock / BSP_TICKS_PER_SEC);
  NVIC_SetPriorityGrouping(0U);
  NVIC_SetPriority(SysTick_IRQn, QF_AWARE_ISR_CMSIS_PRI);
}

int bsp_send_char_over_uart(char c) {
  while ((USART2->SR & USART_FLAG_TXE) == 0);
  USART2->DR = c;
  while ((USART2->SR & USART_FLAG_TXE) == 0);
  return 1;
}

bool bsp_read_char_from_uart(char *out_char) {
  if ((USART2->SR & USART_SR_RXNE) == 0) {
    return false;
  }
  *out_char = USART2->DR;
  return true;
}

void SysTick_Handler(void) {
  // Process time events for tick rate 0
  QF_TICK_X(0U, 0);
}

