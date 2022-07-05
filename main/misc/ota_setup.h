#ifndef OTA_SETUP_H
#define OTA_SETUP_H

#include "global_data.h"

#define OTA_URL_SIZE 256

esp_err_t validate_image_header(esp_app_desc_t *new_app_info);
esp_err_t _http_client_init_cb(esp_http_client_handle_t http_client);
void ota_task(void *pvParameter);

#endif /* OTA_SETUP_H */