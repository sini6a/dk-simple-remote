#ifndef APP_DIAGNOSTICS
#define APP_DIAGNOSTICS 1
#endif
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "../src/ble/battery_service.c"
const uint8_t primaryServiceUUID[]={0,0x28},characterUUID[]={3,0x28},clientCharCfgUUID[]={2,0x29};
static uint32_t ticks,scheduled,refresh_timer;
void power_manager_read_status(uint8_t out[8]) { unsigned i; for(i=0;i<8;++i) out[i]=(uint8_t)i; }
static unsigned refreshes;
static unsigned measures,notifications,frees,subscribed,secure=1,held,allocation_fail,notify_fail,valid_sample=1;
static uint16_t measured_mv=3000;
static uint8_t packet[1];
bool battery_measure_mv(uint16_t *mv) { ++measures; *mv=measured_mv; return valid_sample; }
bStatus_t GATTServApp_SendServiceChangedInd(uint16_t conn,uint8_t id) { assert(conn==7 && id==1); ++refreshes; return SUCCESS; }
uint8_t buttons_gpio_read(void) { return held; }
bool hid_service_has_pending(void) { return false; }
uint8_t remote_hid_secure(void) { return secure; }
uint32_t TMOS_GetSystemClock(void) { return ticks; }
tmosTaskID TMOS_ProcessEventRegister(pTaskEventHandlerFn cb) { assert(cb==process); return 1; }
BOOL tmos_start_task(tmosTaskID id,tmosEvents event,tmosTimer time) { assert(id==1); if(event==MEASURE_EVENT) scheduled=time; else { assert(event==REFRESH_EVENT); refresh_timer=time; } return TRUE; }
bStatus_t tmos_stop_task(tmosTaskID id,tmosEvents event) { assert(id==1 && event==MEASURE_EVENT); scheduled=0; return SUCCESS; }
uint8_t *tmos_msg_receive(tmosTaskID id) { (void)id; return NULL; }
bStatus_t tmos_msg_deallocate(uint8_t *msg) { (void)msg; return SUCCESS; }
void tmos_memcpy(void *dst,const void *src,uint32_t len) { memcpy(dst,src,len); }
void GATTServApp_InitCharCfg(uint16_t conn,gattCharCfg_t *cfg) { assert(conn==INVALID_CONNHANDLE && cfg==ccc); subscribed=0; }
uint16_t GATTServApp_ReadCharCfg(uint16_t conn,gattCharCfg_t *cfg) { assert(conn==7 && cfg==ccc); return subscribed; }
bStatus_t GATTServApp_RegisterService(gattAttribute_t *a,uint16_t count,uint8_t key,gattServiceCBs_t *cb)
{ assert(a==attrs && count==(APP_DIAGNOSTICS ? 8 : 4) && key==GATT_MAX_ENCRYPT_KEY_SIZE && cb==&callbacks); attrs[2].handle=90; return SUCCESS; }
void *GATT_bm_alloc(uint16_t conn,uint8_t op,uint16_t len,uint16_t *actual,uint8_t flag)
{ (void)actual; (void)flag; assert(conn==7 && op==ATT_HANDLE_VALUE_NOTI && len==1); return allocation_fail ? NULL : packet; }
void GATT_bm_free(gattMsg_t *msg,uint8_t op) { (void)msg; assert(op==ATT_HANDLE_VALUE_NOTI); ++frees; }
bStatus_t GATT_Notification(uint16_t conn,attHandleValueNoti_t *n,uint8_t auth)
{ (void)auth; assert(conn==7 && n->handle==90 && n->len==1 && n->pValue[0]<=100); ++notifications; return notify_fail ? bleNoResources : SUCCESS; }
bStatus_t GATTServApp_ProcessCCCWriteReq(uint16_t conn,gattAttribute_t *a,uint8_t *v,uint16_t len,uint16_t off,uint16_t allowed)
{ assert(conn==7 && a==&attrs[3] && allowed==GATT_CLIENT_CFG_NOTIFY); if(len!=2 || off) return ATT_ERR_INVALID_VALUE_SIZE; subscribed=v[0]; return SUCCESS; }
int main(void)
{
    uint8_t out[8],enable[]={1,0}; uint16_t len;
    unsigned before;
    assert(battery_service_init()==SUCCESS && measures==1 && !scheduled);
    assert(level==100 && attrs[3].permissions==(GATT_PERMIT_READ|GATT_PERMIT_ENCRYPT_WRITE));
    before=measures;
    assert(read_value(7,&attrs[2],out,&len,0,4,0)==SUCCESS && len==1 && out[0]==100);
#if APP_DIAGNOSTICS
    assert(read_value(7,&attrs[5],out,&len,0,4,0)==SUCCESS && len==2 && BUILD_UINT16(out[0],out[1])==3000);
#endif
    assert(read_value(7,&attrs[2],out,&len,2,4,0)==ATT_ERR_INVALID_OFFSET && measures==before);
    battery_service_link(7); assert(scheduled==PERIOD_TICKS);
    assert(write_value(7,&attrs[3],enable,2,0,0)==SUCCESS && notifications==1);
    assert(write_value(7,&attrs[2],enable,2,0,0)==ATT_ERR_WRITE_NOT_PERMITTED);
    assert(refresh_timer==MS1_TO_SYSTEM_TIME(1000));
    process(1,REFRESH_EVENT); assert(refreshes==1 && battery_service_refresh_status==SUCCESS);
    refresh_timer=0; subscribed=0; battery_service_notify(); assert(!refresh_timer); subscribed=1;
    secure=0; battery_service_notify(); assert(notifications==1); secure=1;
    allocation_fail=1; battery_service_notify(); assert(notifications==1); allocation_fail=0;
    notify_fail=1; battery_service_notify(); assert(notifications==2 && frees==1); notify_fail=0;
    held=1; process(1,MEASURE_EVENT); assert(measures==before && scheduled==MS1_TO_SYSTEM_TIME(30000));
    /* 12h period, computed without 32-bit overflow. */
    assert(PERIOD_TICKS==69120000UL && SETTLE_TICKS==MS1_TO_SYSTEM_TIME(10000));
    /* A burst takes APP_BATTERY_SAMPLES readings and commits the highest. */
    held=0; ticks=PERIOD_TICKS;
    measured_mv=2900; process(1,MEASURE_EVENT); assert(measures==before+1 && scheduled==SPACING_TICKS);
    measured_mv=3010; process(1,MEASURE_EVENT); assert(scheduled==SPACING_TICKS);
    measured_mv=2950; process(1,MEASURE_EVENT); assert(measures==before+3 && scheduled==PERIOD_TICKS);
    assert(level==100 && estimate.millivolts==3010 && last_sample==PERIOD_TICKS);
#if APP_DIAGNOSTICS
    assert(read_value(7,&attrs[5],out,&len,0,4,0)==SUCCESS && BUILD_UINT16(out[0],out[1])==3010);
#endif
    measured_mv=3000;
    battery_service_link(INVALID_CONNHANDLE); assert(!scheduled && !subscribed);
    process(1,MEASURE_EVENT); assert(measures==before+3 && !scheduled);
    /* Reconnecting resumes the remaining time instead of restarting 12h. */
    ticks+=1000; battery_service_link(7); assert(scheduled==PERIOD_TICKS-1000);
    ticks+=PERIOD_TICKS-1000-SETTLE_TICKS; battery_service_link(7); assert(scheduled==SETTLE_TICKS);
    /* A reconnect mid-burst restarts the burst from scratch. */
    process(1,MEASURE_EVENT); assert(burst_count==1);
    battery_service_link(7); assert(burst_count==0 && scheduled==SETTLE_TICKS);
    ticks+=PERIOD_TICKS+1; battery_service_link(7); assert(scheduled==SETTLE_TICKS);
    valid_sample=0; before=measures;
    process(1,MEASURE_EVENT); process(1,MEASURE_EVENT); process(1,MEASURE_EVENT);
    assert(measures==before+3 && estimate.valid && level==100 && scheduled==PERIOD_TICKS);
    valid_sample=1;
#if APP_DIAGNOSTICS
    assert(read_value(7,&attrs[7],out,&len,0,8,0)==SUCCESS && len==8 && out[3]==3);
#endif
    estimate.valid=false;
#if APP_DIAGNOSTICS
    assert(read_value(7,&attrs[7],out,&len,0,8,0)==SUCCESS && len==8);
#endif
    assert(read_value(7,&attrs[2],out,&len,0,4,0)==ATT_ERR_UNLIKELY);
    puts("PASS: cached BAS/voltage reads, CCC/security, notification allocation cleanup, 12h best-of-burst connected sampling, reconnect resume and disconnected timer shutdown");
    return 0;
}
