//! @file
//!
//! Copyright (c) Memfault, Inc.
//! See License.txt for details
//!
//! A port of the Memfault Diagnostic GATT Service (MDS) to the DA1469x SDK.
//! See mds.h header for more details

#include "memfault/ports/ble/mds.h"

#if defined(CONFIG_USE_BLE_SERVICES)

#if defined(dg_configUSE_MEMFAULT)

#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>

//! Note: not sorting because header order matters
#include "ble_gap.h"
#include "osal.h"
#include "attm_cfg.h"
#include "sdk_queue.h"
#include "ble_att.h"
#include "ble_storage.h"
#include "ble_bufops.h"
#include "ble_common.h"
#include "ble_gatts.h"
#include "ble_gattc.h"
#include "ble_uuid.h"
#include "ble_service.h"

#include "FreeRTOS.h"
#include "timers.h"

#include "memfault/components.h"

#if !defined(MEMFAULT_PROJECT_KEY)
#error "Memfault Project Key not configured. Please visit https://goto.memfault.com/create-key/da1469x"
#endif

//! Profile makes use of the timer task for checking to see if there is any data to send. By
//! default, the FreeRTOS timer task depth is the absolute minimum configMINIMAL_STACK_SIZE (100),
//! so we need a little more room to prevent any stack overflows
MEMFAULT_STATIC_ASSERT(configTIMER_TASK_STACK_DEPTH >= 256,
                       "configTIMER_TASK_STACK_DEPTH must be >= 256, update your custom_config_*.h");

//! ATT Read, Write & Notification responses will have a 3 byte overhead
//! (1 Byte for Opcode + 2 bytes for length)
#define MDS_ATT_HEADER_OVERHEAD 3

//! Note: Attributes that are greater than the MTU size can be returned via long attribute reads
//! but the maximum allowed attribute value is 512 bytes. (See "3.2.9 Long attribute values" of BLE
//! v5.3 Core specification). In practice, all values returned by MDS should be much smaller than
//! this.
#define MDS_MAX_READ_LEN  (512)

//! The interval to check for whether or not there is any new data to send
#ifndef MDS_POLL_INTERVAL_MS
#define MDS_POLL_INTERVAL_MS (60 * 1000)
#endif

#ifndef MDS_MAX_DATA_URI_LENGTH
#define MDS_MAX_DATA_URI_LENGTH 64
#endif

#ifndef MDS_URI_BASE
//! i.e https://chunks.memfault.com/api/v0/chunks/
#define MDS_URI_BASE \
  (MEMFAULT_HTTP_APIS_DEFAULT_SCHEME "://" MEMFAULT_HTTP_CHUNKS_API_HOST "/api/v0/chunks/")
#endif

#ifndef MDS_DYNAMIC_ACCESS_CONTROL
#define MDS_DYNAMIC_ACCESS_CONTROL 0
#endif

#define MDS_VERSION 0x01

#define MDS_MAJOR_VERSION 0x01
#define MDS_MINOR_VERSION 0x00
#define MDS_PATCH_VERSION 0x00

const static uint8_t s_mds_version[] = { MDS_VERSION, MDS_MINOR_VERSION, MDS_PATCH_VERSION};

typedef struct {
  ble_service_t svc;

  uint16_t version_h;
  uint16_t data_uri_h;
  uint16_t auth_h;
  uint16_t device_id_h;

  uint16_t chunk_val_h;
  uint16_t chunk_cccd_h;

  // Note: MDS only allows one active subscriber at any given time
  //
  // If a second connection attempts to subscribe while a first connection is already active, the
  // second request will be ignored.
  struct {
    bool active;
    uint16_t conn_idx;
  } subscriber;

  struct {
    void *buf;
    size_t buf_len;
  } chunk;

  TimerHandle_t timer;
} md_service_t;

typedef enum {
  kMdsAppError_InvalidLength = ATT_ERROR_APPLICATION_ERROR,
  kMdsAppError_ClientAlreadySubscribed,
  kMdsAppError_InsufficientLength,
} eMdsAppError;

static md_service_t *s_mds;

