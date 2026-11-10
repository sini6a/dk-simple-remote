/* Lifecycle follows official HID_Consumer: GAP/HID init before app task,
 * WAIT_FOR_REQ bonding, default open connection filter and advertising on WAITING.
 * Optional demo services are omitted; report/GPIO and explicit reset are app code. */
#include "CONFIG.h"
#include "HAL.h"
#include "ble_manager.h"
#include "app_config.h"
#include "hiddev.h"
#include "hid_service.h"
#include "diagnostic_service.h"
#include "battery_service.h"
#define START_EVENT 0x0001
#define HEARTBEAT_EVENT 0x0002
#define ACTIVITY_EVENT 0x0004
#define RESET_EVENT 0x0008
#define PARAM_EVENT 0x0010
#define RESET_PROGRESS_EVENT 0x0020
#define ADVERT_TIMEOUT_EVENT 0x0040
__attribute__((aligned(4))) uint32_t MEM_BUF[BLE_MEMHEAP_SIZE / 4];
volatile ble_diagnostics_t ble_diagnostics;
static uint8_t task, buttons, connected, reset_pending, advert_busy, reset_phase;
static uint8_t security_status, snapshot_valid, snapshot_flags, snapshot_bonds;
static uint8_t last_session[13], last_session_valid, session_observed, boot_bonds;
static uint8_t quiet;
static uint8_t security_attempts; /* Always zero: SDK owns security procedures. */
static uint16_t transitions;
static uint16_t connection = GAP_CONNHANDLE_INIT;
static uint8_t device_name[] = APP_DEVICE_NAME;
typedef char name_must_fit_advert[(sizeof(APP_DEVICE_NAME) <= 17) ? 1 : -1];
/* Apple accessory guidelines: latency <= 30; max interval x (latency+1) <= 2s;
 * timeout 2-6s and > 3 x max interval x (latency+1); min >= 15ms, max >= min+15ms.
 * Intervals are 1.25ms units and the timeout 10ms units. */
typedef char conn_params_within_host_limits[(APP_CONN_LATENCY <= 30U &&
    APP_CONN_MIN >= 12U && APP_CONN_MAX >= APP_CONN_MIN + 12U &&
    APP_CONN_MAX * (APP_CONN_LATENCY + 1U) * 5U <= 8000U &&
    APP_CONN_TIMEOUT >= 200U && APP_CONN_TIMEOUT <= 600U &&
    APP_CONN_TIMEOUT * 8U > APP_CONN_MAX * (APP_CONN_LATENCY + 1U) * 3U) ? 1 : -1];
static uint8_t advert[14 + sizeof(APP_DEVICE_NAME)];
static uint8_t bond_count(void)
{
    uint8_t count=0;
    GAPBondMgr_GetParameter(GAPBOND_BOND_COUNT,&count);
    return count;
}
uint16_t ble_manager_connection(void) { return connection; }
void ble_manager_param_updated(uint16_t handle,uint16_t interval,uint16_t latency,uint16_t timeout)
{
    if(handle!=connection) return;
    ble_diagnostics.conn_interval=interval; ble_diagnostics.conn_latency=latency;
    ble_diagnostics.conn_timeout=timeout;
}
static uint8_t scan_response[] = {
    30, GAP_ADTYPE_SERVICE_DATA_128BIT,
    0x01,0x01,0xb6,0x83,0x05,0xcb,0x61,0x8e,
    0x52,0x4d,0x7b,0x3a,0x90,0x1f,0xac,0x64,
    2, 0,0,0,0, 0, 0,0, 0,0,0,0,0
};
void ble_manager_read_diagnostics(uint8_t out[32])
{
    uint32_t beat = ble_diagnostics.heartbeat;
    uint8_t *live = &out[15];
    out[0] = 3; out[1] = last_session_valid;
    tmos_memcpy(&out[2], last_session, sizeof(last_session));
    live[0] = 2;
    live[1] = (uint8_t)beat; live[2] = (uint8_t)(beat >> 8);
    live[3] = (uint8_t)(beat >> 16); live[4] = (uint8_t)(beat >> 24);
    live[5] = buttons; live[6] = (uint8_t)transitions; live[7] = (uint8_t)(transitions >> 8);
    live[8] = (connected ? 1 : 0) | (remote_hid_secure() ? 2 : 0) |
        (remote_hid_subscription(1) ? 4 : 0) | (remote_hid_subscription(2) ? 8 : 0) |
        (hid_service_is_suspended() ? 16 : 0) | (reset_pending ? 32 : 0);
    live[9] = bond_count(); live[10] = hid_diagnostics.last_status;
    live[11] = security_status; live[12] = (uint8_t)hid_diagnostics.accepted;
    out[28] = boot_bonds; out[29] = security_attempts;
    out[30] = ble_diagnostics.last_status; out[31] = ble_diagnostics.role_state;
}

