#ifndef DSM_SETUP_H
#define DSM_SETUP_H

#include "global_data.h"

#define DSM_PIN_VERM            GPIO_NUM_34
#define DSM_PIN_AMAR            GPIO_NUM_35
#define GPIO_INPUT_PIN_SEL      (uint64_t)((1ULL<<DSM_PIN_VERM) | (1ULL<<DSM_PIN_AMAR))
#define ESP_INTR_FLAG_DEFAULT   0

void IRAM_ATTR change_v2_isr ( void* arg );
void IRAM_ATTR change_v1_isr ( void* arg );
void dsm_task ( void* pvParameters );

#endif /* DSM_SETUP_H */