#if !MDS_DYNAMIC_ACCESS_CONTROL
//! See mds.h header for more details, we recommend end user override this behavior for production
//! applications
MEMFAULT_WEAK
bool mds_access_enabled(uint16_t connection_handle) {
  return true;
}
#endif

static void prv_handle_disconnected_evt(ble_service_t *svc,
                                    const ble_evt_gap_disconnected_t *evt) {
  md_service_t *mds = (md_service_t *)svc;

  if (mds->subscriber.active && (mds->subscriber.conn_idx == evt->conn_idx)) {
    mds->subscriber.active = false;
    mds->subscriber.conn_idx = 0;
  }

  xTimerStop(mds->timer, 0);
}

static void prv_try_notify(md_service_t *mds, uint16_t conn_idx) {
  if ((!mds->subscriber.active) || (mds->subscriber.conn_idx != conn_idx)) {
    // caller has _not_ subscribed for chunk notifications so we do
    // not want to send the info to them
    return;
  }

  uint16_t mtu_size = 0;
  ble_error_t rv = ble_gattc_get_mtu(conn_idx, &mtu_size);
  if (rv != BLE_STATUS_OK) {
    return;
  }

  if ((mds->chunk.buf == NULL) && memfault_packetizer_data_available()) {
    mds->chunk.buf = OS_MALLOC(mtu_size - MDS_ATT_HEADER_OVERHEAD);
    if (mds->chunk.buf != NULL) {
      mds->chunk.buf_len = mtu_size - MDS_ATT_HEADER_OVERHEAD;
      memfault_packetizer_get_chunk(mds->chunk.buf, &mds->chunk.buf_len);
    }
  }

  if (mds->chunk.buf_len != 0) {
    rv = ble_gatts_send_event(conn_idx, mds->chunk_val_h, GATT_EVENT_NOTIFICATION,
                              mds->chunk.buf_len, mds->chunk.buf);
    if (rv == BLE_STATUS_OK) {
      // Note: No need to schedule a retry since we will pump more data when the sent callback is
      // invoked for the current notification
      OS_FREE(mds->chunk.buf);
      mds->chunk.buf = NULL;
      mds->chunk.buf_len = 0;
      return;
    }
  }

  // There's no more data, we were unable to allocate a buffer to hold a chunk or our send request
  // failed. Let's check to see if there is any more data / try again in 60 seconds.
  xTimerChangePeriod(mds->timer, pdMS_TO_TICKS(MDS_POLL_INTERVAL_MS), 0);
  xTimerStart(mds->timer, 0);
}

//! A timer service run that periodically checks to see if there
//! is any new data to send to Memfault while connected
static void prv_mds_timer_callback(MEMFAULT_UNUSED TimerHandle_t handle) {
  if (s_mds == NULL) {
    // unexpected ... suggests user destroyed MDS while a connection was actively subscribed to it.
    return;
  }

  uint8_t num_conn;
  uint16_t *conn_idx = NULL;

  ble_gap_get_connected(&num_conn, &conn_idx);

  while (num_conn--) {
    prv_try_notify(s_mds, conn_idx[num_conn]);
  }

  if (conn_idx) {
    OS_FREE(conn_idx);
  }
}

