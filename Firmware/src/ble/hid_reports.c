#include "hid_reports.h"
#include "keymap.h"
#include "app_config.h"
#include <string.h>
void hid_reports_reset(hid_reports_t *s) { memset(s, 0, sizeof(*s)); }
void hid_reports_release(hid_reports_t *s, uint32_t now)
{
    s->head = s->phase = 0; s->count = 1;
    memset(&s->queue[0], 0, sizeof(s->queue[0]));
    s->queue[0].at = now;
}
void hid_reports_key(hid_reports_t *s, unsigned key, bool down, uint32_t now)
{
    unsigned i, used = 2;
    hid_frame_t *f;
    if (key >= APP_KEY_COUNT) return;
    if (down) s->down |= (uint16_t)(1U << key);
    else s->down &= (uint16_t)~(1U << key);
    if (s->blocked) {
        if (!s->down) s->blocked = false;
        return;
    }
    if (s->count == HID_PENDING_FRAMES) {
        s->dropped += s->count + 1U;
        hid_reports_release(s, now);
        s->blocked = s->down != 0;
        return;
    }
    f = &s->queue[(s->head + s->count) % HID_PENDING_FRAMES];
    memset(f, 0, sizeof(*f)); f->at = now;
    for (i = 0; i < APP_KEY_COUNT; ++i) {
        uint16_t page, usage;
        if (!(s->down & (1U << i))) continue;
        page = keymap_page(i); usage = keymap_usage(i);
        if (page == 7 && used < 8 && usage <= 255) f->keyboard[used++] = (uint8_t)usage;
        else if (page == 12 && !f->consumer[0] && !f->consumer[1]) {
            f->consumer[0] = (uint8_t)usage;
            f->consumer[1] = (uint8_t)(usage >> 8);
        }
    }
    ++s->count;
}
bool hid_reports_peek(hid_reports_t *s, uint32_t now, uint8_t *id, const uint8_t **body, uint8_t *len)
{
    hid_frame_t *f;
    unsigned i;
    bool nonzero = false;
    if (!s->count) return false;
    f = &s->queue[s->head];
    /* Expiry replaces the entire backlog with a retained all-keys-up frame.
     * Releases never expire: retry them until success or physical link loss. */
    for (i = 0; i < sizeof(f->keyboard); ++i) nonzero |= f->keyboard[i] != 0;
    nonzero |= f->consumer[0] != 0 || f->consumer[1] != 0;
    if (nonzero && (uint32_t)(now - f->at) >= HID_PENDING_TTL_MS) {
        s->dropped += s->count;
        hid_reports_release(s, now); s->blocked = s->down != 0;
        f = &s->queue[0];
    }
    *id = s->phase ? 2 : 1;
    *body = s->phase ? f->consumer : f->keyboard;
    *len = s->phase ? 2 : 8;
    return true;
}
void hid_reports_ack(hid_reports_t *s)
{
    if (!s->count) return;
    if (!s->phase) s->phase = 1;
    else { s->phase = 0; s->head = (s->head + 1U) % HID_PENDING_FRAMES; --s->count; }
}
