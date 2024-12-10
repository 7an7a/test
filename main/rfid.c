#include <stdio.h>
#include <string.h>
#include <esp_log.h>
#include <cJSON.h>
#include "rc522.h"
#include "ws2812.h"
#include "https_request.h"

#define TAG "RFID"
void ws2812_write_data(const uint8_t *data, size_t length);
bool rc522_check_card(void);
bool rc522_read_card_uid(uint8_t *uid);
void https_get_request_with_uid(const char *url);


// 存储扫描到的 UID
char scanned_uid[16] = {0};

char extracted_status[128]; // 假设状态是字符串，长度根据实际情况调整


// 控制 WS2812 的颜色
void ws2812_set_color(uint8_t red, uint8_t green, uint8_t blue) {
    uint8_t color_data[3] = {green, red, blue}; // WS2812 格式为 GRB
    ws2812_write_data(color_data, sizeof(color_data)); // 假设 ws2812_write_data 已实现
    ESP_LOGI(TAG, "Set WS2812 color: R=%d, G=%d, B=%d", red, green, blue);
}

// RFID 任务，扫描卡片并处理
void rfid_task(void *pvParameters) {
    while (1) {
        if (rc522_check_card()) {
            rc522_read_card_uid(scanned_uid);  // 假设此函数已实现
            ESP_LOGI(TAG, "Scanned UID: %s", scanned_uid);

            // 构建包含 UID 的 URL
            char url_with_uid[256];
            snprintf(url_with_uid, sizeof(url_with_uid), 
                     "https://apifoxmock.com/m1/5440136-5115246-default/light/status?uid=%s", 
                     scanned_uid);

            // 发送 HTTPS 请求
            https_get_request_with_uid(url_with_uid);
        }
        vTaskDelay(100 / portTICK_PERIOD_MS); // 循环间隔
    }
}

// HTTPS 请求函数
void https_get_request_with_uid(const char *url) {
    esp_tls_cfg_t cfg = {
        .cacert_buf = (const unsigned char *)server_root_cert_pem_start,
        .cacert_bytes = server_root_cert_pem_end - server_root_cert_pem_start,
    };

    char response[512];
    if (https_get_request(url, &cfg, response, sizeof(response)) == ESP_OK) {
        ESP_LOGI(TAG, "HTTPS Response: %s", response);

        // 解析 JSON 并提取 status
        cJSON *json = cJSON_Parse(response);
        if (json) {
            cJSON *status = cJSON_GetObjectItemCaseSensitive(json, "status");
            if (cJSON_IsString(status) && status->valuestring != NULL) {
                char extracted_status[32];
                snprintf(extracted_status, sizeof(extracted_status), "%s", status->valuestring);
                ESP_LOGI(TAG, "Extracted status: %s", extracted_status);

                // 控制灯光
                if (strcmp(extracted_status, "correct") == 0) {
                    ws2812_set_color(0, 255, 0); // 绿色
                } else if (strcmp(extracted_status, "incorrect") == 0) {
                    ws2812_set_color(255, 0, 0); // 红色
                } else if (strcmp(extracted_status, "invalid Params") == 0) {
                    ws2812_set_color(0, 0, 255); // 蓝色
                } else {
                    ESP_LOGW(TAG, "Unhandled status value");
                }
            } else {
                ESP_LOGE(TAG, "Status field is missing or invalid");
            }
            cJSON_Delete(json);
        } else {
            ESP_LOGE(TAG, "Failed to parse JSON response");
        }
    } else {
        ESP_LOGE(TAG, "HTTPS GET request failed");
    }
}
