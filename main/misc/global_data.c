#include "global_data.h"

dados_t dados;

#ifdef BME280_SENSOR
bme_t bme_rcv;
#endif /* BME280_SENSOR */

#ifdef DSM501A_SENSOR
dsm_t dsm_rcv;
#endif /* DSM501A_SENSOR */

#ifdef INMP441_SENSOR
inmp_t inmp_rcv;
#endif /* INMP441_SENSOR */

#ifdef NEO6M_SENSOR
neo_t neo_rcv;
#endif /* NEO6M_SENSOR */

void rcv_data_task ( void* pvParameters )
{
    mqtt_app_start();
    
    dados.temp = -1;
    dados.umid = -1;
    dados.pres = -1;
    dados.poeira_pm_10 = -1;
    dados.poeira_pm_25 = -1;
    dados.ruido = -1;
    dados.coord[0] = -1;
    dados.coord[1] = -1;

    esp_timer_create_args_t data_timer_args = {
        .callback = &send_data_timer_func,
        .name = "data_timer"
    };

    esp_timer_handle_t data_timer;
    ESP_ERROR_CHECK( esp_timer_create( &data_timer_args, &data_timer ) );

    esp_timer_create_args_t startup_timer_args = {
        .callback = &startup_timer_func,
        .arg = (void*) data_timer,
        .name = "startup_timer"
    };

    esp_timer_handle_t startup_timer;
    ESP_ERROR_CHECK( esp_timer_create( &startup_timer_args, &startup_timer ) );
    ESP_ERROR_CHECK( esp_timer_start_once( startup_timer, TEMPO_STARTUP * 1000000 ) );

    for ( ; ; )
    {
        #ifdef INMP441_SENSOR
        if ( xQueueReceive( inmp_queue, &inmp_rcv, portMAX_DELAY ) )
        {
            dados.ruido = inmp_rcv.ruido;
        }
        #endif /* INMP441_SENSOR */

        #ifdef BME280_SENSOR
        if ( xQueueReceive( bme_queue, &bme_rcv, portMAX_DELAY ) )
        {
            dados.temp = bme_rcv.temp;
            dados.umid = bme_rcv.umid;
            dados.pres = bme_rcv.pres;
        }
        #endif /* BME280_SENSOR */

        #ifdef DSM501A_SENSOR
        if ( xQueueReceive( dsm_queue, &dsm_rcv, portMAX_DELAY ) )
        {
            dados.poeira_pm_10 = dsm_rcv.poeira_pm_10;
            dados.poeira_pm_25 = dsm_rcv.poeira_pm_25;
        }
        #endif /* DSM501A_SENSOR */

        #ifdef NEO6M_SENSOR
        if ( xQueueReceive( neo_queue, &neo_rcv, portMAX_DELAY ) )
        {
            dados.coord[0] = neo_rcv.coord[0];
            dados.coord[1] = neo_rcv.coord[1];
        }
        #endif /* NEO6M_SENSOR */
    }
}

void startup_timer_func ( void* arg ) { ESP_ERROR_CHECK( esp_timer_start_periodic( (esp_timer_handle_t)arg , TEMPO_ANALISE * 1000000 ) ); }

void send_data_timer_func ( void* arg )
{
    ESP_LOGI( __func__, "BME280( %.2f*C, %.2f%%, %dPa ) - "
                        "DSM501a( %.2fpcs/0.01cf, %.2fpcs/0.01cf ) - "
                        "INMP441( %.2fdB ) - "
                        "NEO-6M( %.6f, %.6f )", 
                        dados.temp, dados.umid, dados.pres,
                        dados.poeira_pm_10, dados.poeira_pm_25,
                        dados.ruido,
                        dados.coord[0], dados.coord[1]);

    char tmp[21];
    sprintf(tmp, "%.2f", dados.temp);
    esp_mqtt_client_publish(client, BME_TEMP_TOPIC, tmp, 0, 0, 0);
    sprintf(tmp, "%.2f", dados.umid);
    esp_mqtt_client_publish(client, BME_UMID_TOPIC, tmp, 0, 0, 0);
    sprintf(tmp, "%d", dados.pres);
    esp_mqtt_client_publish(client, BME_PRES_TOPIC, tmp, 0, 0, 0);
    sprintf(tmp, "%.2f", dados.poeira_pm_10);
    esp_mqtt_client_publish(client, DSM_PM10_TOPIC, tmp, 0, 0, 0);
    sprintf(tmp, "%.2f", dados.poeira_pm_25);
    esp_mqtt_client_publish(client, DSM_PM25_TOPIC, tmp, 0, 0, 0);
    sprintf(tmp, "%.2f", dados.ruido);
    esp_mqtt_client_publish(client, _INMP_DB_TOPIC, tmp, 0, 0, 0);
    if (abs(dados.coord[0]) <= 90 && dados.coord[0] != 0) {
        sprintf(tmp, "%.6f", dados.coord[0]);
        esp_mqtt_client_publish(client, _NEO_LAT_TOPIC, tmp, 0, 0, 0);
    }
    if (abs(dados.coord[1]) <= 180 && dados.coord[1] != 0) {
        sprintf(tmp, "%.6f", dados.coord[1]);
        esp_mqtt_client_publish(client, _NEO_LNG_TOPIC, tmp, 0, 0, 0);
    }
}
