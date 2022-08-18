//! @file
//!
//! Copyright (c) Memfault, Inc.
//! See License.txt for details
//!
//! @brief
//! A Software Watchdog port for nRF5 platform which makes use of the RTC Peripheral.
//!
//! Note: RTC2 is used since RTC0 is used by the SoftDevice & RTC1 is used by the app_timer module.
//! If you are already making use of RTC2 for other purposes you may need to fork this port and
//! modify accordingly.
//!
//! Note: While the nRF5 WDT peripheral does have an interrupt that can be enabled when the
//! watchdog expires, the delay until a reboot is only two 32kHz clock cycles and is _not_
//! configurable. This does not provide enough time to save a full coredump so we make use of the
//! RTC peripheral instead as our software watchdog.
//!
//! Note: If you are making use of the nRF5 WDT peripheral, this module will auto configure a
//! software watchdog timeout that is 125ms shorter than the hardware watchdog timeout. When the
//! software watchdog fires, one final feed of the hardware watchdog will be made and a coredump
//! will be captured. A different timeout can be configured by adding the following to your
//! memfault_platform_config.h:
//!  #define MEMFAULT_NRF5_WATCHDOG_SW_AUTOCONFIG 0
//!  #define MEMFAULT_WATCHDOG_SW_TIMEOUT_SECS <desired timeout in seconds>
//!
//! Recommended setup:
//!
//!  1. Add the following to your project's sdk_config.h to enable the RTC2 Peripheral
//!     #define NRFX_RTC_ENABLED 1
//!     #define NRFX_RTC2_ENABLED 1
//!     #define RTC_ENABLED 1 // only needed if these legacy defines are in your sdk_config.h
//!     #define RTC2_ENABLED 1 // only needed if these legacy defines are in your sdk_config.h
//!  2. Add following C files to build system:
//!      ${NRF5_SDK_ROOT}/modules/nrfx/drivers/src/nrfx_rtc.c
//!      ${MEMFAULT_SDK_ROOT}/ports/nrf5_sdk/software_watchdog.c
//!  3. In the same place you enable the hardware watchdog (via wdt_enable)
//!     initialize this module:
//!     #include "memfault/ports/watchdog.h"
//!     ...
//!     memfault_software_watchdog_enable();
//!  4. In the same place you feed the hardware watchdog (via wdt_channel_feed, wdt_feed),
//!     feed this software watchdog:
//!     #include "memfault/ports/watchdog.h"
//!     ...
//!     memfault_software_watchdog_feed();
//!

#include "memfault/ports/watchdog.h"

#include "memfault/components.h"

#include "nrfx_rtc.h"
#include "app_error.h"

#if NRFX_WDT_ENABLED
#include <nrfx_wdt.h>
#endif

//
// Configuration sanity check -- if legacy defines exist in sdk_config.h they will get remapped to
// the expected defines via integration/nrfx/legacy/apply_old_config.h. We use the same logic from
// that header to sanity check the configuration.
//

#if defined(RTC_ENABLED)
#if !RTC_ENABLED
#error "Set RTC_ENABLED 1 to use the Software Watchdog port"
#endif
#elif !NRFX_RTC_ENABLED
#error "Set NRFX_RTC_ENABLED 1 to use the Software Watchdog port"
#endif

#if defined(RTC2_ENABLED)
#if !RTC2_ENABLED
#error "Set RTC2_ENABLED 1 to use the Software Watchdog port"
#endif
#elif !NRFX_RTC2_ENABLED
#error "Set NRFX_RTC2_ENABLED 1 to use the Software Watchdog port"
#endif

//! Note: We set PRESCALE to 2^12-1 which results in a 125ms resolution per RTC tick
#define MEMFAULT_PRESCALE_RESOLUTION_MS 125


#ifndef MEMFAULT_NRF5_WATCHDOG_SW_AUTOCONFIG
//! Auto configure software watchdog timeout relative to hardware watchdog
#define MEMFAULT_NRF5_WATCHDOG_SW_AUTOCONFIG NRFX_WDT_ENABLED
#endif

#if MEMFAULT_NRF5_WATCHDOG_SW_AUTOCONFIG

