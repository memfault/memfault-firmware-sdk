/**
 ****************************************************************************************
 *
 * @file da1458x_config_basic.h
 *
 * @brief Basic compile configuration file.
 *
 * Copyright (C) 2014-2019 Dialog Semiconductor.
 * This computer program includes Confidential, Proprietary Information
 * of Dialog Semiconductor. All Rights Reserved.
 *
 ****************************************************************************************
 */

#ifndef _DA1458X_CONFIG_BASIC_H_
#define _DA1458X_CONFIG_BASIC_H_

#include "da1458x_stack_config.h"
#include "user_profiles_config.h"

#if !defined (__DA14531__)

/***************************************************************************************************************/
/* Integrated or external processor configuration                                                              */
/*    -defined      Integrated processor mode. Host application runs in DA14585 processor. Host application    */
/*                  is the TASK_APP kernel task.                                                               */
/*    -undefined    External processor mode. Host application runs on an external processor. Communicates with */
/*                  BLE application through GTL protocol over a signalling iface (UART, SPI etc)               */
/***************************************************************************************************************/
#define CFG_APP

/****************************************************************************************************************/
/* Enables the BLE security functionality in TASK_APP. If not defined BLE security related code is compiled out.*/
/****************************************************************************************************************/
#undef CFG_APP_SECURITY

/****************************************************************************************************************/
/* Enables WatchDog timer.                                                                                      */
/****************************************************************************************************************/
#define CFG_WDOG

/****************************************************************************************************************/
/* Watchdog timer behavior in production mode:                                                                  */
/*     Flag is not defined: Watchdog timer generates NMI at value 0.                                            */
/*     Flag is defined    : Watchdog timer generates a WDOG (SYS) reset at value 0.                             */
/****************************************************************************************************************/
#undef CFG_WDG_TRIGGER_HW_RESET_IN_PRODUCTION_MODE

/****************************************************************************************************************/
/* Determines maximum concurrent connections supported by application. It configures the heap memory allocated  */
/* to service multiple connections. It is used for GAP central role applications. For GAP peripheral role it    */
/* should be set to 1 for optimizing memory utilization.                                                        */
/*      - MAX value for DA14585: 8                                                                              */
/****************************************************************************************************************/
#define CFG_MAX_CONNECTIONS     (1)

/****************************************************************************************************************/
/* Enables development/debug mode. For production mode builds it must be disabled.                              */
/* When enabled the following debugging features are enabled                                                    */
/*      -   Allows the emulation of the OTP mirroring to System RAM. No actual writing to RAM is done, but the  */
/*          exact same amount of time is spend as if the mirroring would take place. This is to mimic the       */
/*          behavior as if the System Code is already in OTP, and the mirroring takes place after waking up,    */
/*          but the (development) code still resides in an external source.                                     */
/*      -   Validation of GPIO reservations.                                                                    */
/*      -   Enables Debug module and sets code execution in breakpoint in Hardfault and NMI (Watchdog) handlers.*/
/*          It allows developer to hot attach debugger and get debug information                                */
/****************************************************************************************************************/
#define CFG_DEVELOPMENT_DEBUG

/****************************************************************************************************************/
/* UART Console Print. If CFG_PRINTF is defined, serial interface logging mechanism will be enabled.            */
/* If CFG_PRINTF_UART2 is defined, then serial interface logging mechanism is implented using UART2, else UART1 */
/* will be used.                                                                                                */
/****************************************************************************************************************/
#define CFG_PRINTF
#ifdef CFG_PRINTF
    #define CFG_PRINTF_UART2
#endif

/****************************************************************************************************************/
/* UART1 Driver Implementation. If CFG_UART1_SDK is defined, UART1 ROM driver will be overriden and UART SDK    */
/* driver will be used, else ROM driver will be used for UART1 module.                                          */
/****************************************************************************************************************/
#undef CFG_UART1_SDK


/****************************************************************************************************************/
/* Select external memory device for data storage                                                               */
/* SPI FLASH  (#define CFG_SPI_FLASH_ENABLE)                                                                    */
/* I2C EEPROM (#define CFG_I2C_EEPROM_ENABLE)                                                                   */
/****************************************************************************************************************/
#undef CFG_SPI_FLASH_ENABLE
#undef CFG_I2C_EEPROM_ENABLE

/****************************************************************************************************************/
/* Enables/Disables the DMA Support for the following interfaces:                                               */
/*     - UART                                                                                                   */
/*     - SPI                                                                                                    */
/*     - I2C                                                                                                    */
/****************************************************************************************************************/
#undef CFG_UART_DMA_SUPPORT
#undef CFG_SPI_DMA_SUPPORT
#undef CFG_I2C_DMA_SUPPORT



