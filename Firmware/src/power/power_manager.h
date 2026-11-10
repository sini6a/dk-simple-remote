#ifndef REMOTE_POWER_MANAGER_H
#define REMOTE_POWER_MANAGER_H
#include <stdint.h>
void power_manager_init(void);
/* On-demand register snapshot; no timer, ADC measurement or flash write. */
void power_manager_read_status(uint8_t out[8]);
#endif
