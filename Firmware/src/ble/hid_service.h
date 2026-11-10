#ifndef REMOTE_HID_SERVICE_H
#define REMOTE_HID_SERVICE_H
#include <stdint.h>
#include <stdbool.h>
uint8_t hid_service_init(void);
void hid_service_key(unsigned key, bool down, uint32_t now);
void hid_service_poll(uint32_t now);
void hid_service_disconnect(void);
void hid_service_ready(void);
bool hid_service_is_suspended(void);
bool hid_service_has_pending(void);
void hid_service_set_wake(void (*wake)(void));
uint8_t remote_hid_secure(void);
uint8_t remote_hid_report_enabled(uint8_t id);
uint8_t remote_hid_subscription(uint8_t id);
extern volatile uint32_t hid_dropped_actions;
typedef struct {
    uint32_t accepted, retries, unsubscribed, unchanged, max_queue_age_ms;
    uint8_t last_status;
} hid_diagnostics_t;
extern volatile hid_diagnostics_t hid_diagnostics;
#endif
