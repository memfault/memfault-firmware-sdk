//! @file
//!
//! Copyright (c) Memfault, Inc.
//! See LICENSE for details
//!
//! @brief
//! Instrumentation that wraps a Bluetooth SMP (MCUmgr) DFU image upload in a
//! Memfault "session" report (https://mflt.io/session-metrics).
//!
//! What this tracks:
//!  - A `bt_smp` session, started when the phone begins uploading a firmware
//!    image and ended when the upload finishes, fails, or the link drops. The
//!    session automatically records its own duration.
//!  - `bt_smp_bytes_received`: image payload bytes accepted from the phone.
//!  - `bt_smp_image_size`: total image size the phone declared up front.
//!  - `bt_smp_status`: outcome code (see eBtSmpDfuStatus).
//!
//! How it hooks in (no MCUmgr internals are modified):
//!  - The DFU lifecycle is observed through the MCUmgr notification callback API
//!    (MGMT_EVT_OP_IMG_MGMT_DFU_*).
//!  - Memfault event storage on this port is RAM-backed, so a session report is
//!    lost across the reboot into the new image. To give it a chance to reach
//!    the phone (via the Memfault Diagnostic Service) before we reboot, we hook
//!    the MCUmgr reset command (MGMT_EVT_OP_OS_MGMT_RESET) and block there until
//!    the Memfault packetizer drains (or a timeout elapses). That hook runs on
//!    the dedicated MCUmgr SMP work queue, *not* the system work queue that
//!    streams MDS data, so blocking it lets MDS keep pushing while we wait. Once
//!    we return, MCUmgr performs its normal reboot.
//!
//! Note on "bytes sent": only bytes *received* from the phone are tracked. The
//! SMP responses we transmit are tiny acknowledgements, and the BLE SMP
//! transmit path is a set of static functions with no public instrumentation
//! point, so a TX byte count is not practical to collect here.

#include <zephyr/init.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/atomic.h>

#include "memfault/components.h"
#include "memfault/core/data_packetizer.h"
#include "memfault/metrics/metrics.h"

LOG_MODULE_REGISTER(bt_smp_dfu, LOG_LEVEL_INF);

//! Outcome recorded in the `bt_smp_status` session metric.
typedef enum {
  kBtSmpDfuStatus_Success = 0,       //!< Image accepted for test/confirm.
  kBtSmpDfuStatus_Aborted = -1,      //!< MCUmgr reported the upload stopped.
  kBtSmpDfuStatus_Disconnected = -2, //!< Link dropped while an upload was in flight.
} eBtSmpDfuStatus;

#if defined(CONFIG_MCUMGR_GRP_IMG_STATUS_HOOKS) && \
  defined(CONFIG_MCUMGR_GRP_IMG_UPLOAD_CHECK_HOOK)

  #include <zephyr/bluetooth/conn.h>
  #include <zephyr/mgmt/mcumgr/grp/img_mgmt/img_mgmt.h>
  // <.../mgmt/callbacks.h> transitively provides the MCUmgr event enums, the
  // mgmt_callback API, and struct img_mgmt_upload_check. Include it rather than
  // the per-group <.../grp/*/*_callbacks.h> headers directly: those are not
  // self-contained (they reference macros/groups that callbacks.h defines
  // first), so including them standalone fails to compile.
  #include <zephyr/mgmt/mcumgr/mgmt/callbacks.h>

//! Max time to wait for Memfault data to drain before allowing the reboot.
  #define BT_SMP_DFU_DRAIN_TIMEOUT_MS 8000
//! Grace period after the packetizer empties, to let the final BLE notification
//! make it onto the air before the radio goes down.
  #define BT_SMP_DFU_DRAIN_GRACE_MS 500

//! 1 while a bt_smp session is active. Used to coordinate the (rare) race
//! between the MCUmgr handler context and a BT disconnect callback.
static atomic_t s_dfu_active = ATOMIC_INIT(0);

//! Set when a DFU completes successfully and a reboot into the new image is
//! expected. Consumed by the reset hook so unrelated reboots are never delayed.
static atomic_t s_flush_before_reboot = ATOMIC_INIT(0);

static struct {
  uint32_t bytes_received;
  uint32_t image_size;
} s_dfu;

static void prv_end_session(eBtSmpDfuStatus status);

static void prv_start_session(uint32_t image_size) {
  // A fresh upload (offset 0) while a session is still active means the prior
  // attempt was abandoned without notification. Close it out as aborted before
  // starting clean.
  if (atomic_get(&s_dfu_active)) {
    prv_end_session(kBtSmpDfuStatus_Aborted);
  }

  s_dfu.bytes_received = 0;
  s_dfu.image_size = image_size;

  MEMFAULT_METRICS_SESSION_START(bt_smp);
  atomic_set(&s_dfu_active, 1);

  LOG_INF("bt_smp DFU session started (image size %u bytes)", image_size);
}

