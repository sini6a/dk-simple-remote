/* Host-only SDK transport mocks, compiled against REAL vendor declarations.
 * No mock is linked into firmware. Exercise the actual manager event handler. */
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "../src/ble/ble_manager.c"
static uint32_t clock_ticks;
uint8_t battery_service_init(void) { return SUCCESS; }
void battery_service_link(uint16_t conn) { (void)conn; }
void battery_service_notify(void) { }
volatile hid_diagnostics_t hid_diagnostics;
bool hid_service_is_suspended(void) { return false; }
static uint16_t scheduled;
static uint32_t advertising_ticks;
static uint8_t stored_bonds, pair_mode, filter_policy, adv_enabled, secure;
uint8_t remote_hid_report_enabled(uint8_t id) { return secure && id==1; }
uint8_t remote_hid_subscription(uint8_t id) { return id==1; }

static uint8_t erase_status, erase_delayed, stop_requested;
static uint8_t radio_state = GAPROLE_STARTED;
static unsigned erased, terminated, param_calls, security_calls, hid_initialized;
void CH58X_BLEInit(void) { }
void HAL_Init(void) { }
bStatus_t GAPRole_PeripheralInit(void) { return SUCCESS; }
void HidDev_Init(void) { hid_initialized=1; }
uint8_t hid_service_init(void) { return SUCCESS; }
uint8_t diagnostic_service_init(void) { return SUCCESS; }
void hid_service_disconnect(void) { }
void hid_service_ready(void) { }
uint8_t remote_hid_secure(void) { return secure; }
bStatus_t GAPBondMgr_GetParameter(uint16_t param, void *value)
{
    assert(param == GAPBOND_BOND_COUNT); *(uint8_t *)value = stored_bonds; return SUCCESS;
}
bStatus_t GAPBondMgr_SetParameter(uint16_t param, uint8_t len, void *value)
{
    (void)len;
    if (param == GAPBOND_PERI_PAIRING_MODE) pair_mode = *(uint8_t *)value;
    if (param == GAPBOND_ERASE_ALLBONDS) {
        ++erased;
        if (!erase_status && !erase_delayed) stored_bonds = 0;
        return erase_status;
    }
    return SUCCESS;
}
bStatus_t GAPRole_SetParameter(uint16_t param, uint16_t len, void *value)
{
    if (param == GAPROLE_ADVERT_DATA) {
        uint8_t *data=value;
        unsigned at=0,found=0;
        assert(len<=31);
        while(at<len) {
            unsigned n=data[at];
            assert(n && at+n<len);
            if(data[at+1]==GAP_ADTYPE_16BIT_COMPLETE) {
                unsigned j;
                assert(n==5);
                for(j=at+2;j<at+n;j+=2) if(data[j]==0x0f && data[j+1]==0x18) found=1;
            }
            at+=n+1;
        }
        assert(found && at==len);
    }
    if (param == GAPROLE_ADV_FILTER_POLICY) filter_policy = *(uint8_t *)value;
    if (param == GAPROLE_ADVERT_ENABLED) {
        adv_enabled = *(uint8_t *)value;
        if (adv_enabled) { assert(!stop_requested); radio_state = GAPROLE_ADVERTISING; }
        else if (radio_state == GAPROLE_ADVERTISING) stop_requested = 1;
    }
    if (param == GAPROLE_ADVERT_DATA || param == GAPROLE_SCAN_RSP_DATA) {
        unsigned at = 0; uint8_t *p = value;
        assert(len <= 31);
        while(at < len) { assert(p[at] && at+p[at]+1 <= len); at += p[at]+1; }
        assert(at == len);
    }
    return SUCCESS;
}
bStatus_t GAPRole_GetParameter(uint16_t param, void *value)
{ assert(param == GAPROLE_STATE); *(uint8_t *)value = radio_state; return SUCCESS; }
bStatus_t GGS_SetParameter(uint8_t param, uint8_t len, void *value)
{
    if (param == GGS_DEVICE_NAME_ATT) { assert(len == strlen(APP_DEVICE_NAME)); assert(!memcmp(value,APP_DEVICE_NAME,len)); }
    else if (param == GGS_APPEARANCE_ATT) { assert(len == 2 && *(uint16_t *)value == 0x03C1); }
    else { gapPeriConnectParams_t *p=value; assert(param==GGS_PERI_CONN_PARAM_ATT && len==sizeof(*p)); assert(p->intervalMin==12 && p->intervalMax==24 && p->latency==0); }
    return SUCCESS;
}
bStatus_t GAP_SetParamValue(uint16_t param, uint16_t value)
{ (void)param; (void)value; return SUCCESS; }
bStatus_t GAPRole_PeripheralConnParamUpdateReq(uint16_t conn, uint16_t min, uint16_t max, uint16_t latency, uint16_t timeout, uint8_t id)
{ (void)conn; (void)id; assert(min==12 && max==24 && latency==30 && timeout==500); ++param_calls; return SUCCESS; }
bStatus_t GAPBondMgr_PeriSecurityReq(uint16_t handle)
{ assert(handle==4); ++security_calls; return SUCCESS; }
bStatus_t GAPRole_TerminateLink(uint16_t conn)
{ assert(conn == 4); ++terminated; return SUCCESS; }
uint32_t TMOS_GetSystemClock(void) { return clock_ticks; }
tmosTaskID TMOS_ProcessEventRegister(pTaskEventHandlerFn callback)
{ assert(hid_initialized && callback == process); return 1; }
bStatus_t tmos_set_event(tmosTaskID id, tmosEvents event)
{ assert(id == 1); scheduled |= event; return SUCCESS; }
BOOL tmos_start_task(tmosTaskID id, tmosEvents event, tmosTimer time)
{ assert(id == 1 && time); if(event==ADVERT_TIMEOUT_EVENT) advertising_ticks=time; return TRUE; }
void tmos_memcpy(void *dst, const void *src, uint32_t len) { memcpy(dst,src,len); }
bStatus_t tmos_msg_deallocate(uint8_t *msg) { (void)msg; return SUCCESS; }
uint8_t *tmos_msg_receive(tmosTaskID id) { (void)id; return NULL; }
static void event(uint16_t bit) { assert(process(1,bit) == 0); }
static void signal_state(gapRole_States_t state,gapRoleEvent_t *message)
{
    radio_state=(uint8_t)state;
    if(message->gap.opcode==GAP_END_DISCOVERABLE_DONE_EVENT && message->gap.hdr.status==SUCCESS) stop_requested=0;
    ble_manager_state_changed(state,message);
}
int main(void)
{
    gapRoleEvent_t link, ended;
    unsigned before;
    memset(&link,0,sizeof(link)); link.linkCmpl.connectionHandle=4;
    link.gap.opcode=GAP_LINK_ESTABLISHED_EVENT;
    memset(&ended,0,sizeof(ended)); ended.gap.opcode=GAP_END_DISCOVERABLE_DONE_EVENT;
    stored_bonds=1; /* Cold boot already bonded; no physical erase simulated. */
    ble_manager_init(); event(START_EVENT);
    assert(hid_initialized && boot_bonds==1 && adv_enabled);
    assert(advertising_ticks==MS1_TO_SYSTEM_TIME(APP_RECONNECT_WINDOW_MS));
    assert(pair_mode==GAPBOND_PAIRING_MODE_WAIT_FOR_REQ && filter_policy==GAP_FILTER_POLICY_ALL);
    signal_state(GAPROLE_CONNECTED,&link); secure=1;
    ble_manager_bond_event(GAPBOND_PAIRING_STATE_BONDED,SUCCESS);
    ble_manager_bond_event(GAPBOND_PAIRING_STATE_BOND_SAVED,SUCCESS);
    assert(pair_mode==GAPBOND_PAIRING_MODE_WAIT_FOR_REQ && filter_policy==GAP_FILTER_POLICY_ALL);
    before=adv_enabled; event(ACTIVITY_EVENT); event(HEARTBEAT_EVENT);
    assert(adv_enabled==before && security_calls==0); /* No app security retry loop. */
    event(PARAM_EVENT); assert(param_calls==1);
    /* Official app restarts on WAITING, not only one event opcode. */
    signal_state(GAPROLE_WAITING,&ended); secure=0;
    assert(!connected && adv_enabled && stored_bonds==1);
    signal_state(GAPROLE_CONNECTED,&link); secure=1;
    assert(connected); event(RESET_EVENT); event(RESET_PROGRESS_EVENT);
    assert(terminated==1 && erased==0 && reset_pending && !adv_enabled);
    signal_state(GAPROLE_WAITING,&ended);
    erase_status=bleNotReady; event(RESET_PROGRESS_EVENT);
    assert(reset_pending && stored_bonds==1 && !adv_enabled);
    erase_status=SUCCESS; erase_delayed=1; event(HEARTBEAT_EVENT);
    assert(reset_pending && reset_phase==2 && !adv_enabled);
    stored_bonds=0; event(RESET_PROGRESS_EVENT); assert(!reset_pending && adv_enabled);
    assert(advertising_ticks==MS1_TO_SYSTEM_TIME(APP_ADVERTISING_WINDOW_MS));
    assert(pair_mode==GAPBOND_PAIRING_MODE_WAIT_FOR_REQ);
    /* Advertising asynchronously stopping before disconnected reset. */
    stored_bonds=1; before=erased;
    event(RESET_EVENT); event(RESET_PROGRESS_EVENT); event(HEARTBEAT_EVENT);
    assert(stop_requested && !adv_enabled && erased==before);
    signal_state(GAPROLE_WAITING,&ended); erase_delayed=0; event(RESET_PROGRESS_EVENT);
    assert(!reset_pending && erased==before+1 && adv_enabled);
    /* Advancing the clock alone has no effect until TMOS delivers the timeout. */
    clock_ticks+=MS1_TO_SYSTEM_TIME(300000); event(HEARTBEAT_EVENT); assert(adv_enabled);
    event(ADVERT_TIMEOUT_EVENT); assert(!adv_enabled && quiet);
    signal_state(GAPROLE_WAITING,&ended); assert(!adv_enabled);
    event(ACTIVITY_EVENT); assert(adv_enabled && !quiet);
    signal_state(GAPROLE_CONNECTED,&link); event(ADVERT_TIMEOUT_EVENT);
    assert(connected && !quiet);
    puts("PASS: official HID-before-app init, WAIT_FOR_REQ, default filter, WAITING reconnect, SDK-owned security, safe explicit reset");
    return 0;
}