static void prv_handle_read_req(ble_service_t *svc,
                            const ble_evt_gatts_read_req_t *evt) {
  if (!mds_access_enabled(evt->conn_idx)) {
    ble_gatts_read_cfm(evt->conn_idx, evt->handle, ATT_ERROR_READ_NOT_PERMITTED, 0, NULL);
    return;
  }

  const md_service_t *mds = (md_service_t *)svc;

  const void *value = NULL;
  size_t length = 0;
  char uri[MDS_MAX_DATA_URI_LENGTH];
  struct MemfaultDeviceInfo info;

  if (evt->handle == mds->version_h) {
    value = &s_mds_version;
    length = sizeof(s_mds_version);
  } else if (evt->handle == mds->data_uri_h) {
    memfault_platform_get_device_info(&info);
    strncpy(uri, MDS_URI_BASE, sizeof(uri) - 1);
    uri[sizeof(uri) - 1] = '\0';

    const size_t uri_base_length = strlen(MDS_URI_BASE);
    const size_t device_id_length = strlen(info.device_serial);
    const size_t total_length = uri_base_length + device_id_length;

    if (total_length > (sizeof(uri) - 1)) {
      ble_gatts_read_cfm(evt->conn_idx, evt->handle, kMdsAppError_InsufficientLength, 0, NULL);
      return;
    }

    strcpy(uri, MDS_URI_BASE);
    strcpy(&uri[uri_base_length], info.device_serial);
    uri[sizeof(uri) - 1] = '\0';
    value = uri;
    length = total_length;
  } else if (evt->handle == mds->auth_h) {
    value = "Memfault-Project-Key:"MEMFAULT_PROJECT_KEY;
    length = strlen(value);
  } else if (evt->handle == mds->device_id_h) {
    memfault_platform_get_device_info(&info);
    value = info.device_serial;
    length = strlen(value);
  } else {
    ble_gatts_read_cfm(evt->conn_idx, evt->handle, ATT_ERROR_READ_NOT_PERMITTED, 0, NULL);
    return;
  }

  if (length > MDS_MAX_READ_LEN) {
    // response exceeds minimum mtu size in length
    ble_gatts_read_cfm(evt->conn_idx, evt->handle, kMdsAppError_InvalidLength, 0, NULL);
    return;
  }

  if (evt->offset > length) {
    length = 0;
  } else {
    length = length - evt->offset;
    value = ((uint8_t *)value) + evt->offset;
  }

  ble_gatts_read_cfm(evt->conn_idx, evt->handle, ATT_ERROR_OK,
                     length, value);
}

static att_error_t prv_handle_cccd_write(md_service_t *mds, uint16_t conn_idx,
                                         uint16_t offset, uint16_t length,
                                         const uint8_t *value) {
  if (offset != 0) {
    return ATT_ERROR_ATTRIBUTE_NOT_LONG;
  }

  if (length != sizeof(uint16_t)) {
    return kMdsAppError_InvalidLength;
  }

  const uint16_t cccd = get_u16(value);
  const bool subscribe_for_notifs = ((cccd & GATT_CCC_NOTIFICATIONS) != 0);
  if (!mds->subscriber.active) {
    // NB: we expect caller to subscribe for notifications each time they connect
    // so don't persist the state across disconnects _and_ we only allow one
    // active subscription at a time.
    mds->subscriber.active = subscribe_for_notifs;
    mds->subscriber.conn_idx = conn_idx;
  } else if (mds->subscriber.conn_idx == conn_idx) {
    // handle case where client is subscribed (active) and has unsubscribed or re-subscribed for
    // some reason
    mds->subscriber.active = subscribe_for_notifs;
  } else {
    // only one client can be subscribed at any given time
    return kMdsAppError_ClientAlreadySubscribed;
  }

  return ATT_ERROR_OK;
}

static void prv_handle_write_req(ble_service_t *svc,
                             const ble_evt_gatts_write_req_t *evt) {
  if (!mds_access_enabled(evt->conn_idx)) {
    ble_gatts_read_cfm(evt->conn_idx, evt->handle, ATT_ERROR_WRITE_NOT_PERMITTED, 0, NULL);
    return;
  }


  md_service_t *mds = (md_service_t *)svc;
  att_error_t status = ATT_ERROR_ATTRIBUTE_NOT_FOUND;

  if (evt->handle == mds->chunk_cccd_h) {
    status = prv_handle_cccd_write(mds, evt->conn_idx, evt->offset, evt->length,
                                   evt->value);
    if (status == ATT_ERROR_OK) {
      // Empirically, sending a notification immediately will wind up getting flushed
      // before the response for the CCCD write so we defer starting to send chunk
      // notifications so they do not arrive out of order
      xTimerChangePeriod(mds->timer, 10, 0);
      xTimerStart(mds->timer, 0);
    }
  }

  ble_gatts_write_cfm(evt->conn_idx, evt->handle, status);
}

