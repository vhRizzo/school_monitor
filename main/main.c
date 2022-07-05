#include "global_data.h"
#include "wifi_setup.h"
#include "mqtt_setup.h"

#ifdef OTA_SET
#include "ota_setup.h"
#endif /* OTA_SET */

#ifdef BME280_SENSOR
#include "bme_setup.h"
#endif /* BME280_SENSOR */

#ifdef DSM501A_SENSOR
#include "dsm_setup.h"
#endif /* DSM501A_SETUP */

#ifdef INMP441_SENSOR
#include "inmp_setup.h"
#endif /* INMP441_SENSOR */

#ifdef NEO6M_SENSOR
#include "neo_setup.h"
#endif /* NEO6M_SENSOR */

void app_main(void)
{
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    gpio_pad_select_gpio(GPIO_NUM_2);
    gpio_set_direction(GPIO_NUM_2, GPIO_MODE_OUTPUT);
    gpio_set_level(GPIO_NUM_2, 0);
    vTaskDelay(1000/portTICK_PERIOD_MS);
    gpio_set_level(GPIO_NUM_2, 1);

    wifi_init_sta();
    esp_wifi_set_ps(WIFI_PS_NONE);

    #ifdef OTA_SET
    xTaskCreatePinnedToCore( ota_task, "OTA_Task", 1024 * 8, NULL, configMAX_PRIORITIES - 1, NULL, 1 );
    #endif /* OTA_SET */

    xTaskCreatePinnedToCore( rcv_data_task, "Receive_Data_Task", 1024 * 8, NULL, configMAX_PRIORITIES - 2, NULL, 1 );
    
    #ifdef INMP441_SENSOR
    inmp_queue = xQueueCreate(1, sizeof(inmp_t));
    vTaskDelay(500/portTICK_PERIOD_MS);
    xTaskCreatePinnedToCore( mic_i2s_reader_task, "INMP441_READER_TASK", 1024 * 4, NULL, configMAX_PRIORITIES - 4, NULL, 0 );
    xTaskCreatePinnedToCore( mic_i2s_filter_task, "INMP441_FILTER_TASK", 1024 * 4, NULL, configMAX_PRIORITIES - 5, NULL, 0 );
    #endif /* INMP441_SENSOR */

    #ifdef BME280_SENSOR
    bme_queue = xQueueCreate(1, sizeof(bme_t));
    xTaskCreatePinnedToCore( bme_task, "BME280_Task", 1024 * 4, NULL, configMAX_PRIORITIES - 6, NULL, 0 );
    #endif /* BME280_SENSOR */

    #ifdef DSM501A_SENSOR
    dsm_queue = xQueueCreate(1, sizeof(dsm_t));
    xTaskCreatePinnedToCore( dsm_task, "DSM501a_Task", 1024 * 4, NULL, configMAX_PRIORITIES - 7, NULL, 0 );
    #endif /* DSM501A_SENSOR */

    #ifdef NEO6M_SENSOR
    neo_queue = xQueueCreate(1, sizeof(neo_t));
    xTaskCreatePinnedToCore( neo_task, "NEO-6M_Task", 1024 * 4, NULL, configMAX_PRIORITIES - 8, NULL, 1 );
    #endif /* NEO6M_SENSOR */
}
