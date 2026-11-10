#ifndef REMOTE_BUTTONS_H
#define REMOTE_BUTTONS_H
#include <stdbool.h>
#include <stdint.h>
#include "app_config.h"
typedef void (*buttons_emit_fn)(unsigned button, bool pressed, void *context);
typedef struct {
    uint32_t changed_at[APP_BUTTON_COUNT];
    uint32_t held_at[APP_BUTTON_COUNT];
    uint32_t chord_at, power_at;
    uint8_t candidate, stable, consumed, long_fired;
    bool chord_active, chord_fired;
    bool power_down; /* Timed Power long-press in progress; keep sampling. */
} buttons_state_t;
void buttons_reset(buttons_state_t *s);
/* now_ms is monotonic modulo 2^32; call at least every APP_SAMPLE_MS while active.
 * Returns one pairing request per continuous chord; never erases flash itself.
 * Pairing chord is SW1 (Power) + SW2 (Up). SW1 taps on release; SW2 is a
 * long-press key, and once part of the chord it emits neither its tap nor
 * Volume Up. SW4 (Enter) is an ordinary key: press and release as they happen.
 * APP_LONG_PRESS_MASK keys emit their own tap on a short release, or one
 * virtual-key tap (Volume/Back/Home) once held for APP_LONG_PRESS_MS.
 * SW1 alone held APP_LONG_PRESS_MS presses Power for APP_POWER_HOLD_MS. */
bool buttons_step(buttons_state_t *s, uint8_t down, uint32_t now_ms,
                  buttons_emit_fn emit, void *context);
void buttons_gpio_init(void);
void buttons_gpio_arm(void);
void buttons_gpio_disarm(void);
uint8_t buttons_gpio_take_wake(void);
uint8_t buttons_gpio_wake_pending(void);
uint8_t buttons_gpio_read(void);
#endif
