#include <string.h>
#include "https_request.h"
#include "esp_tls.h"
#include "cJSON.h"
#include "esp_log.h"
#include "esp_crt_bundle.h"



#define TAG "HTTPS_REQUEST"
#define WEB_SERVER_URL "https://apifoxmock.com/m1/5440136-5115246-default/light/status?uid="
#define RESPONSE_BUFFER_SIZE 2048
#define STATUS_BUFFER_SIZE 64

extern char response_buffer[RESPONSE_BUFFER_SIZE];
char extracted_status[STATUS_BUFFER_SIZE] = {0};



void https_get_request_with_uid(const char *url) {
    esp_tls_cfg_t cfg = {
        .crt_bundle_attach = esp_crt_bundle_attach
    };
    esp_tls_t *tls = esp_tls_init();
    if (!tls) {
        ESP_LOGE(TAG, "Failed to initialize TLS");
        return;
    }

    if (esp_tls_conn_http_new_sync(url, &cfg, tls) != 1) {
        ESP_LOGE(TAG, "Failed to connect to server");
        esp_tls_conn_destroy(tls);
        return;
    }

    const char *request = "GET %s HTTP/1.1\r\n"
                          "Host: apifoxmock.com\r\n"
                          "User-Agent: esp-idf/1.0 esp32\r\n"
                          "\r\n";
    char request_buffer[512];
    snprintf(request_buffer, sizeof(request_buffer), request, url);

    if (esp_tls_conn_write(tls, request_buffer, strlen(request_buffer)) <= 0) {
        ESP_LOGE(TAG, "Failed to send request");
        esp_tls_conn_destroy(tls);
        return;
    }

    int ret = esp_tls_conn_read(tls, response_buffer, RESPONSE_BUFFER_SIZE - 1);
    if (ret > 0) {
        response_buffer[ret] = '\0';
        ESP_LOGI(TAG, "Response: %s", response_buffer);

        cJSON *json = cJSON_Parse(response_buffer);
        if (json) {
            cJSON *status = cJSON_GetObjectItem(json, "status");
            if (cJSON_IsString(status)) {
                strncpy(extracted_status, status->valuestring, STATUS_BUFFER_SIZE);
                ESP_LOGI(TAG, "Extracted status: %s", extracted_status);
            }
            cJSON_Delete(json);
        } else {
            ESP_LOGE(TAG, "Failed to parse JSON");
        }
    } else {
        ESP_LOGE(TAG, "Failed to read response");
    }

    esp_tls_conn_destroy(tls);
}
