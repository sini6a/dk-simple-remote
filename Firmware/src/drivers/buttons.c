#include "buttons.h"
#include <string.h>
#define POWER_BIT (1U << 0)
#define UP_BIT (1U << 1)
/* SW1 (Power) + SW2 (Up) held APP_PAIR_HOLD_MS clears bonds and reopens pairing. */
#define PAIR_CHORD_MASK (POWER_BIT | UP_BIT)
void buttons_reset(buttons_state_t *s) { memset(s, 0, sizeof(*s)); }
bool buttons_step(buttons_state_t *s, uint8_t down, uint32_t now,
                  buttons_emit_fn emit, void *context)
{
    uint8_t previous = s->stable;
    bool pair = false;
    unsigned i;
    down &= (1U << APP_BUTTON_COUNT) - 1U;
    for (i = 0; i < APP_BUTTON_COUNT; ++i) {
        uint8_t bit = (uint8_t)(1U << i);
        if ((down ^ s->candidate) & bit) {
            s->candidate ^= bit;
            s->changed_at[i] = now;
        }
        if (((s->candidate ^ s->stable) & bit) &&
            (uint32_t)(now - s->changed_at[i]) >= APP_DEBOUNCE_MS) {
            s->stable ^= bit;
            if (s->stable & bit) s->held_at[i] = now; /* Debounced press. */
        }
    }
    if ((s->stable & PAIR_CHORD_MASK) == PAIR_CHORD_MASK) {
        s->consumed |= PAIR_CHORD_MASK;
        if (!s->chord_active) {
            s->chord_active = true;
            s->chord_at = now;
        }
        if (!s->chord_fired && (uint32_t)(now - s->chord_at) >= APP_PAIR_HOLD_MS) {
            pair = true;
            s->chord_fired = true;
        }
    } else {
        s->chord_active = false;
    }
    for (i = 0; i < APP_BUTTON_COUNT; ++i) {
        uint8_t bit = (uint8_t)(1U << i);
        if (!((previous ^ s->stable) & bit)) continue;
        if (bit == POWER_BIT) {
            /* A Power tap while the timed Power hold is active is ignored,
             * so it cannot cut the long press short. */
            if (!(s->stable & bit) && !(s->consumed & bit) && !s->power_down) {
                emit(i, true, context);
                emit(i, false, context);
            }
        } else if (bit & APP_LONG_PRESS_MASK) {
            /* Press is deferred until release or long-press. A key consumed by
             * the pairing chord (Up) emits nothing on release. */
            if (!(s->stable & bit)) {
                if (!(s->long_fired & bit) && !(s->consumed & bit)) {
                    emit(i, true, context);
                    emit(i, false, context);
                }
                s->long_fired &= (uint8_t)~bit;
            }
        } else {
            emit(i, (s->stable & bit) != 0, context);
        }
    }
    for (i = 0; i < APP_BUTTON_COUNT; ++i) {
        uint8_t bit = (uint8_t)(1U << i);
        /* Raw input must still be down: never fire during release debounce. */
        if (!(bit & APP_LONG_PRESS_MASK & s->stable & s->candidate) ||
            (s->long_fired & bit) || (s->consumed & bit)) continue;
        if ((uint32_t)(now - s->held_at[i]) >= APP_LONG_PRESS_MS) {
            unsigned key = APP_LONG_KEY(i);
            s->long_fired |= bit;
            emit(key, true, context); /* One tap per hold. */
            emit(key, false, context);
        }
    }
    /* Power long-press: SW1 alone held APP_LONG_PRESS_MS presses Power for a
     * fixed APP_POWER_HOLD_MS (Android power menu), independent of when the
     * finger lifts, so a long press can never degrade into a standby tap.
     * The SW1+SW2 chord consumes SW1 and therefore takes priority. */
    if (!s->power_down && (s->stable & s->candidate & POWER_BIT) &&
        !((s->stable | s->candidate) & UP_BIT) && !(s->consumed & POWER_BIT) &&
        (uint32_t)(now - s->held_at[0]) >= APP_LONG_PRESS_MS) {
        s->consumed |= POWER_BIT; /* No short tap on release. */
        s->power_down = true;
        s->power_at = now;
        emit(0, true, context);
    } else if (s->power_down && (uint32_t)(now - s->power_at) >= APP_POWER_HOLD_MS) {
        s->power_down = false;
        emit(0, false, context);
    }
    if (!(s->stable & PAIR_CHORD_MASK)) {
        s->consumed = 0;
        s->chord_fired = false;
    }
    return pair;
}
