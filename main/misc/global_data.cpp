#include "global_data.h"

dados_t dados;

#ifdef INMP441_SENSOR
inmp441_t inmp_receive;
#endif
#ifdef DSM501A_SENSOR
dsm501a_t dsm_receive;
#endif
#ifdef BME280_SENSOR
bme280_t bme_receive;
#endif
#ifdef NEO6M_SENSOR
neo6m_t neo_receive;
#endif

esp_mqtt_client_handle_t client;

void send_data_task ( void *pvParameters )
{
    mqtt_app_start();

    dados.poeira_pm_10 = -1;
    dados.poeira_pm_25 = -1;
    dados.temperatura = -1;
    dados.umidade = -1;
    dados.pressao = -1;
    dados.ruido = -1;
    dados.coord[0] = -1;
    dados.coord[1] = -1;

    esp_task_wdt_add(NULL);
    while (true) {
        #ifdef INMP441_SENSOR
        if (xQueueReceive(inmp_queue, &inmp_receive, portMAX_DELAY)) {
            dados.ruido = inmp_receive.ruido;
        }
        #endif

        #ifdef DSM501A_SENSOR
        if (xQueueReceive(dsm_queue, &dsm_receive, portMAX_DELAY))
        {
            dados.poeira_pm_10 = dsm_receive.poeira_pm_10;
            dados.poeira_pm_25 = dsm_receive.poeira_pm_25;
        }
        #endif

        #ifdef BME280_SENSOR
        if (xQueueReceive(bme_queue, &bme_receive, portMAX_DELAY))
        {
            dados.temperatura = bme_receive.temperatura;
            dados.umidade = bme_receive.umidade;
            dados.pressao = bme_receive.pressao;
            xTaskNotifyGive(xTaskGetCurrentTaskHandle());
        }
        #endif

        #ifdef NEO6M_SENSOR
        if (xQueueReceive(neo_queue, &neo_receive, portMAX_DELAY))
        {
            dados.coord[0] = neo_receive.coord[0];
            dados.coord[1] = neo_receive.coord[1];
        }
        #endif

        if (ulTaskNotifyTake(pdTRUE, portMAX_DELAY)) {
            ESP_LOGI(__func__, "BME280(%.2f*C, %.2f%%, %dPa), DSM501a(%.2fpcs/0.01cf, %.2fpcs/0.01cf), INMP441(%.2fdB), NEO-6M(%.6f, %.6f).", 
                dados.temperatura, dados.umidade, dados.pressao, dados.poeira_pm_10, dados.poeira_pm_25, dados.ruido, dados.coord[0], dados.coord[1]);
            
            char tmp[21];
            sprintf(tmp, "%.2f", dados.temperatura);
            esp_mqtt_client_publish(client, "/bme280/temperatura", tmp, 0, 0, 0);
            sprintf(tmp, "%.2f", dados.umidade);
            esp_mqtt_client_publish(client, "/bme280/umidade", tmp, 0, 0, 0);
            sprintf(tmp, "%d", dados.pressao);
            esp_mqtt_client_publish(client, "/bme280/pressao", tmp, 0, 0, 0);
            sprintf(tmp, "%.2f", dados.poeira_pm_10);
            esp_mqtt_client_publish(client, "/dsm501a/poeira_pm_10", tmp, 0, 0, 0);
            sprintf(tmp, "%.2f", dados.poeira_pm_25);
            esp_mqtt_client_publish(client, "/dsm501a/poeira_pm_25", tmp, 0, 0, 0);
            sprintf(tmp, "%.2f", dados.ruido);
            esp_mqtt_client_publish(client, "/inmp441/ruido", tmp, 0, 0, 0);
            sprintf(tmp, "%.6f", dados.coord[0]);
            esp_mqtt_client_publish(client, "/neo6m/lat", tmp, 0, 0, 0);
            sprintf(tmp, "%.6f", dados.coord[1]);
            esp_mqtt_client_publish(client, "/neo6m/lon", tmp, 0, 0, 0);

            esp_task_wdt_reset();
        }
    }
}

void mqtt_app_start(void)
{
    esp_log_level_set("*", ESP_LOG_INFO);
    esp_log_level_set("MQTT_CLIENT", ESP_LOG_VERBOSE);
    esp_log_level_set("MQTT_EXAMPLE", ESP_LOG_VERBOSE);
    esp_log_level_set("TRANSPORT_BASE", ESP_LOG_VERBOSE);
    esp_log_level_set("esp-tls", ESP_LOG_VERBOSE);
    esp_log_level_set("TRANSPORT", ESP_LOG_VERBOSE);
    esp_log_level_set("OUTBOX", ESP_LOG_VERBOSE);

    esp_mqtt_client_config_t mqtt_cfg = {
        .uri = CONFIG_BROKER_URL,
        .task_prio = 2,
        .task_stack = 4096,
    };

    client = esp_mqtt_client_init(&mqtt_cfg);
    /* The last argument may be used to pass data to the event handler, in this example mqtt_event_handler */
    esp_mqtt_client_register_event(client, esp_mqtt_event_id_t(ESP_EVENT_ANY_ID), mqtt_event_handler, NULL);
    esp_mqtt_client_start(client);
}