static void check(uint8_t status)
{
    if(status!=SUCCESS) { ble_diagnostics.last_status=status; ++ble_diagnostics.errors; }
}
static void advertising(uint8_t enabled)
{
    uint8_t status=GAPRole_SetParameter(GAPROLE_ADVERT_ENABLED,sizeof(enabled),&enabled);
    check(status);
    if(status==SUCCESS && enabled) {
        advert_busy=TRUE; quiet=FALSE;
        tmos_start_task(task,ADVERT_TIMEOUT_EVENT,MS1_TO_SYSTEM_TIME(bond_count() ? APP_RECONNECT_WINDOW_MS : APP_ADVERTISING_WINDOW_MS));
    }
}
void ble_manager_state_changed(gapRole_States_t state,gapRoleEvent_t *event)
{
    uint8_t base=state & GAPROLE_STATE_ADV_MASK;
    ble_diagnostics.role_state=state;
    if(event->gap.opcode==GAP_END_DISCOVERABLE_DONE_EVENT && event->gap.hdr.status==SUCCESS) {
        advert_busy=FALSE;
        if(reset_pending) tmos_set_event(task,RESET_PROGRESS_EVENT);
    }
    if(base==GAPROLE_CONNECTED && !connected) {
        /* Same connection-event layout used by the official application. */
        gapEstLinkReqEvent_t *link=(gapEstLinkReqEvent_t *)event;
        connected=TRUE; connection=link->connectionHandle;
        advert_busy=FALSE; session_observed=0;
        hid_service_disconnect();
        battery_service_link(connection);
        ble_manager_param_updated(connection,link->connInterval,link->connLatency,link->connTimeout);
        if(reset_pending) check(GAPRole_TerminateLink(connection));
        else tmos_start_task(task,PARAM_EVENT,12800); /* Official HID update delay: 8s. */
    } else if(base==GAPROLE_WAITING || base==GAPROLE_STARTED) {
        if(connected) {
            if(session_observed) { tmos_memcpy(last_session,&scan_response[18],13); last_session_valid=1; }
            connected=FALSE; connection=GAP_CONNHANDLE_INIT;
            battery_service_link(INVALID_CONNHANDLE);
            hid_service_disconnect();
        }
        if(reset_pending) tmos_set_event(task,RESET_PROGRESS_EVENT);
        else if(base==GAPROLE_WAITING && !quiet) advertising(TRUE); /* Official HidEmu state callback. */
    }
}
void ble_manager_bond_event(uint8_t state,uint8_t status)
{
    if(reset_pending) return;
    check(status);
    if(status==SUCCESS && (state==GAPBOND_PAIRING_STATE_COMPLETE || state==GAPBOND_PAIRING_STATE_BONDED))
        { hid_service_ready(); battery_service_notify(); }
    /* Do not rewrite pairing mode or advertising filter after BOND_SAVED. */
}
void ble_manager_pairing_reset(void) { tmos_set_event(task,RESET_EVENT); }
static void finish_reset(void)
{
    uint8_t status, remaining=255;
    if(connected || advert_busy) return;
    if(reset_phase==1) {
        status=GAPBondMgr_SetParameter(GAPBOND_ERASE_ALLBONDS,0,NULL); check(status);
        if(status!=SUCCESS) return;
        reset_phase=2;
    }
    status=GAPBondMgr_GetParameter(GAPBOND_BOND_COUNT,&remaining); check(status);
    if(status==SUCCESS && remaining==0) {
        reset_pending=FALSE; reset_phase=0; advertising(TRUE);
    }
}
static uint16_t process(uint8_t id,uint16_t events)
{
    if(events & SYS_EVENT_MSG) {
        uint8_t *message;
        while((message=tmos_msg_receive(id))!=NULL) tmos_msg_deallocate(message);
        return events ^ SYS_EVENT_MSG;
    }
    if(events & START_EVENT) {
        boot_bonds=bond_count();
        if(APP_DIAGNOSTICS || reset_pending)
            tmos_start_task(task,HEARTBEAT_EVENT,MS1_TO_SYSTEM_TIME(1000));
        return events ^ START_EVENT;
    }
    if(events & ADVERT_TIMEOUT_EVENT) {
        if(!connected && !reset_pending) {
            quiet=TRUE;
            advertising(FALSE);
        }
        return events ^ ADVERT_TIMEOUT_EVENT;
    }
    if(events & ACTIVITY_EVENT) {
        if(!connected && !reset_pending) advertising(TRUE);
        return events ^ ACTIVITY_EVENT;
    }
    if(events & PARAM_EVENT) {
        if(connected && !reset_pending) {
            ++ble_diagnostics.param_requests;
            check(GAPRole_PeripheralConnParamUpdateReq(connection,APP_CONN_MIN,APP_CONN_MAX,APP_CONN_LATENCY,APP_CONN_TIMEOUT,task));
        }
        return events ^ PARAM_EVENT;
    }
    if(events & RESET_EVENT) {
        if(!reset_pending) {
            reset_pending=TRUE; reset_phase=1;
            tmos_start_task(task,HEARTBEAT_EVENT,MS1_TO_SYSTEM_TIME(1000));
            advertising(FALSE); hid_service_disconnect();
            if(connected) check(GAPRole_TerminateLink(connection));
            tmos_set_event(task,RESET_PROGRESS_EVENT);
        }
        return events ^ RESET_EVENT;
    }
    if(events & RESET_PROGRESS_EVENT) {
        if(reset_pending) finish_reset();
        return events ^ RESET_PROGRESS_EVENT;
    }
    if(events & HEARTBEAT_EVENT) {
        uint32_t beat=++ble_diagnostics.heartbeat;
        if(reset_pending) {
            gapRole_States_t role=GAPROLE_INIT;
            uint8_t status=GAPRole_GetParameter(GAPROLE_STATE,&role); check(status);
            if(status==SUCCESS && ((role & GAPROLE_STATE_ADV_MASK)==GAPROLE_WAITING ||
                                  (role & GAPROLE_STATE_ADV_MASK)==GAPROLE_STARTED)) advert_busy=FALSE;
            if(connected) check(GAPRole_TerminateLink(connection));
            else if(advert_busy) advertising(FALSE);
            finish_reset();
        }
        scan_response[19]=(uint8_t)beat; scan_response[20]=(uint8_t)(beat>>8);
        scan_response[21]=(uint8_t)(beat>>16); scan_response[22]=(uint8_t)(beat>>24);
        scan_response[23]=buttons; scan_response[24]=(uint8_t)transitions; scan_response[25]=(uint8_t)(transitions>>8);
        if(connected || !snapshot_valid) {
            snapshot_flags=(connected?1:0) | (remote_hid_secure()?2:0) |
                (remote_hid_subscription(1)?4:0) | (remote_hid_subscription(2)?8:0) |
                (hid_service_is_suspended()?16:0) | (reset_pending?32:0);
            snapshot_bonds=bond_count(); snapshot_valid=connected;
            if(connected) session_observed=1;
            scan_response[28]=hid_diagnostics.last_status; scan_response[29]=security_status;
            scan_response[30]=(uint8_t)hid_diagnostics.accepted;
        }
        scan_response[26]=snapshot_flags | (!connected && snapshot_valid ? 0x80:0);
        scan_response[27]=snapshot_bonds;
        /* Official examples set AD data before role start. Never update it or
         * request advertising on a connected link. GATT diagnostics remain readable. */
        if(!connected && !reset_pending)
            check(GAPRole_SetParameter(GAPROLE_SCAN_RSP_DATA,sizeof(scan_response),scan_response));
        if(APP_DIAGNOSTICS || reset_pending)
            tmos_start_task(task,HEARTBEAT_EVENT,MS1_TO_SYSTEM_TIME(1000));
        return events ^ HEARTBEAT_EVENT;
    }
    return 0;
}
void ble_manager_buttons(uint8_t down)
{
    if(down!=buttons) ++transitions;
    if(down & (uint8_t)~buttons) tmos_set_event(task,ACTIVITY_EVENT);
    buttons=down;
}
void ble_manager_init(void)
{
    uint8_t enabled=TRUE, disabled=FALSE, mode=GAPBOND_PAIRING_MODE_WAIT_FOR_REQ;
    uint8_t io=GAPBOND_IO_CAP_NO_INPUT_NO_OUTPUT, filter=GAP_FILTER_POLICY_ALL;
    uint32_t passcode=0;
    uint16_t minimum=APP_CONN_MIN, maximum=APP_CONN_MAX, appearance=APP_APPEARANCE;
    unsigned pos=5+sizeof(APP_DEVICE_NAME)-1;
    CH58X_BLEInit(); HAL_Init(); check(GAPRole_PeripheralInit());
    HidDev_Init(); /* Official main: initialize HID task BEFORE application task. */
    task=TMOS_ProcessEventRegister(process);
    if(task==INVALID_TASK_ID) while(1) { }
    check(hid_service_init()); check(diagnostic_service_init());
    check(battery_service_init());
    check(GAPRole_SetParameter(GAPROLE_MIN_CONN_INTERVAL,sizeof(minimum),&minimum));
    check(GAPRole_SetParameter(GAPROLE_MAX_CONN_INTERVAL,sizeof(maximum),&maximum));
    check(GGS_SetParameter(GGS_DEVICE_NAME_ATT,sizeof(device_name)-1,device_name));
    /* Same value in GATT as in the advertisement; SDK default is 0 (Unknown). */
    check(GGS_SetParameter(GGS_APPEARANCE_ATT,sizeof(appearance),&appearance));
    check(GAPBondMgr_SetParameter(GAPBOND_PERI_DEFAULT_PASSCODE,sizeof(passcode),&passcode));
    check(GAPBondMgr_SetParameter(GAPBOND_PERI_PAIRING_MODE,1,&mode));
    check(GAPBondMgr_SetParameter(GAPBOND_PERI_MITM_PROTECTION,1,&disabled));
    check(GAPBondMgr_SetParameter(GAPBOND_PERI_IO_CAPABILITIES,1,&io));
    check(GAPBondMgr_SetParameter(GAPBOND_PERI_BONDING_ENABLED,1,&enabled));
    /* SDK-default auto persistence, no custom security loop/RL/filter switching.
     * Keep automatic eviction disabled: existing bonds change only via the chord. */
    check(GAPBondMgr_SetParameter(GAPBOND_ERASE_AUTO,1,&disabled));
    check(GAPRole_SetParameter(GAPROLE_ADV_FILTER_POLICY,1,&filter));
    advert[0]=2; advert[1]=GAP_ADTYPE_FLAGS;
    advert[2]=GAP_ADTYPE_FLAGS_GENERAL | GAP_ADTYPE_FLAGS_BREDR_NOT_SUPPORTED;
    advert[3]=sizeof(APP_DEVICE_NAME); advert[4]=GAP_ADTYPE_LOCAL_NAME_COMPLETE;
    tmos_memcpy(&advert[5],device_name,sizeof(device_name)-1);
    advert[pos++]=3; advert[pos++]=GAP_ADTYPE_APPEARANCE;
    advert[pos++]=LO_UINT16(APP_APPEARANCE); advert[pos++]=HI_UINT16(APP_APPEARANCE);
    advert[pos++]=5; advert[pos++]=GAP_ADTYPE_16BIT_COMPLETE;
    advert[pos++]=LO_UINT16(HID_SERV_UUID); advert[pos++]=HI_UINT16(HID_SERV_UUID);
    advert[pos++]=0x0f; advert[pos++]=0x18; /* Battery Service discovery hint. */
    check(GAPRole_SetParameter(GAPROLE_ADVERT_DATA,sizeof(advert),advert));
    check(GAPRole_SetParameter(GAPROLE_SCAN_RSP_DATA,sizeof(scan_response),scan_response));
    check(GAP_SetParamValue(TGAP_DISC_ADV_INT_MIN,APP_ADVERTISING_INTERVAL));
    check(GAP_SetParamValue(TGAP_DISC_ADV_INT_MAX,APP_ADVERTISING_INTERVAL));
    advertising(TRUE); /* Official application enables before deferred role start. */
    tmos_set_event(task,START_EVENT);
}
