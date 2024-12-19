#include <string.h>
#include "esp_tls.h"
#include "esp_crt_bundle.h"
#include <stdlib.h>
#include <inttypes.h>
#include <time.h>
#include <sys/time.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_system.h"
#include "esp_timer.h"
#include "nvs_flash.h"
#include "nvs.h"
#include "protocol_examples_common.h"
#include "esp_sntp.h"
#include "esp_netif.h"
#include "lwip/err.h"
#include "lwip/sockets.h"
#include "lwip/sys.h"
#include "lwip/netdb.h"
#include "lwip/dns.h"
#include "sdkconfig.h"
#include "cJSON.h"
#include "https.h"
#include "rc522.h"
#include "rc522_spi.h"
#include "rc522_picc.h"
#include "driver/gpio.h"
#include "led_control.h"  // 引入 LED 控制模块
#include "led_strip.h"
#include "mbedtls/ssl.h"
#include "mbedtls/x509.h"
#include <stdio.h>
#include "lwip/inet.h"
#include "rc522_pcd_internal.h"
#include "rc522_picc_internal.h"
#include "rc522_helpers_internal.h"
#include "rc522_types_internal.h"
#include "rc522_internal.h"
#include <esp_check.h>
//加入ota
#include "esp_ota_ops.h"
#include "esp_http_client.h"
#include "esp_https_ota.h"
//  openssl s_client -connect apifoxmock.com:443


#define RAINBOW_LED_DELAY  10   // 彩虹灯延迟
#define YELLOW_LED_DELAY   1000 // 黄灯延迟时间（2秒）
#define RAINBOW_CHANGE_INTERVAL 50  // 彩虹灯颜色变化的间隔时间（单位：毫秒）

#define CURRENT_FIRMWARE_VERSION "1.0.0"

//添加版本号管理
#define FIRMWARE_VERSION "1.0.0"
// 用于控制彩虹灯显示的标志
static bool rainbow_active = true;

static const char *OTA_TAG = "RC522_OTA";  // 包含两个模块的信息
extern led_strip_handle_t led_strip;
extern led_strip_handle_t led_strip_2;

//ota配置
// 定义固件下载地址
#define FIRMWARE_URL "https://raw.githubusercontent.com/7an7a/test/main/firmware.bin"


// 引脚配置
#define RC522_SPI_BUS_GPIO_MISO    (11)
#define RC522_SPI_BUS_GPIO_MOSI    (12)
#define RC522_SPI_BUS_GPIO_SCLK    (14)
#define RC522_SPI_SCANNER_GPIO_SDA (13)
#define RC522_SPI_SCANNER_GPIO_RST (-1) // soft-reset

#define POST_DATA_SIZE  256 // 可以调整为更大的值
void set_led_rainbow(led_strip_handle_t led_strip, uint16_t hue_offset);
void handle_status(const char *status);
void set_white_led_strip(void);

// 手动定义颜色
uint8_t red[] = {200, 20, 147};
uint8_t orange[] = {128, 0, 128};
uint8_t yellow[] = {75, 0, 130};
uint8_t green[] = {0, 255, 255};
uint8_t cyan[] = {0, 191, 255};
uint8_t blue[] = {0, 0, 139};
uint8_t purple[] = {255, 140, 0};
// 颜色数组，用于循环渐变
uint8_t* colors[] = {red, orange, yellow, green, cyan, blue, purple};

// HTTP 配置
#define WEB_SERVER "apifoxmock.com"
#define WEB_PORT "443"
//#define WEB_URL "https://apifoxmock.com/m1/5440136-5115246-default/light/status"
#ifndef WEB_URL
#define WEB_URL "https://apifoxmock.com/m1/5440136-5115246-default/light/status?uid=%s"
#endif
#define WEB_POST_URL "https://apifoxmock.com/m1/5440136-5115246-default/light/status?uid=%s"

// 全局变量
static const char *TAG = "RC522_Main";
char extracted_status[64] = {0}; // 用于存储提取的返回值
char response_buffer[4096] = {0}; // 用于存储 HTTP 响应数据

// HTTP 请求模板
const char HTTP_GET_REQUEST[] = "GET " WEB_URL " HTTP/1.1\r\n"
                                "Host: " WEB_SERVER "\r\n"
                                "User-Agent: esp-idf/1.0 esp32\r\n"
                                "\r\n";

const char HTTP_POST_TEMPLATE[] = "POST " WEB_POST_URL " HTTP/1.1\r\n"
                                  "Host: " WEB_SERVER "\r\n"
                                  "Content-Type: application/json\r\n"
                                  "Content-Length: %d\r\n"
                                  "\r\n"
                                  "%s";

