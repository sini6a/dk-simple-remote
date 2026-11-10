/* GATT declaration, CCC and notification ownership follow WCH battservice.c.
 * Real ADC readings replace the example's hardcoded adc=300 placeholder.
 * Appended after HID/diagnostics; no HID handles or report maps are moved. */
#include "CONFIG.h"
#include "app_config.h"
#include "battery.h"
#include "battery_service.h"
#include "buttons.h"
#include "hid_service.h"
#include "power_manager.h"
#define MEASURE_EVENT 1
#define REFRESH_EVENT 2
/* 625us ticks. MS1_TO_SYSTEM_TIME multiplies by 1000 in 32 bits and would
 * overflow for 12 hours; ms/5*8 is exact for 5ms multiples. */
#define BATTERY_TICKS(ms) ((uint32_t)((ms)/5UL*8UL))
#define PERIOD_TICKS BATTERY_TICKS(APP_BATTERY_PERIOD_MS)
#define SETTLE_TICKS BATTERY_TICKS(APP_BATTERY_SETTLE_MS)
#define SPACING_TICKS BATTERY_TICKS(APP_BATTERY_SPACING_MS)
#define BUSY_TICKS BATTERY_TICKS(30000UL)
/* TMOS allows timers up to one RTC day (138240000 ticks) minus a margin. */
typedef char battery_period_fits_tmos[(APP_BATTERY_PERIOD_MS % 5UL == 0 &&
    APP_BATTERY_SETTLE_MS % 5UL == 0 && APP_BATTERY_SPACING_MS % 5UL == 0 &&
    APP_BATTERY_PERIOD_MS <= 72000000UL && APP_BATTERY_SAMPLES >= 1U) ? 1 : -1];
