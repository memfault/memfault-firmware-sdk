/**
 ****************************************************************************************
 *
 * @file user_modules_config.h
 *
 * @brief User modules configuration file.
 *
 * Copyright (C) 2015-2019 Dialog Semiconductor.
 * This computer program includes Confidential, Proprietary Information
 * of Dialog Semiconductor. All Rights Reserved.
 *
 ****************************************************************************************
 */

#ifndef _USER_MODULES_CONFIG_H_
#define _USER_MODULES_CONFIG_H_

/**
 ****************************************************************************************
 * @addtogroup APP
 * @ingroup RICOW
 *
 * @brief User modules configuration.
 *
 * @{
 ****************************************************************************************
 */

/*
 * DEFINES
 ****************************************************************************************
 */

/***************************************************************************************/
/* Exclude or not a module in user's application code.                                 */
/*                                                                                     */
/* (0) - The module is included. The module's messages are handled by the SDK.         */
/*                                                                                     */
/* (1) - The module is excluded. The user must handle the module's messages.           */
/*                                                                                     */
/* Note:                                                                               */
/*      This setting has no effect if the respective module is a BLE Profile           */
/*      that is not used included in the user's application.                           */
/***************************************************************************************/
#define EXCLUDE_DLG_GAP             (0)
#define EXCLUDE_DLG_TIMER           (0)
#define EXCLUDE_DLG_MSG             (0)
#define EXCLUDE_DLG_SEC             (0)
#define EXCLUDE_DLG_DISS            (0)
#define EXCLUDE_DLG_PROXR           (0)
#define EXCLUDE_DLG_BASS            (0)
#define EXCLUDE_DLG_FINDL           (0)
#define EXCLUDE_DLG_FINDT           (0)
#define EXCLUDE_DLG_SUOTAR          (0)
#define EXCLUDE_DLG_CUSTS1          (0)
#define EXCLUDE_DLG_CUSTS2          (0)

/// @} APP

#endif // _USER_MODULES_CONFIG_H_