// 证书文件的起止位置
extern const uint8_t server_root_cert_pem_start[] asm("_binary_server_root_cert_pem_start");
extern const uint8_t server_root_cert_pem_end[] asm("_binary_server_root_cert_pem_end");


// RC522 驱动和扫描器句柄
static rc522_spi_config_t driver_config = {
    .host_id = SPI3_HOST,
    .bus_config = &(spi_bus_config_t){
        .miso_io_num = RC522_SPI_BUS_GPIO_MISO,
        .mosi_io_num = RC522_SPI_BUS_GPIO_MOSI,
        .sclk_io_num = RC522_SPI_BUS_GPIO_SCLK,
    },
    .dev_config = {
        .spics_io_num = RC522_SPI_SCANNER_GPIO_SDA,
    },
    .rst_io_num = RC522_SPI_SCANNER_GPIO_RST,
};


static rc522_driver_handle_t driver;
static rc522_handle_t scanner;
//nslookup pool.ntp.org
//Name:    pool.ntp.org
//Addresses:  123.123.123.123


void initialize_sntp(void)
{
    // 使用新的设置操作模式的 API
    esp_sntp_setoperatingmode(SNTP_OPMODE_POLL);  // 设置工作模式为轮询

    // 使用新的初始化函数
    esp_sntp_init();  // 初始化 SNTP
    
    // 设置 NTP 服务器
    ip_addr_t ntp_ip;
    ipaddr_aton("162.159.200.1", &ntp_ip);  // 将 NTP 服务器的 IP 地址转换为 ip_addr_t 类型

    esp_sntp_setserver(0, &ntp_ip); // 设置 NTP 服务器
}


// 去掉字符串中的空格
void remove_spaces(char *str) {
    char *read = str;
    char *write = str;
    while (*read) {
        if (*read != ' ') {
            *write++ = *read;
        }
        read++;
    }
    *write = '\0'; // 添加字符串终止符
}