static battery_estimate_t estimate;
static uint8_t task, level, voltage[2], burst_count;
static uint16_t burst_best;
static uint32_t last_sample;
static bool refresh_requested;
volatile uint8_t battery_service_refresh_status=0xff;
static uint16_t connection=INVALID_CONNHANDLE;
static gattCharCfg_t ccc[GATT_MAX_NUM_CONN];
static const uint8_t service_uuid[]={0x0f,0x18}, level_uuid[]={0x19,0x2a};
static const gattAttrType_t service={ATT_BT_UUID_SIZE,service_uuid};
static uint8_t props=GATT_PROP_READ|GATT_PROP_NOTIFY;
#if APP_DIAGNOSTICS
static uint8_t voltage_props=GATT_PROP_READ;
static uint8_t power_status[8];
static const uint8_t power_uuid[]={0x04,0x02,0xb6,0x83,0x05,0xcb,0x61,0x8e,0x52,0x4d,0x7b,0x3a,0x90,0x1f,0xac,0x64};
static const uint8_t voltage_uuid[]={0x03,0x02,0xb6,0x83,0x05,0xcb,0x61,0x8e,0x52,0x4d,0x7b,0x3a,0x90,0x1f,0xac,0x64};
#endif
static gattAttribute_t attrs[]={
    {{ATT_BT_UUID_SIZE,primaryServiceUUID},GATT_PERMIT_READ,0,(uint8_t *)&service},
    {{ATT_BT_UUID_SIZE,characterUUID},GATT_PERMIT_READ,0,&props},
    {{ATT_BT_UUID_SIZE,level_uuid},GATT_PERMIT_READ,0,&level},
    {{ATT_BT_UUID_SIZE,clientCharCfgUUID},GATT_PERMIT_READ|GATT_PERMIT_ENCRYPT_WRITE,0,(uint8_t *)ccc},
#if APP_DIAGNOSTICS
    {{ATT_BT_UUID_SIZE,characterUUID},GATT_PERMIT_READ,0,&voltage_props},
    {{16,voltage_uuid},GATT_PERMIT_READ,0,voltage},
    {{ATT_BT_UUID_SIZE,characterUUID},GATT_PERMIT_READ,0,&voltage_props},
    {{16,power_uuid},GATT_PERMIT_READ,0,power_status},
#endif
};
static bStatus_t read_value(uint16_t conn,gattAttribute_t *attr,uint8_t *out,
                           uint16_t *len,uint16_t offset,uint16_t max,uint8_t method)
{
    uint16_t size;
    (void)conn; (void)method;
#if APP_DIAGNOSTICS
    if(attr->pValue==power_status) {
        if(offset==0) power_manager_read_status(power_status);
        size=sizeof(power_status);
    } else
#endif
    {
        if(attr->pValue!=&level && attr->pValue!=voltage) return ATT_ERR_ATTR_NOT_FOUND;
        if(!estimate.valid) return ATT_ERR_UNLIKELY;
        size=attr->pValue==&level ? 1 : 2;
    }
    if(offset>size) return ATT_ERR_INVALID_OFFSET;
    *len=MIN(max,size-offset); tmos_memcpy(out,attr->pValue+offset,*len);
    return SUCCESS; /* Cached: a read never starts the ADC. */
}
void battery_service_notify(void)
{
    attHandleValueNoti_t noti;
    if(connection==INVALID_CONNHANDLE || !remote_hid_secure()) return;
    /* Firmware added BAS to an existing database. Ask an already bonded host
     * to rediscover it once per boot, after encryption, without erasing bonds.
     * Hosts that did not subscribe to Service Changed need a manual refresh. */
    if(!refresh_requested) {
        refresh_requested=true;
        tmos_start_task(task,REFRESH_EVENT,MS1_TO_SYSTEM_TIME(1000));
    }
    if(!estimate.valid || !(GATTServApp_ReadCharCfg(connection,ccc)&GATT_CLIENT_CFG_NOTIFY)) return;
    noti.pValue=GATT_bm_alloc(connection,ATT_HANDLE_VALUE_NOTI,1,NULL,0);
    if(!noti.pValue) return; /* No wake/retry loop for nonessential telemetry. */
    noti.handle=attrs[2].handle; noti.len=1; noti.pValue[0]=level;
    if(GATT_Notification(connection,&noti,FALSE)!=SUCCESS)
        GATT_bm_free((gattMsg_t *)&noti,ATT_HANDLE_VALUE_NOTI);
}
static bStatus_t write_value(uint16_t conn,gattAttribute_t *attr,uint8_t *data,
                            uint16_t len,uint16_t offset,uint8_t method)
{
    bStatus_t status;
    (void)method;
    if(attr->pValue!=(uint8_t *)ccc) return ATT_ERR_WRITE_NOT_PERMITTED;
    status=GATTServApp_ProcessCCCWriteReq(conn,attr,data,len,offset,GATT_CLIENT_CFG_NOTIFY);
    if(status==SUCCESS) battery_service_notify();
    return status;
}
static gattServiceCBs_t callbacks={read_value,write_value,NULL};
static void commit(uint16_t mv)
{
    bool changed=battery_estimate_update(&estimate,mv);
    level=estimate.percent;
    voltage[0]=(uint8_t)mv;
    voltage[1]=(uint8_t)(mv>>8);
    last_sample=TMOS_GetSystemClock();
    if(changed) battery_service_notify();
}
static void measure(void)
{
    uint16_t mv;
    if(battery_measure_mv(&mv)) commit(mv);
}
/* One reading of a connected burst; returns the delay to the next event. */
static uint32_t burst_step(void)
{
    uint16_t mv;
    if(battery_measure_mv(&mv) && mv>burst_best) burst_best=mv;
    if(++burst_count<APP_BATTERY_SAMPLES) return SPACING_TICKS;
    if(burst_best) commit(burst_best); /* Failed burst: keep the last value. */
    burst_count=0; burst_best=0;
    return PERIOD_TICKS;
}
static uint16_t process(uint8_t id,uint16_t events)
{
    if(events & SYS_EVENT_MSG) {
        uint8_t *msg;
        while((msg=tmos_msg_receive(id))!=NULL) tmos_msg_deallocate(msg);
        return events^SYS_EVENT_MSG;
    }
    if(events&REFRESH_EVENT) {
        if(connection!=INVALID_CONNHANDLE && remote_hid_secure())
            battery_service_refresh_status=GATTServApp_SendServiceChangedInd(connection,task);
        return events^REFRESH_EVENT;
    }
    if(events&MEASURE_EVENT) {
        if(connection!=INVALID_CONNHANDLE) {
            if(buttons_gpio_read() || hid_service_has_pending())
                tmos_start_task(task,MEASURE_EVENT,BUSY_TICKS);
            else tmos_start_task(task,MEASURE_EVENT,burst_step());
        }
        return events^MEASURE_EVENT;
    }
    return 0;
}
void battery_service_link(uint16_t conn)
{
    uint32_t elapsed=(uint32_t)(TMOS_GetSystemClock()-last_sample);
    connection=conn;
    burst_count=0; burst_best=0;
    tmos_stop_task(task,MEASURE_EVENT);
    if(conn==INVALID_CONNHANDLE) GATTServApp_InitCharCfg(INVALID_CONNHANDLE,ccc);
    /* Resume the 12h schedule across reconnects instead of restarting it, so
     * short connections still get measured; never sooner than the settle time. */
    else tmos_start_task(task,MEASURE_EVENT,
        !estimate.valid || elapsed>=PERIOD_TICKS-SETTLE_TICKS ?
        SETTLE_TICKS : PERIOD_TICKS-elapsed);
}
uint8_t battery_service_init(void)
{
    task=TMOS_ProcessEventRegister(process);
    if(task==INVALID_TASK_ID) while(1) { }
    GATTServApp_InitCharCfg(INVALID_CONNHANDLE,ccc);
    measure(); /* Radio still off: a quiet reading. No disconnected timer. */
    return GATTServApp_RegisterService(attrs,GATT_NUM_ATTRS(attrs),GATT_MAX_ENCRYPT_KEY_SIZE,&callbacks);
}
