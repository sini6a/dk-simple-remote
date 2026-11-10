#include <assert.h>
#include <stdio.h>
#include "buttons.h"
#include "keymap.h"
static unsigned presses, releases, last;
static void event(unsigned key, bool down, void *ctx)
{
    (void)ctx;
    last = key;
    if (down) ++presses; else ++releases;
}
static void step(buttons_state_t *s, unsigned down, uint32_t t)
{
    assert(!buttons_step(s, (uint8_t)down, t, event, NULL));
}
static void reset(buttons_state_t *s)
{
    buttons_reset(s);
    presses = releases = 0;
}
int main(void)
{
    buttons_state_t s;
    reset(&s);
    step(&s, 2, 0); step(&s, 0, 10); step(&s, 2, 15);
    step(&s, 2, 34); assert(presses == 0);
    step(&s, 2, 35); assert(presses == 0); /* Up debounced; deferred to release. */
    step(&s, 0, 40); step(&s, 0, 59); assert(presses == 0);
    step(&s, 0, 60); assert(presses == 1 && releases == 1 && last == 1);
    /* Enter is an ordinary key: press when debounced, release when released. */
    reset(&s);
    step(&s, 8, 0); step(&s, 8, 20); step(&s, 8, 6000);
    assert(presses == 1 && releases == 0 && last == 3);
    step(&s, 0, 6010); step(&s, 0, 6030);
    assert(presses == 1 && releases == 1 && last == 3);
    /* Short Power: tap on release (standby). */
    reset(&s);
    step(&s, 1, 0); step(&s, 1, 20); step(&s, 1, 619);
    step(&s, 0, 630); step(&s, 0, 650);
    assert(presses == 1 && releases == 1 && last == 0 && !s.power_down);
    /* Long Power: Power down at threshold, held a fixed APP_POWER_HOLD_MS even
     * though the finger lifts early; no extra tap on release. */
    reset(&s);
    step(&s, 1, 0); step(&s, 1, 20); step(&s, 1, 619); assert(presses == 0);
    step(&s, 1, 620); assert(presses == 1 && releases == 0 && last == 0 && s.power_down);
    step(&s, 0, 700); step(&s, 0, 720); assert(releases == 0 && s.power_down);
    step(&s, 0, 620 + APP_POWER_HOLD_MS - 1); assert(releases == 0);
    step(&s, 0, 620 + APP_POWER_HOLD_MS);
    assert(presses == 1 && releases == 1 && last == 0 && !s.power_down);
    step(&s, 0, 5000); assert(presses == 1 && releases == 1);
    /* Finger held past the fixed window: still exactly one long press. */
    reset(&s);
    step(&s, 1, 0); step(&s, 1, 20); step(&s, 1, 620);
    step(&s, 1, 620 + APP_POWER_HOLD_MS); assert(releases == 1 && !s.power_down);
    step(&s, 1, 8000); step(&s, 0, 8010); step(&s, 0, 8030);
    assert(presses == 1 && releases == 1);
    /* A Power tap during the timed hold cannot cut it short. */
    reset(&s);
    step(&s, 1, 0); step(&s, 1, 20); step(&s, 1, 620);
    step(&s, 0, 700); step(&s, 0, 720);
    step(&s, 1, 800); step(&s, 1, 820); step(&s, 0, 900); step(&s, 0, 920);
    assert(presses == 1 && releases == 0 && s.power_down);
    step(&s, 0, 620 + APP_POWER_HOLD_MS); assert(presses == 1 && releases == 1);
    /* Chord countdown starts after both keys debounce, fires exactly once. */
    reset(&s);
    step(&s, 1, 0); step(&s, 1, 20);
    step(&s, 3, 100); step(&s, 3, 120); step(&s, 3, 5119);
    assert(buttons_step(&s, 3, 5120, event, NULL));
    step(&s, 3, 10000);
    /* Up still held alone after the chord: no Volume Up, no Up tap. */
    step(&s, 2, 10010); step(&s, 2, 10030); step(&s, 2, 12000);
    step(&s, 0, 12010); step(&s, 0, 12030);
    assert(presses == 0 && releases == 0);
    /* Up first, then Power within the long-press time: chord, no Volume Up. */
    reset(&s);
    step(&s, 2, 0); step(&s, 2, 20);
    step(&s, 3, 300); step(&s, 3, 320); step(&s, 3, 2000);
    assert(presses == 0 && !s.power_down);
    assert(buttons_step(&s, 3, 5320, event, NULL));
    step(&s, 0, 5330); step(&s, 0, 5350);
    assert(presses == 0 && releases == 0);
    /* SW1+SW2 together never trigger the Power long-press or Volume Up. */
    reset(&s);
    step(&s, 3, 0); step(&s, 3, 20); step(&s, 3, 3000);
    assert(presses == 0 && !s.power_down);
    /* Enter no longer forms a chord with Power. */
    reset(&s);
    step(&s, 9, 0); step(&s, 9, 20); step(&s, 9, 6000);
    assert(presses == 2 && releases == 1 && last == 0 && !s.power_down);
    /* Aborted chord suppresses Power/Up too; another full hold can fire. */
    reset(&s);
    step(&s, 3, 0); step(&s, 3, 20);
    step(&s, 0, 100); step(&s, 0, 120);
    assert(presses == 0 && releases == 0);
    step(&s, 3, 200); step(&s, 3, 220);
    assert(buttons_step(&s, 3, 5220, event, NULL));
    /* Millisecond rollover during debounce and chord timing. */
    reset(&s);
    step(&s, 3, UINT32_MAX - 30U); step(&s, 3, UINT32_MAX - 10U);
    step(&s, 3, 4988);
    assert(buttons_step(&s, 3, 4989, event, NULL));
    /* All non-chord keys can coexist and each releases exactly once.
     * All four arrows defer their tap to release. */
    reset(&s);
    step(&s, 0x36, 0); step(&s, 0x36, 20);
    assert(presses == 0);
    step(&s, 0, 50); step(&s, 0, 70); assert(presses == 4 && releases == 4);
    /* Short Left: nothing while held, Left tap on release. */
    reset(&s);
    step(&s, 4, 0); step(&s, 4, 20); step(&s, 4, 619);
    assert(presses == 0);
    step(&s, 0, 630); step(&s, 0, 650);
    assert(presses == 1 && releases == 1 && last == 2);
    /* Long Left: one Back tap at the threshold, nothing on release. */
    reset(&s);
    step(&s, 4, 0); step(&s, 4, 20); step(&s, 4, 619); assert(presses == 0);
    step(&s, 4, 620); assert(presses == 1 && releases == 1 && last == APP_KEY_BACK);
    step(&s, 4, 3000); assert(presses == 1);
    step(&s, 0, 3010); step(&s, 0, 3030); assert(presses == 1 && releases == 1);
    /* Long Right: one Home tap; a following short Right is a normal Right. */
    reset(&s);
    step(&s, 16, 0); step(&s, 16, 20); step(&s, 16, 620);
    assert(presses == 1 && last == APP_KEY_HOME);
    step(&s, 0, 700); step(&s, 0, 720);
    step(&s, 16, 800); step(&s, 16, 820); step(&s, 0, 900); step(&s, 0, 920);
    assert(presses == 2 && releases == 2 && last == 4);
    /* Short Up/Down: arrow tap on release, no volume. */
    reset(&s);
    step(&s, 32, 0); step(&s, 32, 20); step(&s, 32, 500);
    step(&s, 0, 510); step(&s, 0, 530);
    assert(presses == 1 && releases == 1 && last == 5);
    /* Long Up: exactly one Volume Up tap per hold, nothing on release;
     * a second hold gives a second tap. */
    reset(&s);
    step(&s, 2, 0); step(&s, 2, 20); step(&s, 2, 620);
    assert(presses == 1 && releases == 1 && last == APP_KEY_VOLUME_UP);
    step(&s, 2, 5000); assert(presses == 1 && releases == 1);
    step(&s, 0, 5010); step(&s, 0, 5030); assert(presses == 1 && releases == 1);
    step(&s, 2, 6000); step(&s, 2, 6020); step(&s, 2, 6620);
    assert(presses == 2 && releases == 2 && last == APP_KEY_VOLUME_UP);
    /* Long Down: one Volume Down tap. */
    reset(&s);
    step(&s, 32, 0); step(&s, 32, 20); step(&s, 32, 620);
    assert(presses == 1 && releases == 1 && last == APP_KEY_VOLUME_DOWN);
    step(&s, 0, 700); step(&s, 0, 720);
    assert(presses == 1 && releases == 1);
    /* Long-press timing across millisecond rollover. */
    reset(&s);
    step(&s, 4, UINT32_MAX - 30U); step(&s, 4, UINT32_MAX - 10U);
    step(&s, 4, 588); assert(presses == 0);
    step(&s, 4, 589); assert(presses == 1 && last == APP_KEY_BACK);
    assert(keymap_usage(0) == 0x30 && keymap_page(0) == 0x0c);
    assert(keymap_usage(2) == 0x50 && keymap_page(2) == 7);
    assert(keymap_usage(3) == 0x28 && keymap_page(3) == 7);
    assert(keymap_usage(4) == 0x4f && keymap_page(4) == 7);
#if APP_PROGRAM_BUTTONS
    assert(keymap_usage(1) == 0x9c && keymap_page(1) == 0x0c);
    assert(keymap_usage(5) == 0x9d && keymap_page(5) == 0x0c);
#else
    assert(keymap_usage(1) == 0x52 && keymap_page(1) == 7);
    assert(keymap_usage(5) == 0x51 && keymap_page(5) == 7);
#endif
    assert(keymap_usage(APP_KEY_BACK) == 0x224 && keymap_page(APP_KEY_BACK) == 0x0c);
    assert(keymap_usage(APP_KEY_HOME) == 0x223 && keymap_page(APP_KEY_HOME) == 0x0c);
    assert(keymap_usage(APP_KEY_VOLUME_UP) == 0xe9 && keymap_page(APP_KEY_VOLUME_UP) == 0x0c);
    assert(keymap_usage(APP_KEY_VOLUME_DOWN) == 0xea && keymap_page(APP_KEY_VOLUME_DOWN) == 0x0c);
    assert(keymap_usage(10) == 0 && keymap_usage(100) == 0);
    assert(keymap_page(10) == 0 && keymap_page(100) == 0);
    puts("PASS: debounce, press/release, Power deferral, Enter immediate, timed Power long-press, Power+Up chord, Left/Right long-press Back/Home, Up/Down long-press Volume, rollover, keymap");
    return 0;
}
