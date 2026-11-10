#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "hid_reports.h"
static hid_reports_t state;
static const uint8_t *body;
static uint8_t id, length;
static void peek(uint32_t now, uint8_t expected_id, uint8_t expected_len)
{
    assert(hid_reports_peek(&state, now, &id, &body, &length));
    assert(id == expected_id && length == expected_len);
}
static void zero_frame(uint32_t now)
{
    unsigned i;
    peek(now,1,8); for(i=0;i<8;i++) assert(body[i] == 0); hid_reports_ack(&state);
    peek(now,2,2); assert(body[0] == 0 && body[1] == 0); hid_reports_ack(&state);
}
int main(void)
{
    unsigned i;
    hid_reports_reset(&state);
    assert(!hid_reports_peek(&state,0,&id,&body,&length));
    /* Deferred Power/arrow tap arrives as press and release in one callback.
     * Verify both survive as separate snapshots and correct report types. */
    hid_reports_key(&state,0,true,10); hid_reports_key(&state,0,false,10);
    peek(10,1,8); assert(body[2]==0); hid_reports_ack(&state);
    peek(10,2,2); assert(body[0]==0x30 && body[1]==0);
    /* No ACK simulates SDK allocation/notification failure; same body retained. */
    peek(20,2,2); assert(body[0]==0x30); hid_reports_ack(&state);
    zero_frame(20); assert(!state.count);
    /* Virtual Back (AC Back 0x0224) and Home (AC Home 0x0223) taps. */
    hid_reports_key(&state,6,true,25); hid_reports_key(&state,6,false,25);
    peek(25,1,8); assert(body[2]==0); hid_reports_ack(&state);
    peek(25,2,2); assert(body[0]==0x24 && body[1]==0x02); hid_reports_ack(&state);
    zero_frame(25);
    hid_reports_key(&state,7,true,26); hid_reports_key(&state,7,false,26);
    peek(26,1,8); hid_reports_ack(&state);
    peek(26,2,2); assert(body[0]==0x23 && body[1]==0x02); hid_reports_ack(&state);
    zero_frame(26); assert(!state.count);
    /* Virtual Volume Up (0x00e9) and Volume Down (0x00ea) taps. */
    hid_reports_key(&state,8,true,27); hid_reports_key(&state,8,false,27);
    peek(27,1,8); hid_reports_ack(&state);
    peek(27,2,2); assert(body[0]==0xe9 && body[1]==0); hid_reports_ack(&state);
    zero_frame(27);
    hid_reports_key(&state,9,true,28); hid_reports_key(&state,9,false,28);
    peek(28,1,8); hid_reports_ack(&state);
    peek(28,2,2); assert(body[0]==0xea && body[1]==0); hid_reports_ack(&state);
    zero_frame(28); assert(!state.count && !state.down);
    /* Simultaneous Left/Right and independent release. */
    hid_reports_key(&state,2,true,30); hid_reports_key(&state,4,true,40);
    peek(40,1,8); assert(body[2]==0x50); hid_reports_ack(&state); hid_reports_ack(&state);
    peek(40,1,8); assert(body[2]==0x50 && body[3]==0x4f);
    hid_reports_ack(&state); hid_reports_ack(&state);
    hid_reports_key(&state,2,false,50);
    peek(50,1,8); assert(body[2]==0x4f && body[3]==0);
    /* Expiry, including after a partially transmitted frame, forces release. */
    hid_reports_ack(&state);
    zero_frame(2050); assert(state.blocked && state.dropped);
    hid_reports_key(&state,4,false,2060); assert(!state.blocked && !state.count);
    /* Overflow drops pending input, retains release and inhibits held keys. */
    hid_reports_reset(&state);
    for(i=0;i<HID_PENDING_FRAMES;i++) hid_reports_key(&state,1,(i%2)==0,0);
    hid_reports_key(&state,1,true,0);
    assert(state.blocked && state.count==1 && state.dropped==HID_PENDING_FRAMES+1);
    zero_frame(1);
    hid_reports_key(&state,1,false,2); assert(!state.blocked);
    /* Timer wrap must not expire a fresh queued key. */
    hid_reports_key(&state,3,true,UINT32_MAX-10);
    peek(5,1,8); assert(body[2]==0x28);
    hid_reports_reset(&state); assert(!state.count && !state.down && !state.phase);
    /* A failed all-up consumer notification retains its phase beyond TTL. */
    hid_reports_release(&state,0); peek(0,1,8); hid_reports_ack(&state);
    peek(10000,2,2); assert(body[0]==0 && body[1]==0);
    hid_reports_ack(&state); assert(!state.count && !state.dropped);
    puts("PASS: HID pages/encoding, press-release snapshots, retry, simultaneous keys, expiry, overflow, rollover");
    return 0;
}
