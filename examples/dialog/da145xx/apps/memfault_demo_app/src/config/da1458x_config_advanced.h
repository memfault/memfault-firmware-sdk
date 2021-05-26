/**
 ****************************************************************************************
 *
 * @file da1458x_config_advanced.h
 *
 * @brief Advanced compile configuration file.
 *
 * Copyright (C) 2014-2020 Dialog Semiconductor.
 * This computer program includes Confidential, Proprietary Information
 * of Dialog Semiconductor. All Rights Reserved.
 *
 ****************************************************************************************
 */

#ifndef _DA1458X_CONFIG_ADVANCED_H_
#define _DA1458X_CONFIG_ADVANCED_H_

#include "da1458x_stack_config.h"

#if !defined (__DA14531__)

/****************************************************************************************************************/
/* Low Power clock selection.                                                                                   */
/*      -LP_CLK_XTAL32      External XTAL32K oscillator                                                         */
/*      -LP_CLK_RCX20       Internal RCX clock                                                                  */
/*      -LP_CLK_FROM_OTP    Use the selection in the corresponding field of OTP Header                          */
/*                                                                                                              */
/* NOTE: Disable CFG_XTAL16M_ADAPTIVE_SETTLING flag when RCX is chosen as the LP clock either from the OTP      */
/*       header or from the SDK.                                                                                */
/****************************************************************************************************************/
#define CFG_LP_CLK              LP_CLK_XTAL32

/****************************************************************************************************************/
/* If defined the application uses a hardcoded value for XTAL16M trimming. Should be disabled for devices       */
/* where XTAL16M is calibrated and trim value is stored in OTP.                                                 */
/* Important note. The hardcoded value is the average value of the trimming values giving the optimal results   */
/* for DA14585 DK devices. May not be applicable in other designs                                               */
/****************************************************************************************************************/
#define CFG_USE_DEFAULT_XTAL16M_TRIM_VALUE_IF_NOT_CALIBRATED

/****************************************************************************************************************/
/* Periodic wakeup period to poll GTL iface. Time in msec.                                                      */
/****************************************************************************************************************/
#define CFG_MAX_SLEEP_DURATION_PERIODIC_WAKEUP_MS                  500  // 0.5s

/****************************************************************************************************************/
/* Periodic wakeup period if GTL iface is not enabled. Time in msec.                                            */
/****************************************************************************************************************/
#define CFG_MAX_SLEEP_DURATION_EXTERNAL_WAKEUP_MS              600000  // 600s

/****************************************************************************************************************/
/* Wakeup from external processor running host application.                                                     */
/****************************************************************************************************************/
#undef CFG_EXTERNAL_WAKEUP

/****************************************************************************************************************/
/* Wakeup external processor when a message is sent to GTL                                                      */
/****************************************************************************************************************/
#undef CFG_WAKEUP_EXT_PROCESSOR

/****************************************************************************************************************/
/* Enables True Random Number Generator. A true random number, generated at system initialization, is used to   */
/* seed any random number generator (C standard library, ChaCha20, etc.). The following supported options       */
/* provide a trade-off between code size and start-up latency.                                                  */
/* - undefined (or 0): TRNG is disabled.                                                                        */
/* -   32:  Enables TRNG with   32 Bytes Buffer.                                                                */
/* -   64:  Enables TRNG with   64 Bytes Buffer.                                                                */
/* -  128:  Enables TRNG with  128 Bytes Buffer.                                                                */
/* -  256:  Enables TRNG with  256 Bytes Buffer.                                                                */
/* -  512:  Enables TRNG with  512 Bytes Buffer.                                                                */
/* - 1024:  Enables TRNG with 1024 Bytes Buffer.                                                                */
/****************************************************************************************************************/
#define CFG_TRNG (1024)

/****************************************************************************************************************/
/* Creation of private and public keys using Elliptic Curve Diffie Hellman algorithms.                          */
/* - defined:   Creation of ECDH keys and secure connection feature is enabled.                                 */
/* - undefined: Creation of ECDH keys and secure connection feature is disabled. If application does not        */
/*              support secure connections, it is reccomended to undefine CFG_ENABLE_SMP_SECURE in order to     */
/*              enable faster start-up time and reduce code size.                                               */
/****************************************************************************************************************/
#define CFG_ENABLE_SMP_SECURE

/****************************************************************************************************************/
/* Uses ChaCha20 random number generator instead of the C standard library random number generator.             */
/****************************************************************************************************************/
#undef CFG_USE_CHACHA20_RAND