// HTTPS 请求发送 UID
void https_get_request(const char *uid) {
    // 去掉UID中的空格
    char cleaned_uid[128];
    strcpy(cleaned_uid, uid);
    remove_spaces(cleaned_uid);  // 去掉空格
    ESP_LOGI(TAG, "Cleaned UID: %s", cleaned_uid);
    
    // 创建URL并插入处理后的UID
    char url[512];
    snprintf(url, sizeof(url), "https://apifoxmock.com/m1/5440136-5115246-default/light/status?uid=%s", cleaned_uid);
    ESP_LOGI(TAG, "Request URL: %s", url);

    
    // 增加post_data数组的大小
    char post_data[POST_DATA_SIZE];

    // 声明 len 变量并初始化
    int len = 0;
    // 使用 snprintf 来构建 JSON 请求数据，并确保不会超出 post_data 的大小
    len = snprintf(post_data, sizeof(post_data), "{\"uid\": \"%s\"}", cleaned_uid);
    if (len >= sizeof(post_data)) {
        ESP_LOGE(TAG, "POST data is too large and was truncated");
        return;  // 如果数据太大，停止处理
    }

    // 创建 TLS 配置
    esp_tls_t *tls = esp_tls_init();
    esp_tls_cfg_t cfg = {
        .cacert_buf = server_root_cert_pem_start,  // 根证书
        .cacert_bytes = server_root_cert_pem_end - server_root_cert_pem_start,
        // 如果需要客户端证书和私钥，解开以下注释并提供相应数据
        // .client_cert_pem = client_cert_pem_start,
        //.client_key_pem = client_key_pem_start,
        .timeout_ms = 5000,   // 设置接收超时时间为 5000 毫秒（5秒
    };

    ESP_LOGI(TAG, "Trying to create TLS connection to: %s", url);

    // 创建 TLS 连接
    int ret = esp_tls_conn_http_new_sync(url, &cfg, tls);  // 使用新的 API 来建立连接
    if (ret != 1) {
        ESP_LOGE(TAG, "Failed to create TLS connection. Error code: %d", ret);
        ESP_LOGE(TAG, "TLS error: %d", ret);  // 记录详细的错误代码
        ESP_LOGI(TAG, "Request URL: %s", url);  // 打印请求 URL


        return;
    }
    ESP_LOGI(TAG, "TLS connection created successfully");
    // 清空响应缓冲区
    memset(response_buffer, 0, RESPONSE_BUFFER_SIZE);

    // 发送 POST 请求
    //curl "https://apifoxmock.com/m1/5440136-5115246-default/light/status?uid=D5F9FB11"
    //{"status":"incorrect"}%  
                       
    // 创建请求头
    char http_request[1024];
    len = snprintf(http_request, sizeof(http_request), 
    "GET %s HTTP/1.1\r\n"
    "Host: apifoxmock.com\r\n"
    "User-Agent: esp-idf/1.0 esp32\r\n"
    "Connection: close\r\n\r\n", url);

    ESP_LOGI(TAG, "Request sent successfully");
    ret = esp_tls_conn_write(tls, http_request, len);
    if (ret < 0) {
        ESP_LOGE(TAG, "Failed to send request: %d", ret);
        esp_tls_conn_destroy(tls);
        return;
    }else{
        ESP_LOGI(TAG, "Successfully sent GET request: %s", HTTP_GET_REQUEST);
    }

    ESP_LOGI(TAG, "Request sent successfully");

    //读取响应
    char buf[512];
    // memset(response_buffer, 0, sizeof(response_buffer));
    size_t response_len = 0;
    while (true) {
        len = esp_tls_conn_read(tls, buf, sizeof(buf) - 1);
        if (len > 0) {
            buf[len] = '\0';
            ESP_LOGI(TAG, "Received data: %s", buf);  // 打印接收到的数据
            if (response_len + len < sizeof(response_buffer)) {
                strncat(response_buffer, buf, len);  // 拼接数据
                response_len += len;
            } else {
                ESP_LOGE(TAG, "Response buffer overflow");
                break;
            }
        } else if (len == 0) {
            ESP_LOGI(TAG, "Connection closed by the server");
            break;  // 连接关闭时退出循环
        } else if (len < 0) {
            if (len == MBEDTLS_ERR_SSL_TIMEOUT) {
                ESP_LOGE(TAG, "Read timeout occurred");
            } else if (len == MBEDTLS_ERR_SSL_PEER_CLOSE_NOTIFY) {
                ESP_LOGI(TAG, "Server closed the connection gracefully");
            } else {
                ESP_LOGE(TAG, "Error while reading response: %d", len);
            }
            break;  // 读取错误时退出
        }
    }

    ESP_LOGI(TAG, "Full response: %s", response_buffer);

    // 确保读取到了有效的响应数据
    if (response_buffer[0] == '\0') {
        ESP_LOGE(TAG, "No data received in response buffer.");
        esp_tls_conn_destroy(tls);
        return;
    }


// 打印完整的响应数据
ESP_LOGI(TAG, "Full response: %s", response_buffer);

void handle_status(const char *status) {
     rainbow_active = false;  // 暂停彩虹灯

    if (strcmp(status, "correct") == 0) {
        set_led_color(led_strip, 0, 255, 0);  // 正确时显示绿色，传入正确的 strip 参数
        
         vTaskDelay(5000 / portTICK_PERIOD_MS);
         rainbow_active = true;
    } else if (strcmp(status, "incorrect") == 0) {
        set_led_color(led_strip, 255, 0, 0);  // 错误时显示红色，传入正确的 strip 参数
         vTaskDelay(5000 / portTICK_PERIOD_MS);
         rainbow_active = true;
   } else {
        rainbow_active = true;  // 暂停彩虹灯
        //set_led_rainbow();  // 恢复彩虹灯效果
        return;
   }
    vTaskDelay(500 / portTICK_PERIOD_MS);  // 等待1秒，确保状态处理完成

}
/// 提取响应体
char *body = strstr(response_buffer, "\r\n\r\n");
if (body) {
    body += 4;  // 跳过 HTTP 头部的空行（\r\n\r\n）

    // 输出提取的 HTTP 响应体
    ESP_LOGI(TAG, "HTTP body:\n%s", body);

    // 解析 JSON 数据
    cJSON *json = cJSON_Parse(body);
    if (json) {
        // 获取 "status" 字段的值
        cJSON *status = cJSON_GetObjectItemCaseSensitive(json, "status");
        if (cJSON_IsString(status) && status->valuestring) {
            // 将解析的状态存储到 extracted_status 中
            snprintf(extracted_status, sizeof(extracted_status), "%s", status->valuestring);
            ESP_LOGI(TAG, "Extracted status: %s", extracted_status);

            // 处理状态（可以是进一步的操作）
            handle_status(extracted_status);
        } else {
            ESP_LOGE(TAG, "Invalid status field");
        }
        cJSON_Delete(json);  // 解析完成后释放 JSON 对象
    } else {
        ESP_LOGE(TAG, "Failed to parse JSON");
    }
} else {
    ESP_LOGE(TAG, "Failed to extract HTTP body");
}
    //关闭 TLS 连接
    esp_tls_conn_destroy(tls);
}

