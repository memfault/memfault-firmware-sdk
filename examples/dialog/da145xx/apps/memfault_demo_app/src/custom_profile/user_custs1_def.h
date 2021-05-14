/**
 ****************************************************************************************
 *
 * @file user_custs1_def.h
 *
 * @brief Custom Server 1 (CUSTS1) profile database definitions.
 *
 * Copyright (C) 2016-2019 Dialog Semiconductor.
 * This computer program includes Confidential, Proprietary Information
 * of Dialog Semiconductor. All Rights Reserved.
 *
 ****************************************************************************************
 */

#ifndef _USER_CUSTS1_DEF_H_
#define _USER_CUSTS1_DEF_H_

/**
 ****************************************************************************************
 * @defgroup USER_CONFIG
 * @ingroup USER
 * @brief Custom Server 1 (CUSTS1) profile database definitions.
 *
 * @{
 ****************************************************************************************
 */

/*
 * INCLUDE FILES
 ****************************************************************************************
 */

#include "attm_db_128.h"

/*
 * DEFINES
 ****************************************************************************************
 */

// Service 1 of the custom server 1
#define DEF_SVC1_UUID_128                {0x59, 0x5a, 0x08, 0xe4, 0x86, 0x2a, 0x9e, 0x8f, 0xe9, 0x11, 0xbc, 0x7c, 0x98, 0x43, 0x42, 0x18}

#define DEF_SVC1_CTRL_POINT_UUID_128     {0x20, 0xEE, 0x8D, 0x0C, 0xE1, 0xF0, 0x4A, 0x0C, 0xB3, 0x25, 0xDC, 0x53, 0x6A, 0x68, 0x86, 0x2D}
#define DEF_SVC1_LED_STATE_UUID_128      {0x4F, 0x43, 0x31, 0x3C, 0x93, 0x92, 0x42, 0xE6, 0xA8, 0x76, 0xFA, 0x3B, 0xEF, 0xB4, 0x87, 0x5A}
#define DEF_SVC1_ADC_VAL_1_UUID_128      {0x17, 0xB9, 0x67, 0x98, 0x4C, 0x66, 0x4C, 0x01, 0x96, 0x33, 0x31, 0xB1, 0x91, 0x59, 0x00, 0x15}
#define DEF_SVC1_ADC_VAL_2_UUID_128      {0x23, 0x68, 0xEC, 0x52, 0x1E, 0x62, 0x44, 0x74, 0x9A, 0x1B, 0xD1, 0x8B, 0xAB, 0x75, 0xB6, 0x6E}
#define DEF_SVC1_BUTTON_STATE_UUID_128   {0x9E, 0xE7, 0xBA, 0x08, 0xB9, 0xA9, 0x48, 0xAB, 0xA1, 0xAC, 0x03, 0x1C, 0x2E, 0x0D, 0x29, 0x6C}
#define DEF_SVC1_INDICATEABLE_UUID_128   {0x28, 0xD5, 0xE1, 0xC1, 0xE1, 0xC5, 0x47, 0x29, 0xB5, 0x57, 0x65, 0xC3, 0xBA, 0x47, 0x15, 0x9E}
#define DEF_SVC1_LONG_VALUE_UUID_128     {0x8C, 0x09, 0xE0, 0xD1, 0x81, 0x54, 0x42, 0x40, 0x8E, 0x4F, 0xD2, 0xB3, 0x77, 0xE3, 0x2A, 0x77}

#define DEF_SVC1_CTRL_POINT_CHAR_LEN     1
#define DEF_SVC1_LED_STATE_CHAR_LEN      1
#define DEF_SVC1_ADC_VAL_1_CHAR_LEN      2
#define DEF_SVC1_ADC_VAL_2_CHAR_LEN      2
#define DEF_SVC1_BUTTON_STATE_CHAR_LEN   1
#define DEF_SVC1_INDICATEABLE_CHAR_LEN   20
#define DEF_SVC1_LONG_VALUE_CHAR_LEN     50

#define DEF_SVC1_CONTROL_POINT_USER_DESC     "Control Point"
#define DEF_SVC1_LED_STATE_USER_DESC         "LED State"
#define DEF_SVC1_ADC_VAL_1_USER_DESC         "ADC Value 1"
#define DEF_SVC1_ADC_VAL_2_USER_DESC         "ADC Value 2"
#define DEF_SVC1_BUTTON_STATE_USER_DESC      "Button State"
#define DEF_SVC1_INDICATEABLE_USER_DESC      "Indicateable"
#define DEF_SVC1_LONG_VALUE_CHAR_USER_DESC   "Long Value"

// Service 2 of the custom server 1
#define DEF_SVC2_UUID_128                {0x59, 0x5a, 0x08, 0xe4, 0x86, 0x2a, 0x9e, 0x8f, 0xe9, 0x11, 0xbc, 0x7c, 0x7c, 0x46, 0x42, 0x18}

#define DEF_SVC2_WRITE_VAL_1_UUID_128    {0x20, 0xEE, 0x8D, 0x0C, 0xE1, 0xF0, 0x4A, 0x0C, 0xB3, 0x25, 0xDC, 0x53, 0x6A, 0x68, 0x86, 0x2C}
#define DEF_SVC2_WRITE_VAL_2_UUID_128    {0x4F, 0x43, 0x31, 0x3C, 0x93, 0x92, 0x42, 0xE6, 0xA8, 0x76, 0xFA, 0x3B, 0xEF, 0xB4, 0x87, 0x59}