#if NRFX_WDT_CONFIG_RELOAD_VALUE < MEMFAULT_PRESCALE_RESOLUTION_MS
#error "NRFX_WDT_CONFIG_RELOAD_VALUE must be > MEMFAULT_PRESCALE_RESOLUTION_MS (125ms)"
#endif

#define MEMFAULT_NRF5_WATCHDOG_SW_TIMEOUT_MS \
  (NRFX_WDT_CONFIG_RELOAD_VALUE - MEMFAULT_PRESCALE_RESOLUTION_MS)

#else

#if (NRFX_WDT_ENABLED && (NRFX_WDT_CONFIG_RELOAD_VALUE < (MEMFAULT_WATCHDOG_SW_TIMEOUT_SECS * 1000)))
#error "Set MEMFAULT_WATCHDOG_SW_TIMEOUT_SECS must be less than hardware watchdog timeout (NRFX_WDT_CONFIG_RELOAD_VALUE)"
#endif

#define MEMFAULT_NRF5_WATCHDOG_SW_TIMEOUT_MS  (MEMFAULT_WATCHDOG_SW_TIMEOUT_SECS * 1000)

#endif /* MEMFAULT_NRF5_WATCHDOG_SW_AUTOCONFIG */

#ifndef MEMFAULT_NRF5_WATCHDOG_SW_CHANNEL
#define MEMFAULT_NRF5_WATCHDOG_SW_CHANNEL 0
#endif

//! The nRF5 MCU has 3 RTC peripheral instances. RTC0 is used for scheduling by the SoftDevice,
//! RTC1 is used for the app_timer library. And
static const nrfx_rtc_t s_rtc = NRFX_RTC_INSTANCE(2);

static void prv_software_watchdog_timeout(nrfx_rtc_int_type_t int_type) {
#if NRFX_WDT_ENABLED
  nrfx_wdt_feed();
#endif

  //! Note: We can't call MEMFAULT_EXC_HANDLER_WATCHDOG directly here because the IRQ is handled by
  //! the nRF5 SDK so we will trap into the assert handler instead.
  //!
  //! Stack at this point for reference:
  //!   irq_handler (p_reg=0x40024000, instance_id=0, channel_count=4)
  //!   RTC2_IRQHandler ()
  MEMFAULT_SOFTWARE_WATCHDOG();
}

int memfault_software_watchdog_enable(void) {
  const nrfx_rtc_config_t config = {
    // Use highest configurable priority interrupt so we can catch hangs from low priority ISRs
    .interrupt_priority = 0,
    // This will make our timer resolution equal to (32768 / 2^12) = 125ms
    .prescaler = 0xfff,
    // NB: tick_latency is only used if reliable == true
    .tick_latency = 0xff,
    .reliable = false,
  };

  ret_code_t err_code = nrfx_rtc_init(&s_rtc, &config, &prv_software_watchdog_timeout);
  if (err_code != NRF_SUCCESS) {
    MEMFAULT_LOG_ERROR("Failed to configure rtc software watchdog: rv=%d", (int)err_code);
    return err_code;
  }

  nrfx_rtc_tick_disable(&s_rtc);
  nrfx_rtc_counter_clear(&s_rtc);
  nrfx_rtc_enable(&s_rtc);

  memfault_software_watchdog_update_timeout(MEMFAULT_NRF5_WATCHDOG_SW_TIMEOUT_MS);
  return 0;
}

int memfault_software_watchdog_disable(void) {
  nrfx_rtc_disable(&s_rtc);
  return 0;
}

int memfault_software_watchdog_feed(void) {
  nrfx_rtc_counter_clear(&s_rtc);
  return 0;
}

int memfault_software_watchdog_update_timeout(uint32_t timeout_ms) {
  const uint32_t counter_val = (timeout_ms / MEMFAULT_PRESCALE_RESOLUTION_MS);
  MEMFAULT_LOG_INFO("Software Watchdog configured to %dms",
                    (int)(counter_val * MEMFAULT_PRESCALE_RESOLUTION_MS));

  // We configure a counter interrupt to fire when the desired timeout is hit.
  // When we "feed" the watchdog, we reset the counter back to 0. Therefore, the interrupt
  // will only trigger if we fail to feed the watchdog in the specified window.
  nrfx_rtc_cc_set(&s_rtc, MEMFAULT_NRF5_WATCHDOG_SW_CHANNEL, counter_val, true /* enable_irq */);

  return 0;
}