// 卡片状态变化事件回调
static void on_picc_state_changed(void *arg, esp_event_base_t base, int32_t event_id, void *data) {
    rc522_picc_state_changed_event_t *event = (rc522_picc_state_changed_event_t *)data;
    rc522_picc_t *picc = event->picc;

    ESP_LOGI(TAG, "Card state changed: %d", picc->state);

    if (picc->state == RC522_PICC_STATE_ACTIVE) {
        // 卡片检测成功，提取 UID
        ESP_LOGI(TAG, "Card detected!");
        rainbow_active = false;  // 暂停彩虹灯
        //vTaskDelay(100 / portTICK_PERIOD_MS);  // 确保初始化完成后再进行操作
       set_led_color(led_strip, 255, 255, 0);  // 黄灯，传入正确的白色灯条 strip 参数
        vTaskDelay(YELLOW_LED_DELAY / portTICK_PERIOD_MS);

        // 获取 UID 并转换为字符串格式
        char uid_str[RC522_PICC_UID_STR_BUFFER_SIZE_MAX] = {0};
        esp_err_t err = rc522_picc_uid_to_str(&picc->uid, uid_str, sizeof(uid_str));
        if (err != ESP_OK) {
            ESP_LOGE(TAG, "Failed to convert UID to string");
            return;
        }

        ESP_LOGI(TAG, "UID: %s", uid_str);

        // 调用 HTTPS 请求发送 UID
        https_get_request(uid_str);
        ESP_LOGI(TAG, "UID sent to server");

    } else if (picc->state == RC522_PICC_STATE_IDLE && event->old_state >= RC522_PICC_STATE_ACTIVE) {
        ESP_LOGI(TAG, "Card removed");
        rainbow_active = true; 
        
    }
}

// RGB color structure
typedef struct {
    uint8_t r;
    uint8_t g;
    uint8_t b;
} rgb_color_t;

// HSV to RGB conversion function
rgb_color_t hsv_to_rgb(uint16_t h, uint8_t s, uint8_t v) {
    uint8_t r, g, b;
    uint16_t region, remainder, p, q, t;

    if (s == 0) {
        r = g = b = v;  // If saturation is 0, it's a gray color
        return (rgb_color_t){r, g, b};
    }

    region = h / 60;          // Determine the region of the hue (360° -> 6 sections)
    remainder = h - (region * 60);  // Find remainder of the hue within the region
    p = (v * (255 - s)) / 255;  // Calculate the RGB components
    q = (v * (255 - (s * remainder) / 60)) / 255;
    t = (v * (255 - (s * (60 - remainder)) / 60)) / 255;

    switch (region) {
        case 0:
            r = v; g = t; b = p; break;
        case 1:
            r = q; g = v; b = p; break;
        case 2:
            r = p; g = v; b = t; break;
        case 3:
            r = p; g = q; b = v; break;
        case 4:
            r = t; g = p; b = v; break;
        case 5:
            r = v; g = p; b = q; break;
        default:
            r = g = b = 0;  // Error case
    }

    return (rgb_color_t){r, g, b};
}


// 设置彩虹灯效果
void set_led_rainbow(led_strip_handle_t led_strip, uint16_t hue_offset) {
    // Iterate through each LED and set its color to create a rainbow effect
    for (int i = 0; i < LED_COUNT; i++) {
        uint16_t hue = (i * 360 / LED_COUNT + hue_offset) % 360; // Apply offset to make the effect flow
        rgb_color_t color = hsv_to_rgb(hue, 255, 255); // Convert HSV to RGB
        ESP_ERROR_CHECK(led_strip_set_pixel(led_strip, i, color.r, color.g, color.b)); // Set LED color
    }

    // Refresh the strip to send the data
    ESP_ERROR_CHECK(led_strip_refresh(led_strip));
    
}



