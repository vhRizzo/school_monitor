#ifndef MQTT_SETUP_H
#define MQTT_SETUP_H

#include "global_data.h"

void log_error_if_nonzero(const char *message, int error_code);
void mqtt_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data);
void mqtt_app_start(void);

#endif /* MQTT_SETUP_H */