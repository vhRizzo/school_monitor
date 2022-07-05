#ifndef NEO_SETUP_H
#define NEO_SETUP_H

#include "global_data.h"

#define TX_pin_GPS          32 //pin
#define RX_pin_GPS          33 //pin
#define RD_BUF_SIZE         1024
#define PATTERN_CHR_NUM     (1)

void neo_task (void *pvParameters);

#endif /* NEO_SETUP_H */