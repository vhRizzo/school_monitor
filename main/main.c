#include "global_data.h"

#ifdef DSM501A_SENSOR
#include "dsm501a_setup.h"
#endif
#ifdef INMP441_SENSOR
#include "inmp441_setup.h"
#endif
#ifdef BME280_SENSOR
#include "bme280_setup.h"
#endif
#ifdef NEO6M_SENSOR
#include "neo6m_setup.h"
#endif

#include "mqtt_setup.h"
#include "wifi_setup.h"
#include "ota_setup.h"

// extern "C" {
//     void app_main(void);
// }

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
    vTaskDelay( 1000/portTICK_PERIOD_MS );
    gpio_set_level(GPIO_NUM_2, 1);

    wifi_init_sta();
    esp_wifi_set_ps(WIFI_PS_NONE);
    
    // xTaskCreatePinnedToCore(ota_task, "OTA_Task", 8 * 1024, NULL, configMAX_PRIORITIES - 1, NULL, 1);
    xTaskCreatePinnedToCore(rcv_data_task, "Data_Receive_Task", 4 * 1024, NULL, configMAX_PRIORITIES - 2, NULL, 1);

    #ifdef INMP441_SENSOR
    inmp_queue = xQueueCreate(1, sizeof(inmp441_t));
    vTaskDelay(500/portTICK_PERIOD_MS); // delay para sincronizar com a task de dados.
    xTaskCreatePinnedToCore(mic_i2s_reader_task, "INMP441_Reader_Task", 2 * 1024, NULL, configMAX_PRIORITIES - 4, NULL, 0);
    xTaskCreatePinnedToCore(mic_i2s_filter_task, "INMP441_Filter_Task", 2 * 1024, NULL, configMAX_PRIORITIES - 5, NULL, 0);
    #endif

    #ifdef BME280_SENSOR
    bme_queue = xQueueCreate(1, sizeof(bme280_t));
    xTaskCreatePinnedToCore(bme280_task, "BME280_Task", 4 * 1024, NULL, configMAX_PRIORITIES - 6, NULL, 0);
    #endif

    #ifdef DSM501A_SENSOR
    dsm_queue = xQueueCreate(1, sizeof(dsm501a_t));
    xTaskCreatePinnedToCore(dsm501a_task, "DSM501a_Task", 4 * 1024, NULL, configMAX_PRIORITIES - 7, NULL, 0);
    #endif

    #ifdef NEO6M_SENSOR
    neo_queue = xQueueCreate(1, sizeof(neo6m_t));
    xTaskCreatePinnedToCore(neo6m_task, "NEO-6M_Task", 4 * 1024, NULL, configMAX_PRIORITIES - 8, NULL, 0);
    #endif
}