static void prv_end_session(eBtSmpDfuStatus status) {
  // Atomically claim the session so a concurrent disconnect + lifecycle event
  // can't end it twice.
  if (!atomic_cas(&s_dfu_active, 1, 0)) {
    return;
  }

  MEMFAULT_METRIC_SESSION_SET_UNSIGNED(bt_smp_bytes_received, bt_smp, s_dfu.bytes_received);
  MEMFAULT_METRIC_SESSION_SET_UNSIGNED(bt_smp_image_size, bt_smp, s_dfu.image_size);
  MEMFAULT_METRIC_SESSION_SET_SIGNED(bt_smp_status, bt_smp, status);

  MEMFAULT_METRICS_SESSION_END(bt_smp);

  LOG_INF("bt_smp DFU session ended: status %d, %u/%u bytes received", (int)status,
          s_dfu.bytes_received, s_dfu.image_size);

  if (status == kBtSmpDfuStatus_Success) {
    // A reboot into the new image is imminent; ask the reset hook to flush the
    // session report to the phone first.
    atomic_set(&s_flush_before_reboot, 1);
  }
}

//! Block until the Memfault packetizer drains (or a timeout elapses).
//!
//! Called from the MCUmgr SMP work queue (the reset command handler). The
//! Memfault Diagnostic Service streams from the *system* work queue, so it keeps
//! pushing chunks to the phone while we sleep here.
static void prv_flush_memfault_data(void) {
  LOG_INF("DFU reset requested; flushing Memfault data to phone before reboot (<= %d ms)",
          BT_SMP_DFU_DRAIN_TIMEOUT_MS);

  const int64_t deadline = k_uptime_get() + BT_SMP_DFU_DRAIN_TIMEOUT_MS;
  while (memfault_packetizer_data_available() && (k_uptime_get() < deadline)) {
    k_sleep(K_MSEC(50));
  }

  const bool drained = !memfault_packetizer_data_available();
  LOG_INF("Memfault drain %s; allowing reboot", drained ? "complete" : "timed out");

  // Let the last notification flush over the air before the radio drops.
  k_sleep(K_MSEC(BT_SMP_DFU_DRAIN_GRACE_MS));
}

static enum mgmt_cb_return prv_img_mgmt_cb(uint32_t event, enum mgmt_cb_return prev_status,
                                           int32_t *rc, uint16_t *group, bool *abort_more,
                                           void *data, size_t data_size) {
  ARG_UNUSED(prev_status);
  ARG_UNUSED(rc);
  ARG_UNUSED(group);
  ARG_UNUSED(abort_more);

  switch (event) {
    case MGMT_EVT_OP_IMG_MGMT_DFU_CHUNK: {
      if ((data == NULL) || (data_size != sizeof(struct img_mgmt_upload_check))) {
        break;
      }
      const struct img_mgmt_upload_check *check = data;
      const struct img_mgmt_upload_req *req = check->req;

      if (req->off == 0) {
        // First (or restarted) chunk. action->size is the validated total image
        // size (req->size is SIZE_MAX when the phone omits it).
        prv_start_session((uint32_t)check->action->size);
      }

      if (atomic_get(&s_dfu_active)) {
        s_dfu.bytes_received += (uint32_t)req->img_data.len;
      }
      break;
    }

    case MGMT_EVT_OP_IMG_MGMT_DFU_PENDING:
    case MGMT_EVT_OP_IMG_MGMT_DFU_CONFIRMED:
      // Image written and marked for test (PENDING) or confirmed: success.
      prv_end_session(kBtSmpDfuStatus_Success);
      break;

    case MGMT_EVT_OP_IMG_MGMT_DFU_STOPPED:
      prv_end_session(kBtSmpDfuStatus_Aborted);
      break;

    default:
      break;
  }

  return MGMT_CB_OK;
}

static struct mgmt_callback s_img_mgmt_cb = {
  .callback = prv_img_mgmt_cb,
  .event_id = MGMT_EVT_OP_IMG_MGMT_DFU_CHUNK | MGMT_EVT_OP_IMG_MGMT_DFU_PENDING |
              MGMT_EVT_OP_IMG_MGMT_DFU_CONFIRMED | MGMT_EVT_OP_IMG_MGMT_DFU_STOPPED,
};

  #if defined(CONFIG_MCUMGR_GRP_OS_RESET_HOOK)
//! Hook the MCUmgr reset command so we can flush the session report to the phone
//! before the device reboots into the freshly uploaded image.
static enum mgmt_cb_return prv_os_mgmt_cb(uint32_t event, enum mgmt_cb_return prev_status,
                                          int32_t *rc, uint16_t *group, bool *abort_more,
                                          void *data, size_t data_size) {
  ARG_UNUSED(prev_status);
  ARG_UNUSED(rc);
  ARG_UNUSED(group);
  ARG_UNUSED(abort_more);
  ARG_UNUSED(data);
  ARG_UNUSED(data_size);

  if ((event == MGMT_EVT_OP_OS_MGMT_RESET) && atomic_cas(&s_flush_before_reboot, 1, 0)) {
    prv_flush_memfault_data();
  }

  // Always allow the reset to proceed.
  return MGMT_CB_OK;
}