/****************************************************************************************************************/
/* Custom heap sizes                                                                                            */
/****************************************************************************************************************/
// #define DB_HEAP_SZ              1024
// #define ENV_HEAP_SZ             4928
// #define MSG_HEAP_SZ             6880
// #define NON_RET_HEAP_SZ         2048

/****************************************************************************************************************/
/* NVDS configuration                                                                                           */
/* - CFG_NVDS_TAG_BD_ADDRESS            Default bdaddress. If bdaddress is written in OTP header this value is  */
/*                                      ignored                                                                 */
/* - CFG_NVDS_TAG_LPCLK_DRIFT           Low power clock drift. Permitted values in ppm are:                     */
/*      + DRIFT_20PPM                                                                                           */
/*      + DRIFT_30PPM                                                                                           */
/*      + DRIFT_50PPM                                                                                           */
/*      + DRIFT_75PPM                                                                                           */
/*      + DRIFT_100PPM                                                                                          */
/*      + DRIFT_150PPM                                                                                          */
/*      + DRIFT_250PPM                                                                                          */
/*      + DRIFT_500PPM                  Default value (500 ppm)                                                 */
/* - CFG_NVDS_TAG_BLE_CA_TIMER_DUR      Channel Assessment Timer duration (Multiple of 10ms)                    */
/* - CFG_NVDS_TAG_BLE_CRA_TIMER_DUR     Channel Reassessment Timer duration (Multiple of CA timer duration)     */
/* - CFG_NVDS_TAG_BLE_CA_MIN_RSSI       Minimum RSSI Threshold                                                  */
/* - CFG_NVDS_TAG_BLE_CA_NB_PKT         Number of packets to receive for statistics                             */
/* - CFG_NVDS_TAG_BLE_CA_NB_BAD_PKT     Number  of bad packets needed to remove a channel                       */
/****************************************************************************************************************/
#define CFG_NVDS_TAG_BD_ADDRESS             {0x01, 0x00, 0x70, 0xCA, 0xEA, 0x80}

#define CFG_NVDS_TAG_LPCLK_DRIFT            DRIFT_500PPM
#define CFG_NVDS_TAG_BLE_CA_TIMER_DUR       2000
#define CFG_NVDS_TAG_BLE_CRA_TIMER_DUR      6
#define CFG_NVDS_TAG_BLE_CA_MIN_RSSI        0x40
#define CFG_NVDS_TAG_BLE_CA_NB_PKT          100
#define CFG_NVDS_TAG_BLE_CA_NB_BAD_PKT      50

/****************************************************************************************************************/
/* Enables the logging of heap memories usage. The feature can be used in development/debug mode.               */
/* Application must be executed in Keil debugger environment and "da14585_586.lib" must be replaced with        */
/* "da14585_586_with_heap_logging.lib" in project structure under sdk_arch. Developer must stop execution       */
/* and type disp_heaplog() in debugger's command window. Heap memory statistics will be displayed on window     */
/****************************************************************************************************************/
#undef CFG_LOG_HEAP_USAGE

/****************************************************************************************************************/
/* Enables the BLE statistics measurement feature.                                                              */
/****************************************************************************************************************/
#undef CFG_BLE_METRICS

/****************************************************************************************************************/
/* Output the Hardfault arguments to serial/UART interface.                                                     */
/****************************************************************************************************************/
#undef CFG_PRODUCTION_DEBUG_OUTPUT

/****************************************************************************************************************/
/* Maximum supported TX data packet length (supportedMaxTxOctets value, as defined in 4.2 Specification).       */
/* Range: 27 - 251 octets.                                                                                      */
/* NOTE 1: Even number of octets are not supported. A selected even number will be automatically converted to   */
/*         the next odd one.                                                                                    */
/* NOTE 2: The supportedMaxTxTime value is automatically calculated by the ROM code, according to the following */
/*         equation:                                                                                            */
/*             supportedMaxTxTime = (supportedMaxTxOctets + 11 + 3 ) * 8                                        */
/*         Range: 328 - 2120 usec.                                                                              */
/****************************************************************************************************************/
#define CFG_MAX_TX_PACKET_LENGTH        (251)

