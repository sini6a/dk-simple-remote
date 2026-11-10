#ifndef REMOTE_APP_H
#define REMOTE_APP_H
#include <stdint.h>
typedef struct {
    uint32_t presses, releases, pairing_requests;
    uint16_t last_usage;
    uint16_t last_page;
} remote_diagnostics_t;
/* Inspect button counters with a debugger; normal operation emits HID reports. */
extern volatile remote_diagnostics_t remote_diagnostics;
void remote_init(void);
void remote_process_wake(void);
#endif