static struct mgmt_callback s_os_mgmt_cb = {
  .callback = prv_os_mgmt_cb,
  .event_id = MGMT_EVT_OP_OS_MGMT_RESET,
};
  #endif  // CONFIG_MCUMGR_GRP_OS_RESET_HOOK

//! End an in-flight session if the phone disconnects mid-upload. Zephyr allows
//! multiple BT_CONN_CB_DEFINE registrations, so this composes with the set in
//! main.c.
static void prv_dfu_disconnected(struct bt_conn *conn, uint8_t reason) {
  ARG_UNUSED(conn);
  ARG_UNUSED(reason);

  if (atomic_get(&s_dfu_active)) {
    LOG_WRN("Link dropped mid-DFU; ending bt_smp session as failed");
    prv_end_session(kBtSmpDfuStatus_Disconnected);
  }
}

BT_CONN_CB_DEFINE(bt_smp_dfu_conn_cb) = {
  .disconnected = prv_dfu_disconnected,
};

static int prv_bt_smp_dfu_session_init(void) {
  mgmt_callback_register(&s_img_mgmt_cb);
  #if defined(CONFIG_MCUMGR_GRP_OS_RESET_HOOK)
  mgmt_callback_register(&s_os_mgmt_cb);
  #endif
  LOG_INF("bt_smp DFU session tracking registered");
  return 0;
}

SYS_INIT(prv_bt_smp_dfu_session_init, APPLICATION, CONFIG_APPLICATION_INIT_PRIORITY);

// ---------------------------------------------------------------------------
// Bench-test shell commands. These drive the same code paths a real DFU does,
// so you can validate metric recording and the pre-reboot drain without a phone.
// ---------------------------------------------------------------------------

  #if defined(CONFIG_SHELL)
    #include <stdlib.h>

    #include <zephyr/shell/shell.h>

//! Simulate a complete, successful DFU upload of `image_size` bytes.
static int prv_cmd_sim(const struct shell *sh, size_t argc, char **argv) {
  const uint32_t image_size = (argc > 1) ? (uint32_t)strtoul(argv[1], NULL, 0) : 65536;

  shell_print(sh, "Simulating a successful bt_smp DFU of %u bytes", image_size);

  // Mimic the MCUmgr upload: start on the first chunk, accumulate per chunk.
  prv_start_session(image_size);
  const uint32_t chunk = 244;  // typical SMP-over-BLE payload size
  for (uint32_t off = 0; off < image_size; off += chunk) {
    s_dfu.bytes_received += MIN(chunk, image_size - off);
  }
  prv_end_session(kBtSmpDfuStatus_Success);

  shell_print(sh, "Done. The session report is queued and flush-before-reboot is armed.");
  shell_print(sh, "Inspect it with `mflt metrics_dump` or `mflt export`.");
  return 0;
}

//! End the active session as aborted (starting one first if none is active).
static int prv_cmd_abort(const struct shell *sh, size_t argc, char **argv) {
  ARG_UNUSED(argc);
  ARG_UNUSED(argv);

  if (!atomic_get(&s_dfu_active)) {
    shell_print(sh, "No active session; starting one to simulate a mid-upload abort");
    prv_start_session(65536);
    s_dfu.bytes_received = 1024;
  }
  prv_end_session(kBtSmpDfuStatus_Aborted);
  shell_print(sh, "bt_smp session ended as aborted");
  return 0;
}

//! Exercise the pre-reboot drain logic (logs how long it waits) without
//! actually rebooting.
static int prv_cmd_drain(const struct shell *sh, size_t argc, char **argv) {
  ARG_UNUSED(argc);
  ARG_UNUSED(argv);

  shell_print(sh, "Running the pre-reboot Memfault drain (no reboot will occur)...");
  prv_flush_memfault_data();
  shell_print(sh, "Drain routine returned.");
  return 0;
}

SHELL_STATIC_SUBCMD_SET_CREATE(
  sub_bt_smp,
  SHELL_CMD_ARG(sim, NULL, "Simulate a successful DFU session. Usage: bt_smp sim [image_size_bytes]",
                prv_cmd_sim, 1, 1),
  SHELL_CMD(abort, NULL, "End the active DFU session as aborted (starts one if none active)",
            prv_cmd_abort),
  SHELL_CMD(drain, NULL, "Exercise the pre-reboot Memfault drain logic without rebooting",
            prv_cmd_drain),
  SHELL_SUBCMD_SET_END);
SHELL_CMD_REGISTER(bt_smp, &sub_bt_smp, "Bluetooth SMP DFU session test commands", NULL);
  #endif  // CONFIG_SHELL

#else  // MCUmgr image hooks not enabled

  #warning \
    "bt_smp DFU session tracking is inactive: enable CONFIG_MCUMGR_GRP_IMG_STATUS_HOOKS and " \
    "CONFIG_MCUMGR_GRP_IMG_UPLOAD_CHECK_HOOK (see prj.conf)"

#endif  // CONFIG_MCUMGR_GRP_IMG_STATUS_HOOKS && CONFIG_MCUMGR_GRP_IMG_UPLOAD_CHECK_HOOK