/****************************************************************************************************************/
/* Maximum supported RX data packet length (supportedMaxRxOctets value, as defined in 4.2 Specification).       */
/* Range: 27 - 251 octets.                                                                                      */
/* NOTE 1: Even number of octets are not supported. A selected even number will be automatically converted to   */
/*         the next odd one.                                                                                    */
/* NOTE 2: The supportedMaxRxTime value is automatically calculated by the ROM code, according to the following */
/*         equation:                                                                                            */
/*             supportedMaxRxTime = (supportedMaxRxOctets + 11 + 3 ) * 8                                        */
/*         Range: 328 - 2120 usec.                                                                              */
/****************************************************************************************************************/
#define CFG_MAX_RX_PACKET_LENGTH        (251)

/****************************************************************************************************************/
/* Select external application/host transport layer:                                                            */
/*     - 0 = GTL (auto)                                                                                         */
/*     - 1 = HCI (auto)                                                                                         */
/*     - 8 = GTL (fixed)                                                                                        */
/*     - 9 = HCI (fixed)                                                                                        */
/****************************************************************************************************************/
#define CFG_USE_H4TL                    (0)

/****************************************************************************************************************/
/* Duplicate filter max value for the scan report list. The maximum value shall be 100.                         */
/****************************************************************************************************************/
#define CFG_BLE_DUPLICATE_FILTER_MAX    (10)

/****************************************************************************************************************/
/* Duplicate filter flag for the scan report list. This flag controls what will be reported if the              */
/* CFG_BLE_DUPLICATE_FILTER_MAX number is exceeded.                                                             */
/*     - If the flag is defined, the extra devices are considered to be in the list and will not be reported.   */
/****************************************************************************************************************/
#undef CFG_BLE_DUPLICATE_FILTER_FOUND

/****************************************************************************************************************/
/* Resolving list maximum size.                                                                                 */
/****************************************************************************************************************/
#define CFG_LLM_RESOLVING_LIST_MAX      LLM_RESOLVING_LIST_MAX

/****************************************************************************************************************/
/* Enables automatic data packet length negotiation.                                                            */
/* NOTE: Enable only if peer device supports data length extension!!                                            */
/****************************************************************************************************************/
#undef AUTO_DATA_LENGTH_NEGOTIATION_UPON_NEW_CONNECTION

/****************************************************************************************************************/
/* Maximum retention memory in bytes. The base address of the retention data is calculated from the selected    */
/* size.                                                                                                        */
/****************************************************************************************************************/
#define CFG_RET_DATA_SIZE    (2048)

/****************************************************************************************************************/
/* Maximum uninitialized retained data required by the application.                                             */
/****************************************************************************************************************/
#define CFG_RET_DATA_UNINIT_SIZE (64)

/****************************************************************************************************************/
/* The Keil scatter file may be provided by the user. If the user provides his own scatter file, the system has */
/* to be aware which RAM blocks has to retain. The 4th RAM block is always retained, since it contains the ROM  */
/* data.                                                                                                        */
/*     - CFG_RETAIN_RAM_1_BLOCK: if defined, the 1st RAM block must be retained.                                */
/*     - CFG_RETAIN_RAM_2_BLOCK: if defined, the 2nd RAM block must be retained.                                */
/*     - CFG_RETAIN_RAM_3_BLOCK: if defined, the 3rd RAM block must be retained.                                */
/*                                                                                                              */
/* If the CFG_CUSTOM_SCATTER_FILE flag is undefined, the system knows which blocks to retain based on the       */
/* default SDK scatter file.                                                                                    */
/****************************************************************************************************************/
#undef CFG_CUSTOM_SCATTER_FILE
#ifdef CFG_CUSTOM_SCATTER_FILE
    #define CFG_RETAIN_RAM_1_BLOCK
    #define CFG_RETAIN_RAM_2_BLOCK
    #define CFG_RETAIN_RAM_3_BLOCK
#endif

/****************************************************************************************************************/
/* Code location selection.                                                                                     */
/*     - CFG_CODE_LOCATION_EXT: Code is loaded from SPI flash / I2C EEPROM / UART                               */
/*     - CFG_CODE_LOCATION_OTP: Code is burned in the OTP                                                       */
/* The above options are mutually exclusive and exactly one of them must be enabled.                            */
/****************************************************************************************************************/
#define CFG_CODE_LOCATION_EXT
#undef CFG_CODE_LOCATION_OTP

