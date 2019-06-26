#pragma once

#include "esp_err.h"
#include "esp_http_client.h"

/*
 * @brief      Handler function to provide the request body. The handler implementation is expected to
 *             call esp_http_client_write() one or more times to write the request body.
 */
typedef void (*esp_http_request_body_cb)(esp_http_client_handle_t client);

/**
 * @brief      Sets a handler function to provide the request body. This is an alternative to providing a request
 *             body buffer using esp_http_client_set_post_field(). When it is time to write out the request body,
 *             the handler will be called. The handler implementation is expected to call esp_http_client_write() one
 *             or more times to write the request body.
 *
 * @note       The handler is responsible for ensuring that the total size
 *             of the written request body matches the request_body_len that is given when calling
 *             esp_http_client_set_request_body_handler().
 *
 * @param      client  The esp_http_client handle
 * @param      handler  The handler that will write the request body
 * @param      request_body_len  The size of the request body in number of bytes
 *
 * @return
 *  - ESP_OK on successful
 *  - ESP_FAIL on error
 */
esp_err_t esp_http_client_set_request_body_handler(esp_http_client_handle_t client,
                                                   esp_http_request_body_cb handler,
                                                   size_t request_body_len);

/**
 * @brief      Gets the user_data that was previously assigned using esp_http_client_set_user_data() or through the
 *             configuration given to esp_http_client_init().
 *
 * @param      client  The esp_http_client handle
 *
 * @return     The user_data
 */
void *esp_http_client_get_user_data(esp_http_client_handle_t client);

/**
 * @brief      Sets the user_data of the client, overwriting any value that was previously assigned using
 *             esp_http_client_set_user_data() or assigned through the configuration given to esp_http_client_init().
 *
 * @param      client  The esp_http_client handle
 * @param[in]  user_data The new user_data
 */
void esp_http_client_set_user_data(esp_http_client_handle_t client, void *user_data);