static void prv_cleanup_service(ble_service_t *svc) {
  md_service_t *mds = (md_service_t *)svc;
  if (mds->chunk.buf != NULL) {
    OS_FREE(mds->chunk.buf);
  }
  if (mds->timer != NULL) {
    xTimerDelete(mds->timer, portMAX_DELAY);
  }

  OS_FREE(mds);
  s_mds = NULL;
}

static void prv_handle_event_sent(ble_service_t *svc, const ble_evt_gatts_event_sent_t *evt) {
  prv_try_notify((md_service_t *)svc, evt->conn_idx);
}

void *mds_boot(void) {
  md_service_t *mds = OS_MALLOC(sizeof(*mds));
  memset(mds, 0, sizeof(*mds));

  mds->timer = xTimerCreate("mds_timer", pdMS_TO_TICKS(1000), pdFALSE, /* auto reload */
                             (void *)NULL, prv_mds_timer_callback);

  mds->svc.disconnected_evt = prv_handle_disconnected_evt;
  mds->svc.read_req = prv_handle_read_req;
  mds->svc.write_req = prv_handle_write_req;
  mds->svc.event_sent = prv_handle_event_sent;
  mds->svc.cleanup = prv_cleanup_service;

  const uint16_t num_includes = 0;
  const uint16_t num_characteristics = 6;
  const uint16_t num_descriptors = 1;
  uint16_t num_attr = ble_gatts_get_num_attr(num_includes, num_characteristics, num_descriptors);

  att_uuid_t uuid;
  ble_uuid_from_string("54220000-f6a5-4007-a371-722f4ebd8436", &uuid);
  ble_gatts_add_service(&uuid, GATT_SERVICE_PRIMARY, num_attr);

  ble_uuid_from_string("54220001-f6a5-4007-a371-722f4ebd8436", &uuid);
  ble_gatts_add_characteristic(&uuid, GATT_PROP_READ, ATT_PERM_RW, sizeof(uint8_t),
                               GATTS_FLAG_CHAR_READ_REQ, NULL, &mds->version_h);

  ble_uuid_from_string("54220002-f6a5-4007-a371-722f4ebd8436", &uuid);
  ble_gatts_add_characteristic(&uuid, GATT_PROP_READ, ATT_PERM_RW, sizeof(uint8_t),
                               GATTS_FLAG_CHAR_READ_REQ, NULL, &mds->device_id_h);

  ble_uuid_from_string("54220003-f6a5-4007-a371-722f4ebd8436", &uuid);
  ble_gatts_add_characteristic(&uuid, GATT_PROP_READ, ATT_PERM_RW, sizeof(uint8_t),
                               GATTS_FLAG_CHAR_READ_REQ, NULL, &mds->data_uri_h);

  ble_uuid_from_string("54220004-f6a5-4007-a371-722f4ebd8436", &uuid);
  ble_gatts_add_characteristic(&uuid, GATT_PROP_READ, ATT_PERM_RW, sizeof(uint8_t),
                               GATTS_FLAG_CHAR_READ_REQ, NULL, &mds->auth_h);

  ble_uuid_from_string("54220005-f6a5-4007-a371-722f4ebd8436", &uuid);
  ble_gatts_add_characteristic(&uuid, GATT_PROP_NOTIFY, ATT_PERM_RW, sizeof(uint8_t),
                               GATTS_FLAG_CHAR_READ_REQ, NULL, &mds->chunk_val_h);

  ble_uuid_create16(UUID_GATT_CLIENT_CHAR_CONFIGURATION, &uuid);
  ble_gatts_add_descriptor(&uuid, ATT_PERM_RW, 2, 0, &mds->chunk_cccd_h);

  ble_gatts_register_service(&mds->svc.start_h, &mds->version_h, &mds->device_id_h,
                             &mds->data_uri_h, &mds->auth_h, &mds->chunk_val_h,
                             &mds->chunk_cccd_h, 0);

  mds->svc.end_h = mds->svc.start_h + num_attr;

  ble_service_add(&mds->svc);

  s_mds = mds;
  return &mds->svc;
}

#endif /* defined(dg_configUSE_MEMFAULT) */
#endif /* defined(CONFIG_USE_BLE_SERVICES) */