/****************************************************************************************************************/
/* Temperature range selection.                                                                                 */
/* - CFG_HIGH_TEMPERATURE:         Device is configured to operate at high temperature range (-40C to +105C).   */
/* - CFG_AMB_TEMPERATURE:          Device is configured to operate at ambient temperature range (-40C to +40C). */
/* - CFG_MID_TEMPERATURE:          Device is configured to operate at mid temperature range (-40C to +60C).     */
/* - CFG_EXT_TEMPERATURE:          Device is configured to operate at ext temperature range (-40C to +85C).     */
/* NOTE 1: High temperature support is not compatible with power optimizations. User shall undefine the         */
/*         CFG_POWER_OPTIMIZATIONS flag, if device is to support the high temperature range feature.            */
/****************************************************************************************************************/
#define CFG_AMB_TEMPERATURE

/****************************************************************************************************************/
/* Enable power optimizations using the XTAL16M adaptive settling algorithm.                                    */
/* NOTE: The XTAL16M adaptive settling algorithm works only with XTAL32K and not with RCX, as the LP clock.     */
/****************************************************************************************************************/
#define CFG_XTAL16M_ADAPTIVE_SETTLING



#else



/****************************************************************************************************************/
/* Low Power clock selection.                                                                                   */
/*      -LP_CLK_XTAL32      External XTAL32K oscillator                                                         */
/*      -LP_CLK_RCX20       Internal RCX clock                                                                  */
/*      -LP_CLK_FROM_OTP    Use the selection in the corresponding field of OTP Header                          */
/****************************************************************************************************************/
#define CFG_LP_CLK              LP_CLK_RCX20

/****************************************************************************************************************/
/* Periodic wakeup period to poll GTL iface. Time in msec.                                                      */
/****************************************************************************************************************/
#define CFG_MAX_SLEEP_DURATION_PERIODIC_WAKEUP_MS                  500  // 0.5s

/****************************************************************************************************************/
/* Periodic wakeup period if GTL iface is not enabled. Time in msec.                                            */
/****************************************************************************************************************/
#define CFG_MAX_SLEEP_DURATION_EXTERNAL_WAKEUP_MS              600000  // 600s

/****************************************************************************************************************/
/* Wakeup from external processor running host application.                                                     */
/****************************************************************************************************************/
#undef CFG_EXTERNAL_WAKEUP

/****************************************************************************************************************/
/* Wakeup external processor when a message is sent to GTL                                                      */
/****************************************************************************************************************/
#undef CFG_WAKEUP_EXT_PROCESSOR

/****************************************************************************************************************/
/* Enables True Random Number Generator. A true random number, generated at system initialization, is used to   */
/* seed any random number generator (C standard library, ChaCha20, etc.).                                       */
/****************************************************************************************************************/
#define CFG_TRNG

/****************************************************************************************************************/
/* Creation of private and public keys using Elliptic Curve Diffie Hellman algorithms.                          */
/* - defined:   Creation of ECDH keys and secure connection feature is enabled.                                 */
/* - undefined: Creation of ECDH keys and secure connection feature is disabled. If application does not        */
/*              support secure connections, it is reccomended to undefine CFG_ENABLE_SMP_SECURE in order to     */
/*              enable faster start-up time and reduce code size.                                               */
/****************************************************************************************************************/
#undef CFG_ENABLE_SMP_SECURE

/****************************************************************************************************************/
/* Uses ChaCha20 random number generator instead of the C standard library random number generator.             */
/****************************************************************************************************************/
#undef CFG_USE_CHACHA20_RAND

/****************************************************************************************************************/
/* Custom heap sizes                                                                                            */
/****************************************************************************************************************/
// #define DB_HEAP_SZ              1024
// #define ENV_HEAP_SZ             4928
// #define MSG_HEAP_SZ             6880
// #define NON_RET_HEAP_SZ         2048