//OTA
void check_and_perform_ota() {
    ESP_LOGI(OTA_TAG, "开始检查固件更新");


    // 添加根证书声明
    extern const uint8_t server_root_cert_pem_start[] asm("_binary_server_root_cert_pem_start");
    extern const uint8_t server_root_cert_pem_end[] asm("_binary_server_root_cert_pem_end");

    // 获取当前固件版本
    esp_app_desc_t running_app_info;
    esp_err_t err = esp_ota_get_partition_description(esp_ota_get_running_partition(), &running_app_info);
    
    if (err != ESP_OK) {
        ESP_LOGE(OTA_TAG, "获取当前固件版本失败");
        return;
    }

    ESP_LOGI(OTA_TAG, "当前固件版本: %s", running_app_info.version);

    esp_http_client_config_t config = {
        .url = FIRMWARE_URL,
        .cert_pem = (const char *)server_root_cert_pem_start,
        .timeout_ms = 60000,
    };
    
    esp_https_ota_config_t ota_config = {
        .http_config = &config,
    };

    esp_err_t ret = esp_https_ota(&ota_config);
    if (ret == ESP_OK) {
        ESP_LOGI(OTA_TAG, "OTA更新成功");
        esp_restart();
    } else {
        ESP_LOGE(OTA_TAG, "OTA更新失败,错误码: 0x%x", ret);
        
        // 使用通用错误处理
        switch(ret) {
            case ESP_FAIL:
                ESP_LOGE(OTA_TAG, "OTA通用失败");
                break;
            default:
                ESP_LOGE(OTA_TAG, "未知错误");
                break;
        }
    }
}

// OTA检查任务
void ota_check_task(void *pvParameters) {
    while(1) {
        vTaskDelay(pdMS_TO_TICKS(3600));  // 每小时检查一次
        check_and_perform_ota();
    }
}



// 主函数
void app_main(void) {


    ESP_ERROR_CHECK(nvs_flash_init());
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());

    // 初始化 LED 控制
    ESP_LOGI(TAG, "Initializing LED...");
    led_init();
    ESP_LOGI(TAG, "LED initialized successfully");

    ESP_LOGI(TAG, "Setting white color...");
    set_white_led_strip();
    ESP_LOGI(TAG, " WHIIT LED initialized successfully");
    

    // 初始化网络连接
    ESP_LOGI(TAG, "Initializing Wi-Fi...");
    ESP_ERROR_CHECK(example_connect());
    ESP_LOGI(TAG, "Wi-Fi connected successfully");

    // 创建 OTA 检查任务
    xTaskCreate(ota_check_task, "OTA_CHECK", 4096, NULL, 5, NULL);
    check_and_perform_ota();

    
    // 初始化 SNTP
    vTaskDelay(pdMS_TO_TICKS(1000)); 
    initialize_sntp();

    //获取栈的高水位标记
    uint32_t highWaterMark = uxTaskGetStackHighWaterMark(NULL);

    // 输出栈高水位标记
   printf("Stack high water mark: %lu\n", highWaterMark);

    // 初始化RC522驱动
    ESP_LOGI(TAG, "Initializing RC522...");
    esp_err_t err = rc522_spi_create(&driver_config, &driver);  // 使用统一的 err 变量
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to create SPI driver: %s", esp_err_to_name(err));
        return;
    }
    ESP_LOGI(TAG, "RC522 SPI driver created successfully");

    rc522_driver_install(driver);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to install RC522 driver: %s", esp_err_to_name(err));
        return;
    }
    ESP_LOGI(TAG, "RC522 driver installed");

    rc522_config_t scanner_config = {
        .driver = driver,
    };
    vTaskDelay(100 / portTICK_PERIOD_MS);  // 确保 SPI 完全初始化

    // 创建RC522扫描器
    err = rc522_create(&scanner_config, &scanner);  // 继续使用同一个 err 变量
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to create RC522 scanner: %s", esp_err_to_name(err));
        return;  // 如果失败，退出函数
    }
    ESP_LOGI(TAG, "RC522 scanner created successfully");

    // 注册事件
    err = rc522_register_events(scanner, RC522_EVENT_PICC_STATE_CHANGED, on_picc_state_changed, NULL);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to register RC522 events: %s", esp_err_to_name(err));
        return;  // 如果失败，退出函数
    }
    ESP_LOGI(TAG, "RC522 event handler registered");

  

    // 启动扫描
    rc522_start(scanner);
    ESP_LOGI(TAG, "RC522 scanner started");
    ESP_LOGI(TAG, "RC522 scanner initialized and started!");
    

    // 初始启动彩虹灯
    vTaskDelay(100 / portTICK_PERIOD_MS);  // 确保初始化完成后再进行操作
    uint16_t hue_offset = 0; // The hue offset to shift the colors
    set_led_rainbow(led_strip, hue_offset);
    
  

    // 主任务循环
    while (true) {
          if (rainbow_active) {
            set_led_rainbow(led_strip, hue_offset);
            hue_offset += 1;

        // Make sure the offset stays within the valid range (0 to 360)
          if (hue_offset >= 360) {
            hue_offset = 0;
           }
        }
    
       
        vTaskDelay(10 / portTICK_PERIOD_MS);
       
    }
}