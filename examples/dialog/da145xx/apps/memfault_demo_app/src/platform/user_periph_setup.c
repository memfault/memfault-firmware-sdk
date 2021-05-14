/**
 ****************************************************************************************
 *
 * @file user_periph_setup.c
 *
 * @brief Peripherals setup and initialization.
 *
 * Copyright (C) 2015-2019 Dialog Semiconductor.
 * This computer program includes Confidential, Proprietary Information
 * of Dialog Semiconductor. All Rights Reserved.
 *
 ****************************************************************************************
 */

/*
 * INCLUDE FILES
 ****************************************************************************************
 */

#include "user_periph_setup.h"
#include "datasheet.h"
#include "system_library.h"
#include "rwip_config.h"
#include "gpio.h"
#include "uart.h"
#include "syscntl.h"
#include "spi.h"
#include "spi_flash.h"

/*
 * GLOBAL VARIABLE DEFINITIONS
 ****************************************************************************************
 */

#if DEVELOPMENT_DEBUG

void GPIO_reservations(void)
{
/*
    i.e. to reserve P0_1 as Generic Purpose I/O:
    RESERVE_GPIO(DESCRIPTIVE_NAME, GPIO_PORT_0, GPIO_PIN_1, PID_GPIO);
*/
    RESERVE_GPIO(SPI_FLASH_CS, SPI_EN_PORT, SPI_EN_PIN, PID_SPI_EN);
    RESERVE_GPIO(SPI_FLASH_CLK, SPI_CLK_PORT, SPI_CLK_PIN, PID_SPI_CLK);
    RESERVE_GPIO(SPI_FLASH_DO, SPI_DO_PORT, SPI_DO_PIN, PID_SPI_DO);
    RESERVE_GPIO(SPI_FLASH_DI, SPI_DI_PORT, SPI_DI_PIN, PID_SPI_DI);

#if defined (CFG_PRINTF_UART2)
    RESERVE_GPIO(UART2_TX, UART2_TX_PORT, UART2_TX_PIN, PID_UART2_TX);
    RESERVE_GPIO(UART2_RX, UART2_RX_PORT, UART2_RX_PIN, PID_UART2_RX);
#endif
}

#endif

void set_pad_functions(void)
{
/*
    i.e. to set P0_1 as Generic purpose Output:
    GPIO_ConfigurePin(GPIO_PORT_0, GPIO_PIN_1, OUTPUT, PID_GPIO, false);
*/
#if defined (__DA14531__)
    /* P0_0 is used as both the SPI data out and the reset input. */
    GPIO_Disable_HW_Reset();
#endif

    GPIO_ConfigurePin(SPI_EN_PORT, SPI_EN_PIN, OUTPUT, PID_SPI_EN, true);
    GPIO_ConfigurePin(SPI_CLK_PORT, SPI_CLK_PIN, OUTPUT, PID_SPI_CLK, false);
    GPIO_ConfigurePin(SPI_DO_PORT, SPI_DO_PIN, OUTPUT, PID_SPI_DO, false);
    GPIO_ConfigurePin(SPI_DI_PORT, SPI_DI_PIN, INPUT, PID_SPI_DI, false);

#if defined (__DA14586__)
    // Disallow spontaneous DA14586 SPI Flash wake-up
    GPIO_ConfigurePin(GPIO_PORT_2, GPIO_PIN_3, OUTPUT, PID_GPIO, true);
#endif

#if defined (CFG_PRINTF_UART2)
    // Configure UART2 TX Pad
    GPIO_ConfigurePin(UART2_TX_PORT, UART2_TX_PIN, OUTPUT, PID_UART2_TX, false);
    GPIO_ConfigurePin(UART2_RX_PORT, UART2_RX_PIN, OUTPUT, PID_UART2_RX, false);
#endif

}

#if defined (CFG_PRINTF_UART2)
// Configuration struct for UART2
static const uart_cfg_t uart_cfg = {
    .baud_rate = UART2_BAUDRATE,
    .data_bits = UART2_DATABITS,
    .parity = UART2_PARITY,
    .stop_bits = UART2_STOPBITS,
    .auto_flow_control = UART2_AFCE,
    .use_fifo = UART2_FIFO,
    .tx_fifo_tr_lvl = UART2_TX_FIFO_LEVEL,
    .rx_fifo_tr_lvl = UART2_RX_FIFO_LEVEL,
    .intr_priority = 2,
};
#endif

/* Default SPI configuration */
static const spi_cfg_t spi_cfg = {
    .spi_ms = SPI_MS_MODE_MASTER,
    .spi_cp = SPI_CP_MODE_0,
    .spi_speed = SPI_SPEED_MODE_4MHz,
    .spi_wsz = SPI_MODE_8BIT,
    .spi_cs = SPI_CS_0,
    .cs_pad.port = SPI_EN_PORT,
    .cs_pad.pin = SPI_EN_PIN,
#if defined (__DA14531__)
    .spi_capture = SPI_MASTER_EDGE_CAPTURE,
#endif
};

/* SPI flash configuration - assumes use of a Macronix MXR2035F as this is
   present on the DA145xx PRO development kit */
static const spi_flash_cfg_t spi_flash_cfg = {
    .dev_index = MX25R2035F_DEV_INDEX,
    .jedec_id  = MX25V2035F_JEDEC_ID,
    .chip_size = MX25V2035F_CHIP_SIZE,
};

void periph_init(void)
{
#if defined (__DA14531__)
    // In Boost mode enable the DCDC converter to supply VBAT_HIGH for the used GPIOs
    syscntl_dcdc_turn_on_in_boost(SYSCNTL_DCDC_LEVEL_3V0);
#else
    // Power up peripherals' power domain
    SetBits16(PMU_CTRL_REG, PERIPH_SLEEP, 0);
    while (!(GetWord16(SYS_STAT_REG) & PER_IS_UP));
    SetBits16(CLK_16M_REG, XTAL16_BIAS_SH_ENABLE, 1);
#endif

    // ROM patch
    patch_func();

    // Initialize peripherals
#if defined (CFG_PRINTF_UART2)
    // Initialize UART2
    uart_initialize(UART2, &uart_cfg);
#endif

    // Initialize interface to SPI flash so we can use it from within the application
    spi_flash_configure_env(&spi_flash_cfg);
    spi_initialize(&spi_cfg);

    // Set pad functionality
    set_pad_functions();

    // Enable the pads
    GPIO_set_pad_latch_en(true);
}
