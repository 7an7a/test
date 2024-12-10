#ifndef HTTPS_REQUEST_H
#define HTTPS_REQUEST_H


#include <stddef.h>
#include "esp_tls.h"



// HTTP 请求信息
#define WEB_SERVER "apifoxmock.com"
#define WEB_URL "https://apifoxmock.com/m1/5440136-5115246-default/light/status"




// 函数声明
void https_get_request(esp_tls_cfg_t cfg, const char *WEB_SERVER_URL, const char *REQUEST);

#endif // HTTPS_REQUEST_H
