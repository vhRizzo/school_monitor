#ifndef GLOBAL_DATA_H
#define GLOBAL_DATA_H

#define TEMPO_ANALISE   60
#define TEMPO_STARTUP   60

#define OTA_SET
#define BME280_SENSOR
#define DSM501A_SENSOR
#define INMP441_SENSOR
#define NEO6M_SENSOR

#define WIFI_SSID       "Herbert Richers"
#define WIFI_PASS       "kinodertoten"
#define MAXIMUM_RETRY   10

#define BROKER_URL      "mqtt://user1:gabriel@iot.coenc.ap.utfpr.edu.br"
#define BME_TEMP_TOPIC  "tuto/2104652/_bme280/temp"
#define BME_UMID_TOPIC  "tuto/2104652/_bme280/umid"
#define BME_PRES_TOPIC  "tuto/2104652/_bme280/pres"
#define DSM_PM10_TOPIC  "tuto/2104652/dsm501a/pm10"
#define DSM_PM25_TOPIC  "tuto/2104652/dsm501a/pm25"
#define _INMP_DB_TOPIC  "tuto/2104652/inmp441/_dB_"
#define _NEO_LAT_TOPIC  "tuto/2104652/_neo6m_/_lat"
#define _NEO_LNG_TOPIC  "tuto/2104652/_neo6m_/_lng"
#define OTA_TRIG_TOPIC  "tuto/2104652/ota_trg/_ota"
#define MQTT_CONN_EVNT  "tuto/2104652/wifi_ev/wifi"

#define OTA_URL         "https://raw.githubusercontent.com/vhRizzo/ota_test/main/ota_test/tuto_monitor.bin"
#define OTA_CHECK       300

#include <math.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <stddef.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "freertos/semphr.h"
#include "freertos/event_groups.h"
#include "esp_spi_flash.h"
#include "driver/i2c.h"
#include "driver/i2s.h"
#include "driver/uart.h"
#include "esp_err.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "esp_now.h"
#include "nvs_flash.h"
#include "esp_netif.h"
#include "mqtt_client.h"
#include "esp_ota_ops.h"
#include "esp_http_client.h"
#include "esp_https_ota.h"
#include "nvs.h"
#include "mqtt_setup.h"

#include "lwip/err.h"
#include "lwip/sys.h"
#include "lwip/sockets.h"
#include "lwip/dns.h"
#include "lwip/netdb.h"

void rcv_data_task ( void* pvParameters );
void startup_timer_func ( void* arg );
void send_data_timer_func ( void* arg );

#ifdef BME280_SENSOR
typedef struct __attribute__((__packed__))
{
    float temp;
    float umid;
    uint32_t pres;
} bme_t;
extern QueueHandle_t bme_queue;
#endif /* BME280_SENSOR */

#ifdef INMP441_SENSOR
typedef struct __attribute__((__packed__))
{
    double ruido;
} inmp_t;
extern QueueHandle_t inmp_queue;
#endif /* INMP441_SENSOR */

#ifdef DSM501A_SENSOR
typedef struct __attribute__((__packed__))
{
    float poeira_pm_10;
    float poeira_pm_25;
} dsm_t;
extern QueueHandle_t dsm_queue;
#endif /* DSM501A_SENSOR */

#ifdef NEO6M_SENSOR
typedef struct __attribute__((__packed__))
{
    float coord[2]; // [0] lat - [1] lng
} neo_t;
extern QueueHandle_t neo_queue;
#endif /* NEO6M_SENSOR */

typedef struct __attribute__((__packed__))
{
    float temp;
    float umid;
    uint32_t pres;
    double ruido;
    float poeira_pm_10;
    float poeira_pm_25;
    float coord[2];
} dados_t;

extern esp_mqtt_client_handle_t client;

#endif /* GLOBAL_DATA_H */
