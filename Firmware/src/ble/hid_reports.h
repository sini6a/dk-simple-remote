#ifndef REMOTE_HID_REPORTS_H
#define REMOTE_HID_REPORTS_H
#include <stdint.h>
#include <stdbool.h>
#define HID_PENDING_FRAMES 8
#define HID_PENDING_TTL_MS 2000U
typedef struct { uint8_t keyboard[8], consumer[2]; uint32_t at; } hid_frame_t;
typedef struct {
    hid_frame_t queue[HID_PENDING_FRAMES];
    uint8_t head, count, phase;
    uint16_t down; /* One bit per APP_KEY_COUNT physical/virtual key. */
    bool blocked;
    uint32_t dropped;
} hid_reports_t;
void hid_reports_reset(hid_reports_t *s);
void hid_reports_release(hid_reports_t *s, uint32_t now);
void hid_reports_key(hid_reports_t *s, unsigned key, bool down, uint32_t now);
/* False when empty; report ID and body exclude the on-wire report ID byte. */
bool hid_reports_peek(hid_reports_t *s, uint32_t now, uint8_t *id, const uint8_t **body, uint8_t *len);
void hid_reports_ack(hid_reports_t *s);
#endif