#else



/***************************************************************************************************************/
/* Integrated or external processor configuration                                                              */
/*    -defined      Integrated processor mode. Host application runs in DA14585 processor. Host application    */
/*                  is the TASK_APP kernel task.                                                               */
/*    -undefined    External processor mode. Host application runs on an external processor. Communicates with */
/*                  BLE application through GTL protocol over a signalling iface (UART, SPI etc)               */
/***************************************************************************************************************/
#define CFG_APP

/****************************************************************************************************************/
/* Enables the BLE security functionality in TASK_APP. If not defined BLE security related code is compiled out.*/
/****************************************************************************************************************/
#undef CFG_APP_SECURITY

/****************************************************************************************************************/
/* Enables WatchDog timer.                                                                                      */
/****************************************************************************************************************/
#define CFG_WDOG

/****************************************************************************************************************/
/* Watchdog timer behavior in production mode:                                                                  */
/*     Flag is not defined: Watchdog timer generates NMI at value 0.                                            */
/*     Flag is defined    : Watchdog timer generates a WDOG (SYS) reset at value 0.                             */
/****************************************************************************************************************/
#undef CFG_WDG_TRIGGER_HW_RESET_IN_PRODUCTION_MODE

/****************************************************************************************************************/
/* Determines maximum concurrent connections supported by application. It configures the heap memory allocated  */
/* to service multiple connections. It is used for GAP central role applications. For GAP peripheral role it    */
/* should be set to 1 for optimizing memory utilization.                                                        */
/*      - MAX value for DA14531: 3                                                                              */
/****************************************************************************************************************/
#define CFG_MAX_CONNECTIONS     (1)

/****************************************************************************************************************/
/* Enables development/debug mode. For production mode builds it must be disabled.                              */
/* When enabled the following debugging features are enabled                                                    */
/*      -   Allows the emulation of the OTP mirroring to System RAM. No actual writing to RAM is done, but the  */
/*          exact same amount of time is spend as if the mirroring would take place. This is to mimic the       */
/*          behavior as if the System Code is already in OTP, and the mirroring takes place after waking up,    */
/*          but the (development) code still resides in an external source.                                     */
/*      -   Validation of GPIO reservations.                                                                    */
/*      -   Enables Debug module and sets code execution in breakpoint in Hardfault and NMI (Watchdog) handlers.*/
/*          It allows developer to hot attach debugger and get debug information                                */
/****************************************************************************************************************/
#define CFG_DEVELOPMENT_DEBUG

/****************************************************************************************************************/
/* UART Console Print. If CFG_PRINTF is defined, serial interface logging mechanism will be enabled.            */
/* If CFG_PRINTF_UART2 is defined, then serial interface logging mechanism is implented using UART2, else UART1 */
/* will be used.                                                                                                */
/****************************************************************************************************************/
#define CFG_PRINTF
#ifdef CFG_PRINTF
    #define CFG_PRINTF_UART2
#endif

/****************************************************************************************************************/
/* UART1 Driver Implementation. If CFG_UART1_SDK is defined, UART1 ROM driver will be overriden and UART SDK    */
/* driver will be used, else ROM driver will be used for UART1 module.                                          */
/****************************************************************************************************************/
#undef CFG_UART1_SDK


/****************************************************************************************************************/
/* Select external memory device for data storage                                                               */
/* SPI FLASH  (#define CFG_SPI_FLASH_ENABLE)                                                                    */
/* I2C EEPROM (#define CFG_I2C_EEPROM_ENABLE)                                                                   */
/****************************************************************************************************************/
#undef CFG_SPI_FLASH_ENABLE
#undef CFG_I2C_EEPROM_ENABLE

/****************************************************************************************************************/
/* Enables/Disables the DMA Support for the following interfaces:                                               */
/*     - UART                                                                                                   */
/*     - SPI                                                                                                    */
/*     - I2C                                                                                                    */
/*     - ADC                                                                                                    */
/****************************************************************************************************************/
#undef CFG_UART_DMA_SUPPORT
#undef CFG_SPI_DMA_SUPPORT
#undef CFG_I2C_DMA_SUPPORT
#undef CFG_ADC_DMA_SUPPORT

/****************************************************************************************************************/
/* Notify the SDK about the fixed power mode (currently used only for Bypass):                                  */
/*     - CFG_POWER_MODE_BYPASS = Bypass mode                                                                    */
/****************************************************************************************************************/
#undef CFG_POWER_MODE_BYPASS

#endif

#endif // _DA1458X_CONFIG_BASIC_H_
