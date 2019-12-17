//! @file
//! @brief
//! This code creates an RTOS thread that blinks an LED.  It is makes the
//! demo more interesting since you can view multiple threads in a coredump
//! on Memfault.  It also helps to show when Memfault's HardFault handler
//! is running (the LED will freeze).  Most targets should have LED1 defined,
//! but if yours does not, you can remove this code and still run the demo.
#include "blinky.h"

#include "mbed.h"
#include "rtos.h"

static void prv_blinky_thread(void) {
  DigitalOut led1(LED1);

  while (1) {
    led1 = !led1;
    wait(0.5);
  }
}

static uint64_t s_blinky_thread_stack[OS_STACK_SIZE / 8];

void blinky_init(void) {
  static Thread t(osPriorityNormal, OS_STACK_SIZE, (uint8_t *)&s_blinky_thread_stack[0], "blinky_thread");
  t.start(callback(prv_blinky_thread));
}