/****************************************************************************************************************/
/* NVDS configuration                                                                                           */
/* - CFG_NVDS_TAG_BD_ADDRESS            Default bdaddress. If bdaddress is written in OTP header this value is  */
/*                                      ignored                                                                 */
/* - CFG_NVDS_TAG_LPCLK_DRIFT           Low power clock drift. Permitted values in ppm are:                     */
/*      + DRIFT_20PPM                                                                                           */
/*      + DRIFT_30PPM                                                                                           */
/*      + DRIFT_50PPM                                                                                           */
/*      + DRIFT_75PPM                                                                                           */
/*      + DRIFT_100PPM                                                                                          */
/*      + DRIFT_150PPM                                                                                          */
/*      + DRIFT_250PPM                                                                                          */
/*      + DRIFT_500PPM                  Default value (500 ppm)                                                 */
/* - CFG_NVDS_TAG_BLE_CA_TIMER_DUR      Channel Assessment Timer duration (Multiple of 10ms)                    */
/* - CFG_NVDS_TAG_BLE_CRA_TIMER_DUR     Channel Reassessment Timer duration (Multiple of CA timer duration)     */
/* - CFG_NVDS_TAG_BLE_CA_MIN_RSSI       Minimum RSSI Threshold                                                  */
/* - CFG_NVDS_TAG_BLE_CA_NB_PKT         Number of packets to receive for statistics                             */
/* - CFG_NVDS_TAG_BLE_CA_NB_BAD_PKT     Number  of bad packets needed to remove a channel                       */
/****************************************************************************************************************/
#define CFG_NVDS_TAG_BD_ADDRESS             {0x01, 0x00, 0x70, 0xCA, 0xEA, 0x80}

#define CFG_NVDS_TAG_LPCLK_DRIFT            DRIFT_500PPM
#define CFG_NVDS_TAG_BLE_CA_TIMER_DUR       2000
#define CFG_NVDS_TAG_BLE_CRA_TIMER_DUR      6
#define CFG_NVDS_TAG_BLE_CA_MIN_RSSI        0x40
#define CFG_NVDS_TAG_BLE_CA_NB_PKT          100
#define CFG_NVDS_TAG_BLE_CA_NB_BAD_PKT      50

/****************************************************************************************************************/
/* Enables the logging of heap memories usage. The feature can be used in development/debug mode.               */
/* Application must be executed in Keil debugger environment and "da14531.lib" must be replaced with            */
/* "da14531_with_heap_logging.lib" in project structure under sdk_arch. Developer must stop execution           */
/* and type disp_heaplog() in debugger's command window. Heap memory statistics will be displayed on window     */
/****************************************************************************************************************/
#undef CFG_LOG_HEAP_USAGE

/****************************************************************************************************************/
/* Enables the BLE statistics measurement feature.                                                              */
/****************************************************************************************************************/
#undef CFG_BLE_METRICS

/****************************************************************************************************************/
/* Output the Hardfault arguments to serial/UART interface.                                                     */
/****************************************************************************************************************/
#undef CFG_PRODUCTION_DEBUG_OUTPUT

/****************************************************************************************************************/
/* Maximum supported TX data packet length (supportedMaxTxOctets value, as defined in 4.2 Specification).       */
/* Range: 27 - 251 octets.                                                                                      */
/* NOTE 1: Even number of octets are not supported. A selected even number will be automatically converted to   */
/*         the next odd one.                                                                                    */
/* NOTE 2: The supportedMaxTxTime value is automatically calculated by the ROM code, according to the following */
/*         equation:                                                                                            */
/*             supportedMaxTxTime = (supportedMaxTxOctets + 11 + 3 ) * 8                                        */
/*         Range: 328 - 2120 usec.                                                                              */
/****************************************************************************************************************/
#define CFG_MAX_TX_PACKET_LENGTH        (251)

/****************************************************************************************************************/
/* Maximum supported RX data packet length (supportedMaxRxOctets value, as defined in 4.2 Specification).       */
/* Range: 27 - 251 octets.                                                                                      */
/* NOTE 1: Even number of octets are not supported. A selected even number will be automatically converted to   */
/*         the next odd one.                                                                                    */
/* NOTE 2: The supportedMaxRxTime value is automatically calculated by the ROM code, according to the following */
/*         equation:                                                                                            */
/*             supportedMaxRxTime = (supportedMaxRxOctets + 11 + 3 ) * 8                                        */
/*         Range: 328 - 2120 usec.                                                                              */
/****************************************************************************************************************/
#define CFG_MAX_RX_PACKET_LENGTH        (251)

/****************************************************************************************************************/
/* Select external application/host transport layer:                                                            */
/*     - 0 = GTL (auto)                                                                                         */
/*     - 1 = HCI (auto)                                                                                         */
/*     - 8 = GTL (fixed)                                                                                        */
/*     - 9 = HCI (fixed)                                                                                        */
/****************************************************************************************************************/
#define CFG_USE_H4TL                    (0)

/****************************************************************************************************************/
/* Duplicate filter max value for the scan report list. The maximum value shall be 100.                         */
/****************************************************************************************************************/
#define CFG_BLE_DUPLICATE_FILTER_MAX    (10)