#define DEF_SVC2_WRITE_VAL_1_CHAR_LEN    1
#define DEF_SVC2_WRITE_VAL_2_CHAR_LEN    1

#define DEF_SVC2_WRITE_VAL_1_USER_DESC   "Write me"
#define DEF_SVC2_WRITE_VAL_2_USER_DESC   "Write me (no rsp)"

// Service 3 of the custom server 1
#define DEF_SVC3_UUID_128                {0x59, 0x5a, 0x08, 0xe4, 0x86, 0x2a, 0x9e, 0x8, 0xe9, 0x11, 0xbc, 0x7c, 0xd0, 0x47, 0x42, 0x18}

#define DEF_SVC3_READ_VAL_1_UUID_128     {0x17, 0xB9, 0x67, 0x98, 0x4C, 0x66, 0x4C, 0x01, 0x96, 0x33, 0x31, 0xB1, 0x91, 0x59, 0x00, 0x14}
#define DEF_SVC3_READ_VAL_2_UUID_128     {0x23, 0x68, 0xEC, 0x52, 0x1E, 0x62, 0x44, 0x74, 0x9A, 0x1B, 0xD1, 0x8B, 0xAB, 0x75, 0xB6, 0x6D}
#define DEF_SVC3_READ_VAL_3_UUID_128     {0x28, 0xD5, 0xE1, 0xC1, 0xE1, 0xC5, 0x47, 0x29, 0xB5, 0x57, 0x65, 0xC3, 0xBA, 0x47, 0x15, 0x9D}

#define DEF_SVC3_READ_VAL_1_CHAR_LEN     2
#define DEF_SVC3_READ_VAL_2_CHAR_LEN     2
#define DEF_SVC3_READ_VAL_3_CHAR_LEN     2

#define DEF_SVC3_READ_VAL_1_USER_DESC    "Read me (notify)"
#define DEF_SVC3_READ_VAL_2_USER_DESC    "Read me"
#define DEF_SVC3_READ_VAL_3_USER_DESC    "Read me (indicate)"

/// Custom1 Service Data Base Characteristic enum
enum
{
    // Custom Service 1
    SVC1_IDX_SVC = 0,

    SVC1_IDX_CONTROL_POINT_CHAR,
    SVC1_IDX_CONTROL_POINT_VAL,
    SVC1_IDX_CONTROL_POINT_USER_DESC,

    SVC1_IDX_LED_STATE_CHAR,
    SVC1_IDX_LED_STATE_VAL,
    SVC1_IDX_LED_STATE_USER_DESC,

    SVC1_IDX_ADC_VAL_1_CHAR,
    SVC1_IDX_ADC_VAL_1_VAL,
    SVC1_IDX_ADC_VAL_1_NTF_CFG,
    SVC1_IDX_ADC_VAL_1_USER_DESC,

    SVC1_IDX_ADC_VAL_2_CHAR,
    SVC1_IDX_ADC_VAL_2_VAL,
    SVC1_IDX_ADC_VAL_2_USER_DESC,

    SVC1_IDX_BUTTON_STATE_CHAR,
    SVC1_IDX_BUTTON_STATE_VAL,
    SVC1_IDX_BUTTON_STATE_NTF_CFG,
    SVC1_IDX_BUTTON_STATE_USER_DESC,

    SVC1_IDX_INDICATEABLE_CHAR,
    SVC1_IDX_INDICATEABLE_VAL,
    SVC1_IDX_INDICATEABLE_IND_CFG,
    SVC1_IDX_INDICATEABLE_USER_DESC,

    SVC1_IDX_LONG_VALUE_CHAR,
    SVC1_IDX_LONG_VALUE_VAL,
    SVC1_IDX_LONG_VALUE_NTF_CFG,
    SVC1_IDX_LONG_VALUE_USER_DESC,
    
    // Custom Service 2
    SVC2_IDX_SVC,
    
    SVC2_WRITE_1_CHAR,
    SVC2_WRITE_1_VAL,
    SVC2_WRITE_1_USER_DESC,

    SVC2_WRITE_2_CHAR,
    SVC2_WRITE_2_VAL,
    SVC2_WRITE_2_USER_DESC,
    
    // Custom Service 3
    SVC3_IDX_SVC,
    
    SVC3_IDX_READ_1_CHAR,
    SVC3_IDX_READ_1_VAL,
    SVC3_IDX_READ_1_NTF_CFG,
    SVC3_IDX_READ_1_USER_DESC,

    SVC3_IDX_READ_2_CHAR,
    SVC3_IDX_READ_2_VAL,
    SVC3_IDX_READ_2_USER_DESC,
    
    SVC3_IDX_READ_3_CHAR,
    SVC3_IDX_READ_3_VAL,
    SVC3_IDX_READ_3_IND_CFG,
    SVC3_IDX_READ_3_USER_DESC,

    CUSTS1_IDX_NB
};

/// @} USER_CONFIG

#endif // _USER_CUSTS1_DEF_H_
