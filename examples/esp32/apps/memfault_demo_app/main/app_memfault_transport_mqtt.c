//! @file
//!
//! Copyright (c) Memfault, Inc.
//! See License.txt for details

#include <stddef.h>
#include <stdint.h>

#include "app_memfault_transport.h"
#include "esp_log.h"
#include "memfault/components.h"
#include "mqtt_client.h"

// TODO: Fill in with device's Memfault project
#define MEMFAULT_PROJECT "my_project"

static const char *TAG = "app_memfault_transport_mqtt";

// TODO: Fill in with broker connection configuration
static esp_mqtt_client_config_t s_mqtt_config = {
  .broker.address.uri = "mqtt://192.168.50.88",
  .credentials.username = "test",
  .credentials.authentication.password = "test1234",
  .session.protocol_ver = MQTT_PROTOCOL_V_5,
};
static SemaphoreHandle_t s_mqtt_connected = NULL;

static esp_mqtt_client_handle_t s_mqtt_client = NULL;
static esp_mqtt5_publish_property_config_t s_publish_property = {
  .topic_alias = 1,
};
static char s_topic_string[128] = {0};

static uint8_t s_chunk_data[1024] = {0};

static void mqtt_event_handler(MEMFAULT_UNUSED void *handler_args,
                               MEMFAULT_UNUSED esp_event_base_t base,
                               MEMFAULT_UNUSED int32_t event_id, void *event_data) {
  esp_mqtt_event_handle_t event = (esp_mqtt_event_handle_t)event_data;

  switch (event->event_id) {
    case MQTT_EVENT_CONNECTED:
      ESP_LOGI(TAG, "Connected to MQTT broker");
      xSemaphoreGive(s_mqtt_connected);
      break;
    default:
      ESP_LOGE(TAG, "Unknown MQTT event received[%d]", event->event_id);
      break;
  }
}

static void prv_close_client(void) {
  int rv = esp_mqtt_client_disconnect(s_mqtt_client);
  if (rv) {
    ESP_LOGW(TAG, "Failed to disconnect[%d]", rv);
  }

  rv = esp_mqtt_client_destroy(s_mqtt_client);
  if (rv) {
    ESP_LOGW(TAG, "Failed to destroy client[%d]", rv);
  }
  MEMFAULT_METRIC_TIMER_STOP(mqtt_conn_uptime);

  s_mqtt_client = NULL;
}

static int prv_create_client(void) {
  if (s_mqtt_client) {
    return 0;
  }

  s_mqtt_client = esp_mqtt_client_init(&s_mqtt_config);
  if (s_mqtt_client == NULL) {
    ESP_LOGE(TAG, "MQTT client failed to initialize");
    return -1;
  }

  int rv =
    esp_mqtt_client_register_event(s_mqtt_client, MQTT_EVENT_CONNECTED, mqtt_event_handler, NULL);
  if (rv) {
    ESP_LOGE(TAG, "MQTT event handler registration failed[%d]", rv);
  }

  rv = esp_mqtt_client_start(s_mqtt_client);
  if (rv) {
    ESP_LOGE(TAG, "MQTT client start failed[%d]", rv);
    return -1;
  }

  // Wait for successful connection
  rv = xSemaphoreTake(s_mqtt_connected, (1000 * 10) / portTICK_PERIOD_MS);
  if (rv != pdTRUE) {
    ESP_LOGE(TAG, "MQTT client failed to connect[%d]", rv);
    MEMFAULT_METRIC_TIMER_START(mqtt_conn_downtime);
    prv_close_client();
    return -1;
  }

  // Update connection metrics when connected
  MEMFAULT_METRIC_TIMER_STOP(mqtt_conn_downtime);
  MEMFAULT_METRIC_TIMER_START(mqtt_conn_uptime);

  // Set topic alias before publishing
  rv = esp_mqtt5_client_set_publish_property(s_mqtt_client, &s_publish_property);
  if (rv != 0) {
    ESP_LOGW(TAG, "MQTT client could not set publish property [%d]", rv);
  }
  return 0;
}

static const char *prv_get_device_serial(void) {
  sMemfaultDeviceInfo info = {0};
  memfault_platform_get_device_info(&info);
  return info.device_serial;
}

void prv_build_topic_string(void) {
  // String already built
  if (strlen(s_topic_string) > 0) {
    return;
  }

  const char *device_serial = prv_get_device_serial();
  snprintf(s_topic_string, MEMFAULT_ARRAY_SIZE(s_topic_string),
           "memfault/" MEMFAULT_PROJECT "/%s/chunks", device_serial);
}

void app_memfault_transport_init(void) {
#if MEMFAULT_FREERTOS_PORT_USE_STATIC_ALLOCATION != 0
  static StaticSemaphore_t s_mqtt_connected;
  s_mqtt_connected = xSemaphoreCreateBinaryStatic(&s_mqtt_connected);
#else
  s_mqtt_connected = xSemaphoreCreateBinary();
#endif
}

int app_memfault_transport_send_chunks(void) {
  int rv = prv_create_client();

  if (rv) {
    return rv;
  }

  prv_build_topic_string();

  ESP_LOGD(TAG, "Checking for packetizer data");
  while (memfault_packetizer_data_available()) {
    size_t chunk_size = MEMFAULT_ARRAY_SIZE(s_chunk_data);
    bool chunk_filled = memfault_packetizer_get_chunk(s_chunk_data, &chunk_size);

    if (!chunk_filled) {
      ESP_LOGW(TAG, "No chunk data produced");
      break;
    }

    rv = esp_mqtt_client_publish(s_mqtt_client, s_topic_string, (char *)s_chunk_data, chunk_size, 1,
                                 0);
    if (rv < 0) {
      ESP_LOGE(TAG, "MQTT failed to publish[%d]", rv);
      memfault_packetizer_abort();
      break;
    }

    MEMFAULT_METRIC_ADD(mqtt_publish_bytes, chunk_size);
    MEMFAULT_METRIC_ADD(mqtt_publish_count, 1);
    ESP_LOGD(TAG, "chunk[%d], len[%zu] published to %s", rv, chunk_size, s_topic_string);
  }

  prv_close_client();
  return rv;
}
