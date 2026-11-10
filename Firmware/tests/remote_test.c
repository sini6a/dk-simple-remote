#include <assert.h>
#include <stdio.h>
#include "CONFIG.h"
#include "buttons.h"
#include "hid_service.h"
#include "remote.h"
static uint16_t (*process)(uint8_t,uint16_t);
static void (*wake)(void);
static uint32_t ticks, delay;
static unsigned events, raw, irq, armed, reports, pending, pumps;
uint8_t TMOS_ProcessEventRegister(uint16_t (*fn)(uint8_t,uint16_t)) { process=fn; return 1; }
uint32_t TMOS_GetSystemClock(void) { return ticks; }
void tmos_set_event(uint8_t t,uint16_t e) { assert(t==1); events|=e; }
void tmos_start_task(uint8_t t,uint16_t e,uint32_t n) { assert(t==1 && e==1); delay=n; }
void tmos_stop_task(uint8_t t,uint16_t e) { assert(t==1 && e==1); delay=0; }
uint8_t *tmos_msg_receive(uint8_t t) { (void)t; return NULL; }
void tmos_msg_deallocate(uint8_t *m) { (void)m; }
uint8_t buttons_gpio_read(void) { return raw; }
void buttons_gpio_arm(void) { armed=1; }
void buttons_gpio_disarm(void) { armed=0; }
uint8_t buttons_gpio_take_wake(void) { unsigned n=irq; irq=0; return n; }
void ble_manager_buttons(uint8_t b) { (void)b; }
void ble_manager_pairing_reset(void) {}
void hid_service_set_wake(void (*fn)(void)) { wake=fn; }
bool hid_service_has_pending(void) { return pending; }
void hid_service_key(unsigned k,bool down,uint32_t now) { assert(k==1); (void)down; (void)now; ++reports; }
void hid_service_poll(uint32_t now) { (void)now; ++pumps; }
static void step(unsigned ms) { ticks+=ms*8/5; events=0; delay=0; assert(process(1,1)==0); }
int main(void)
{
    unsigned i;
    remote_init(); assert(events==1 && wake);
    step(0); assert(armed && !delay);
    raw=2; irq=1; remote_process_wake(); assert(events==1);
    step(5); assert(delay && !armed);
    for(i=0;i<4;i++) step(5);
    assert(reports==0); /* Up is deferred: long-press may become Volume Up. */
    raw=0; for(i=0;i<5;i++) step(5);
    assert(reports==2 && armed && !delay); /* Short press: Up tap on release. */
    pending=1; wake(); assert(events==1);
    step(5); assert(delay && !armed);
    pending=0; step(5); assert(armed && !delay && pumps>0);
    puts("PASS: interrupt-to-task handoff, active debounce, idle polling stop and HID callback/retry wake");
    return 0;
}