/****************************************************************************************************************/
/* Duplicate filter flag for the scan report list. This flag controls what will be reported if the              */
/* CFG_BLE_DUPLICATE_FILTER_MAX number is exceeded.                                                             */
/*     - If the flag is defined, the extra devices are considered to be in the list and will not be reported.   */
/****************************************************************************************************************/
#undef CFG_BLE_DUPLICATE_FILTER_FOUND

/****************************************************************************************************************/
/* Resolving list maximum size.                                                                                 */
/****************************************************************************************************************/
#define CFG_LLM_RESOLVING_LIST_MAX      LLM_RESOLVING_LIST_MAX

/****************************************************************************************************************/
/* Enables automatic data packet length negotiation.                                                            */
/* NOTE: Enable only if peer device supports data length extension!!                                            */
/****************************************************************************************************************/
#undef AUTO_DATA_LENGTH_NEGOTIATION_UPON_NEW_CONNECTION

/****************************************************************************************************************/
/* Maximum retention memory in bytes. The base address of the retention data is calculated from the selected    */
/* size.                                                                                                        */
/****************************************************************************************************************/
#define CFG_RET_DATA_SIZE    (2048)

/****************************************************************************************************************/
/* Maximum uninitialized retained data required by the application.                                             */
/****************************************************************************************************************/
// Note: There's two possible allocations we need here for Memfault
// 1. MEMFAULT_REBOOT_TRACKING_REGION_SIZE (64 bytes) - used to hold info about resets taking place
// 2. MEMFAULT_PLATFORM_COREDUMP_STORAGE_RAM_SIZE if and only if MEMFAULT_PLATFORM_COREDUMP_STORAGE_USE_RAM=1
//    We recommend storing coredumps to flash since there is more space typically available
#define CFG_RET_DATA_UNINIT_SIZE (64)

/****************************************************************************************************************/
/* The Keil scatter file may be provided by the user. If the user provides his own scatter file, the system has */
/* to be aware which RAM blocks has to retain. The 3rd RAM block is always retained, since it contains the ROM  */
/* data.                                                                                                        */
/*     - CFG_RETAIN_RAM_1_BLOCK: if defined, the 1st RAM block must be retained.                                */
/*     - CFG_RETAIN_RAM_2_BLOCK: if defined, the 2nd RAM block must be retained.                                */
/*                                                                                                              */
/* If the CFG_CUSTOM_SCATTER_FILE flag is undefined, the system knows which blocks to retain based on the       */
/* default SDK scatter file.                                                                                    */
/****************************************************************************************************************/
#undef CFG_CUSTOM_SCATTER_FILE
#ifdef CFG_CUSTOM_SCATTER_FILE
    #define CFG_RETAIN_RAM_1_BLOCK
    #define CFG_RETAIN_RAM_2_BLOCK
#endif

/****************************************************************************************************************/
/* Code location selection.                                                                                     */
/*     - CFG_CODE_LOCATION_EXT: Code is loaded from SPI flash / I2C EEPROM / UART                               */
/*     - CFG_CODE_LOCATION_OTP: Code is burned in the OTP                                                       */
/* The above options are mutually exclusive and exactly one of them must be enabled.                            */
/****************************************************************************************************************/
#define CFG_CODE_LOCATION_EXT
#undef CFG_CODE_LOCATION_OTP

/****************************************************************************************************************/
/* Temperature range selection.                                                                                 */
/* - CFG_HIGH_TEMPERATURE:         Device is configured to operate at high temperature range (-40C to +105C).   */
/* - CFG_AMB_TEMPERATURE:          Device is configured to operate at ambient temperature range (-40C to +40C). */
/* - CFG_MID_TEMPERATURE:          Device is configured to operate at mid temperature range (-40C to +60C).     */
/* - CFG_EXT_TEMPERATURE:          Device is configured to operate at ext temperature range (-40C to +85C).     */
/****************************************************************************************************************/
#define CFG_AMB_TEMPERATURE

/****************************************************************************************************************/
/* Disable quadrature decoder on start up. The quadrature decoder is by default enabled on system power up and  */
/* it may count events. This leads to WKUP_QUADEC_IRQn pending interrupts.                                      */
/****************************************************************************************************************/
#define CFG_DISABLE_QUADEC_ON_START_UP

#endif
#endif // _DA1458X_CONFIG_ADVANCED_H_
