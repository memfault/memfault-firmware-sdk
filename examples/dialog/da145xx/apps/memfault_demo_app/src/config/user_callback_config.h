/**
 ****************************************************************************************
 *
 * @file user_callback_config.h
 *
 * @brief Callback functions configuration file.
 *
 * Copyright (C) 2015-2019 Dialog Semiconductor.
 * This computer program includes Confidential, Proprietary Information
 * of Dialog Semiconductor. All Rights Reserved.
 *
 ****************************************************************************************
 */

#ifndef _USER_CALLBACK_CONFIG_H_
#define _USER_CALLBACK_CONFIG_H_

/*
 * INCLUDE FILES
 ****************************************************************************************
 */

#include "app_api.h"
#include "app_bass.h"
#include "app_findme.h"
#include "app_proxr.h"
#include "app_suotar.h"
#include "app_callback.h"
#include "app_prf_types.h"
#if (BLE_APP_SEC)
#include "app_bond_db.h"
#endif // (BLE_APP_SEC)
#include "user_app.h"

/*
 * FUNCTION DECLARATIONS
 ****************************************************************************************
 */

/**
 ****************************************************************************************
 * @brief Function to be called on the advertising completion event.
 * @param[in] uint8_t GAP Error code
 ****************************************************************************************
 */
void app_advertise_complete(const uint8_t);

/**
 ****************************************************************************************
 * @brief SUOTAR session start or stop event handler.
 * @param[in] suotar_event SUOTAR_START/SUOTAR_STOP
 ****************************************************************************************
 */
void on_suotar_status_change(const uint8_t suotar_event);


/*
 * LOCAL VARIABLE DEFINITIONS
 ****************************************************************************************
 */

#if (BLE_BATT_SERVER)
static const struct app_bass_cb user_app_bass_cb = {
    .on_batt_level_upd_rsp      = NULL,
    .on_batt_level_ntf_cfg_ind  = NULL,
};
#endif

#if (BLE_FINDME_TARGET)
static const struct app_findt_cb user_app_findt_cb = {
    .on_findt_alert_ind         = default_findt_alert_ind_handler,
};
#endif

#if (BLE_PROX_REPORTER)
static const struct app_proxr_cb user_app_proxr_cb = {
    .on_proxr_alert_ind      = default_proxr_alert_ind_handler,
};
#endif

#if (BLE_SUOTA_RECEIVER)
static const struct app_suotar_cb user_app_suotar_cb = {
    .on_suotar_status_change = on_suotar_status_change,
};
#endif

static const struct app_callbacks user_app_callbacks = {
    .app_on_connection                  = user_on_connection,
    .app_on_disconnect                  = user_on_disconnect,
    .app_on_update_params_rejected      = NULL,
    .app_on_update_params_complete      = NULL,
    .app_on_set_dev_config_complete     = default_app_on_set_dev_config_complete,
    .app_on_adv_nonconn_complete        = NULL,
    .app_on_adv_undirect_complete       = NULL,
    .app_on_adv_direct_complete         = NULL,
    .app_on_db_init_complete            = default_app_on_db_init_complete,
    .app_on_scanning_completed          = NULL,
    .app_on_adv_report_ind              = NULL,
    .app_on_get_dev_name                = default_app_on_get_dev_name,
    .app_on_get_dev_appearance          = default_app_on_get_dev_appearance,
    .app_on_get_dev_slv_pref_params     = default_app_on_get_dev_slv_pref_params,
    .app_on_set_dev_info                = default_app_on_set_dev_info,
    .app_on_data_length_change          = NULL,
    .app_on_update_params_request       = default_app_update_params_request,
    .app_on_generate_static_random_addr = default_app_generate_static_random_addr,
    .app_on_svc_changed_cfg_ind         = NULL,
    .app_on_get_peer_features           = NULL,
#if (BLE_APP_SEC)
    .app_on_pairing_request             = default_app_on_pairing_request,
    .app_on_tk_exch                     = default_app_on_tk_exch,
    .app_on_irk_exch                    = NULL,
    .app_on_csrk_exch                   = NULL,
    .app_on_ltk_exch                    = default_app_on_ltk_exch,
    .app_on_pairing_succeeded           = NULL,
    .app_on_encrypt_ind                 = NULL,
    .app_on_encrypt_req_ind             = NULL,
    .app_on_security_req_ind            = NULL,
    .app_on_addr_solved_ind             = NULL,
    .app_on_addr_resolve_failed         = NULL,
    .app_on_ral_cmp_evt                 = NULL,
    .app_on_ral_size_ind                = NULL,
    .app_on_ral_addr_ind                = NULL,
#endif // (BLE_APP_SEC)
};

#if (BLE_APP_SEC)
static const struct app_bond_db_callbacks user_app_bond_db_callbacks = {
    .app_bdb_init                       = NULL,
    .app_bdb_get_size                   = NULL,
    .app_bdb_add_entry                  = NULL,
    .app_bdb_remove_entry               = NULL,
    .app_bdb_search_entry               = NULL,
    .app_bdb_get_number_of_stored_irks  = NULL,
    .app_bdb_get_stored_irks            = NULL,
    .app_bdb_get_device_info_from_slot  = NULL,
};
#endif // (BLE_APP_SEC)

/*
 * "app_process_catch_rest_cb" symbol handling:
 * - Use #define if "user_catch_rest_hndl" is defined by the user
 * - Use const declaration if "user_catch_rest_hndl" is NULL
 */
//#define app_process_catch_rest_cb       user_catch_rest_hndl
//static const catch_rest_event_func_t app_process_catch_rest_cb = NULL;
#if defined ( __CC_ARM )
static const catch_rest_event_func_t app_process_catch_rest_cb = NULL;
#elif defined ( __GNUC__ )
#define app_process_catch_rest_cb ((const catch_rest_event_func_t)NULL)
#endif

static const struct default_app_operations user_default_app_operations = {
    .default_operation_adv = default_advertise_operation,
};

static const struct arch_main_loop_callbacks user_app_main_loop_callbacks = {
    .app_on_init            = user_app_on_init,

    // By default the watchdog timer is reloaded and resumed when the system wakes up.
    // The user has to take into account the watchdog timer handling (keep it running,
    // freeze it, reload it, resume it, etc), when the app_on_ble_powered() is being
    // called and may potentially affect the main loop.
    .app_on_ble_powered     = NULL,

    // By default the watchdog timer is reloaded and resumed when the system wakes up.
    // The user has to take into account the watchdog timer handling (keep it running,
    // freeze it, reload it, resume it, etc), when the app_on_system_powered() is being
    // called and may potentially affect the main loop.
    .app_on_system_powered  = user_app_on_system_powered,

    .app_before_sleep       = NULL,
    .app_validate_sleep     = NULL,
    .app_going_to_sleep     = NULL,
    .app_resume_from_sleep  = NULL,
};

//place in this structure the app_<profile>_db_create and app_<profile>_enable functions
//for SIG profiles that do not have this function already implemented in the SDK
//or if you want to override the functionality. Check the prf_func array in the SDK
//for your reference of which profiles are supported.
static const struct prf_func_callbacks user_prf_funcs[] =
{
    {TASK_ID_INVALID,    NULL, NULL}   // DO NOT MOVE. Must always be last
};

#endif // _USER_CALLBACK_CONFIG_H_
