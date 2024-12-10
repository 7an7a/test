#ifndef HTTPS_H
#define HTTPS_H

#include <stdint.h>   // For uint8_t types
#include "esp_tls.h"  // For TLS connection
#include "esp_log.h"  // For logging
#include "cJSON.h"    // For JSON parsing

// Constants and macro definitions
#define WEB_SERVER "apifoxmock.com"
#define WEB_PORT "443"

#ifndef WEB_URL
#define WEB_URL "https://apifoxmock.com/m1/5440136-5115246-default/light/status"
#endif


#define SERVER_URL_MAX_SZ 256

// Buffer sizes
#define RESPONSE_BUFFER_SIZE 4096
#define EXTRACTED_STATUS_SIZE 64

// External variables
extern char extracted_status[EXTRACTED_STATUS_SIZE]; // 用于存储提取的返回值
extern char response_buffer[RESPONSE_BUFFER_SIZE];   // 用于存储 HTTP 响应数据

// Function declarations

/**
 * @brief Initializes and starts the HTTPS request using the provided configuration.
 *
 * @param cfg The TLS configuration for the connection.
 * @param WEB_SERVER_URL The URL of the web server to connect to.
 * @param REQUEST The HTTP request string to be sent.
 */
//void https_get_request(esp_tls_cfg_t cfg, const char *WEB_SERVER_URL, const char *REQUEST);
 void https_get_request(const char *uid);

/**
 * @brief Starts an HTTPS GET request using certificate bundle.
 */
void https_get_request_using_crt_bundle(void);

/**
 * @brief Starts an HTTPS GET request using certificate buffer.
 */
void https_get_request_using_cacert_buf(void);

/**
 * @brief Starts an HTTPS GET request using specified ciphersuites.
 */
void https_get_request_using_specified_ciphersuites(void);

/**
 * @brief Starts an HTTPS GET request using global CA store.
 */
void https_get_request_using_global_ca_store(void);

/**
 * @brief Starts the task that performs HTTPS requests.
 *
 * This task calls the various HTTPS request functions, such as using a certificate buffer
 * or global CA store, and handles the server response.
 */
void https_request_task(void *pvparameters);

/**
 * @brief Main function to initialize everything and start HTTPS requests.
 *
 * This function initializes NVS, Wi-Fi/Ethernet, and sets up the HTTPS request task.
 */
void https_main(void);

#endif // HTTPS_H
