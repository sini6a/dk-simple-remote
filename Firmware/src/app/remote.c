#include "CONFIG.h"
#include "buttons.h"
#include "keymap.h"
#include "remote.h"
#include "ble_manager.h"
#include "hid_service.h"
#define SAMPLE_EVENT 0x0001
volatile remote_diagnostics_t remote_diagnostics;
static buttons_state_t buttons;
static uint8_t task;
static bool sampling;
static void request_poll(void) { tmos_set_event(task, SAMPLE_EVENT); }
static uint32_t previous_ticks, now_ms, fraction;
static void emit(unsigned key, bool down, void *context)
{
    (void)context;
    remote_diagnostics.last_page = keymap_page(key);
    remote_diagnostics.last_usage = keymap_usage(key);
    if (down) ++remote_diagnostics.presses;
    else ++remote_diagnostics.releases;
    hid_service_key(key, down, now_ms);
}
static uint16_t process(uint8_t id, uint16_t events)
{
    if (events & SYS_EVENT_MSG) {
        uint8_t *msg;
        while ((msg = tmos_msg_receive(id)) != NULL) tmos_msg_deallocate(msg);
        return events ^ SYS_EVENT_MSG;
    }
    if (events & SAMPLE_EVENT) {
        uint32_t ticks = TMOS_GetSystemClock();
        buttons_gpio_disarm();
        sampling = true;
        uint32_t delta = ticks - previous_ticks;
        previous_ticks = ticks;
        /* 625us ticks: preserve fractional ms, and timer wrap. */
        now_ms += (delta / 8U) * 5U;
        fraction += (delta % 8U) * 625U;
        now_ms += fraction / 1000U;
        fraction %= 1000U;
        if (buttons_step(&buttons, buttons_gpio_read(), now_ms, emit, NULL)) {
            ++remote_diagnostics.pairing_requests;
            ble_manager_pairing_reset();
        }
        ble_manager_buttons(buttons.stable);
        hid_service_poll(now_ms);
        if (buttons.candidate || buttons.stable || buttons.power_down ||
            buttons_gpio_read() || hid_service_has_pending()) {
            tmos_start_task(task, SAMPLE_EVENT, MS1_TO_SYSTEM_TIME(APP_SAMPLE_MS));
        } else {
            sampling = false;
            buttons_gpio_arm();
            /* Security and CCC callbacks wake this task when reports need work.
             * No periodic button polling remains while all switches are idle. */
        }
        return events ^ SAMPLE_EVENT;
    }
    return 0;
}
void remote_init(void)
{
    buttons_reset(&buttons);
    task = TMOS_ProcessEventRegister(process);
    if (task == INVALID_TASK_ID) while (1) { /* Fatal init: inspect debugger. */ }
    hid_service_set_wake(request_poll);
    previous_ticks = TMOS_GetSystemClock();
    tmos_set_event(task, SAMPLE_EVENT);
}

void remote_process_wake(void)
{
    if (buttons_gpio_take_wake() && !sampling) {
        tmos_stop_task(task, SAMPLE_EVENT);
        tmos_set_event(task, SAMPLE_EVENT);
    }
}
