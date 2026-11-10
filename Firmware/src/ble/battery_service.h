#ifndef REMOTE_BATTERY_SERVICE_H
#define REMOTE_BATTERY_SERVICE_H
#include <stdint.h>
uint8_t battery_service_init(void);
void battery_service_link(uint16_t connection);
void battery_service_notify(void);
#endif
