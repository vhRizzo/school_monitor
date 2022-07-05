#include "global_data.h"

#ifdef NEO6M_SENSOR
#include "neo_setup.h"

QueueHandle_t neo_queue;

void neo_task (void *pvParameters) {
    const uart_port_t uart_num = UART_NUM_2;
    uart_config_t uart_config = {
        .baud_rate = 9600,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE, //UART_HW_FLOWCTRL_CTS_RTS,
        .rx_flow_ctrl_thresh = 122,
    };
    // Configure UART parameters
    uart_param_config(uart_num, &uart_config);
    // Set UART pins(TX, RX, RTS, CTS)
    uart_set_pin(uart_num, TX_pin_GPS, RX_pin_GPS, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);
    // Setup UART buffered IO with event queue
    const int uart_buffer_size = (RD_BUF_SIZE * 2);
    QueueHandle_t uart_queue;
    // Install UART driver using an event queue here
    uart_driver_install(uart_num, uart_buffer_size, 0 /*sem um tx buffer*/, 20, &uart_queue, 0);


    //adaptado de:https://github.com/espressif/esp-idf/blob/master/examples/peripherals/uart/nmea0183_parser/main/nmea_parser.c
    //habilita a detecção de pattern, no caso '\n' ao final e cada item de padrão de GPS
    uart_enable_pattern_det_baud_intr(uart_num, '\n', PATTERN_CHR_NUM, 9, 0, 0);
    /* Set pattern queue size */
    uart_pattern_queue_reset(uart_num, 20);
    uart_flush(uart_num);
    //fim adaptado

    neo_t gps_dados;
    gps_dados.coord[0] = -1;
    gps_dados.coord[0] = -1;
    int pos;

    unsigned long starttime_gps = esp_timer_get_time();

    uart_event_t event;
    uint8_t* dtmp = (uint8_t*) malloc(RD_BUF_SIZE);
    while (1) {
        //Waiting for UART event.
        if (xQueueReceive(uart_queue, (void * )&event, (portTickType)portMAX_DELAY)) {
            bzero(dtmp, RD_BUF_SIZE);
            switch (event.type) {
                //Event of UART receving data
                /*We'd better handler data event fast, there would be much more data events than
                other types of events. If we take too much time on data event, the queue might
                be full.*/
                case UART_DATA:
                    //ESP_LOGI(__func__, "[UART DATA]: %d", event.size);
                    uart_read_bytes(UART_NUM_2, dtmp, event.size, portMAX_DELAY);
                    if ( esp_timer_get_time() - starttime_gps >= TEMPO_ANALISE ) {
                        xQueueOverwrite(neo_queue, (void*)&gps_dados);
                        starttime_gps = esp_timer_get_time();
                    }
                    //ESP_LOGI(__func__, "[DATA EVT]:");
                    //uart_write_bytes(UART_NUM_2, (const char*) dtmp, event.size);
                    break;
                //Event of HW FIFO overflow detected
                case UART_FIFO_OVF:
                    ESP_LOGE(__func__, "hw fifo overflow");
                    // If fifo overflow happened, you should consider adding flow control for your application.
                    // The ISR has already reset the rx FIFO,
                    // As an example, we directly flush the rx buffer here in order to read more data.
                    uart_flush_input(uart_num);
                    xQueueReset(uart_queue);
                    break;
                //Event of UART ring buffer full
                case UART_BUFFER_FULL:
                    ESP_LOGE(__func__, "ring buffer full");
                    // If buffer full happened, you should consider encreasing your buffer size
                    // As an example, we directly flush the rx buffer here in order to read more data.
                    uart_flush_input(uart_num);
                    xQueueReset(uart_queue);
                    break;
                //Event of UART RX break detected
                case UART_BREAK:
                    ESP_LOGE(__func__, "uart rx break");
                    break;
                //Event of UART parity check error
                case UART_PARITY_ERR:
                    ESP_LOGE(__func__, "uart parity error");
                    break;
                //Event of UART frame error
                case UART_FRAME_ERR:
                    ESP_LOGE(__func__, "uart frame error");
                    break;
                case UART_DATA_BREAK:
                    ESP_LOGE(__func__, "uart data break");
                    break;
                case UART_EVENT_MAX:
                    ESP_LOGE(__func__, "uart event max");
                    break;
                //UART_PATTERN_DET
                case UART_PATTERN_DET:
                    pos = uart_pattern_pop_pos(uart_num);
                    //ESP_LOGI(__func__, "[UART PATTERN DETECTED] pos: %d, buffered size: %d", pos, buffered_size);
                    if (pos == -1) {
                        // There used to be a UART_PATTERN_DET event, but the pattern position queue is full so that it can not
                        // record the position. We should set a larger queue size.
                        // As an example, we directly flush the rx buffer here.
                        uart_flush_input(uart_num);
                    } else {
                        uart_read_bytes(uart_num, dtmp, pos, 100 / portTICK_PERIOD_MS); //faz a leitura do buffer até pos, antes do '\n'

                        uint8_t pat[PATTERN_CHR_NUM + 1];
                        memset(pat, 0, sizeof(pat));
                        uart_read_bytes(uart_num, pat, PATTERN_CHR_NUM, 100 / portTICK_PERIOD_MS); //retira o padrão '\n' do buffer, preparando o buffer para ser lido de novo...

                        //abaixo token com base no ',' e o if em seguida só analisa em laço while() se 1o token GPGLL que tem latitude e longintude... o resto ignora e segue...
                        char *ptr_dtmp_tok = strtok ((char*)dtmp, ","); //https://www.best-microcontroller-projects.com/arduino-strtok.html
                        if (ptr_dtmp_tok != NULL) {
                            if (strcmp(ptr_dtmp_tok, "$GPGLL") == 0) {
                                uint8_t contador = 0; //já usou o "$GPGLL", agora pegar latitude (contador 0) e sequencias...
                                ptr_dtmp_tok = strtok (NULL, ","); //tenta próximo token. Se valido, será Latitude, e na próxima interação while() outros do padrão GPGLL...
                                while (ptr_dtmp_tok != NULL) {  //pega próximo token valido e analisa //https://www.best-microcontroller-projects.com/arduino-strtok.html
                                    contador++;
                                    if (contador == 1 || contador == 3) { //latitude || longitude
                                        float ll = strtof(ptr_dtmp_tok, NULL);
                                        int deg = ((int)ll) / 100;
                                        float min = ll - (deg * 100);
                                        ll = deg + min / 60.0f;
                                        gps_dados.coord[!(contador % 3)] = ll; //contador%3==1 (latitude) , contador%3==0 (longitude),
                                        //ao aplicar !, inverte 0 para 1 e vice versa para acomodar no
                                        //indice correto do vetor
                                    }
                                    if (contador == 2 || contador == 4) { // Latitude north(1)/south(-1) information
                                        // Longitude east(1)/west(-1) information
                                        if (ptr_dtmp_tok[0] == 'S' || ptr_dtmp_tok[0] == 's')
                                            gps_dados.coord[0] *= -1;

                                        if (ptr_dtmp_tok[0] == 'W' || ptr_dtmp_tok[0] == 'w')
                                            gps_dados.coord[1] *= -1;
                                    }
                                    ptr_dtmp_tok = strtok (NULL, ","); //pega o próximo token válido (se tiver ,, gerará NULL, então alguma imprecisão do GPS... sairá do laço)
                                }
                                xQueueOverwrite(neo_queue, (void *)&gps_dados);
                            }
                        }
                    }
                    break;
            }
        }
    }
}

#endif