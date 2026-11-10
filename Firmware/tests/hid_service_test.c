/* Host-only transport mocks using real SDK declarations. Tests the actual
 * report pump, including a keyboard-only host that never enables Consumer CCC. */
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "../src/ble/hid_service.c"
const uint8_t primaryServiceUUID[] = {0x00,0x28};
const uint8_t characterUUID[] = {0x03,0x28};
const uint8_t clientCharCfgUUID[] = {0x02,0x29};
const uint8_t reportRefUUID[] = {0x08,0x29};
static uint8_t keyboard_enabled=1, consumer_enabled, notify_status, link_secure=1;
static unsigned key_packets, consumer_packets;
static uint8_t sent_keyboard[8], sent_consumer[2];
uint8_t remote_hid_secure(void) { return link_secure; }
/* Deliberately stale application context: never used for the send decision. */
uint16_t ble_manager_connection(void) { assert(0 && "Application handle must not gate HID"); return 99; }
uint8_t remote_hid_report_enabled(uint8_t id) { return id==1 ? keyboard_enabled : consumer_enabled; }
void ble_manager_state_changed(gapRole_States_t state, gapRoleEvent_t *event)
{ (void)state; (void)event; }
void HidDev_Register(hidDevCfg_t *cfg, hidDevCB_t *cb) { (void)cfg; (void)cb; }
void HidDev_RegisterReports(uint8_t count, hidRptMap_t *map)
{ assert(count==2 && map[0].id==1 && map[1].id==2); }
bStatus_t GATTServApp_RegisterService(gattAttribute_t *table, uint16_t count, uint8_t size, gattServiceCBs_t *cb)
{ unsigned i; (void)size; (void)cb; for(i=0;i<count;i++) table[i].handle=(uint16_t)(i+1); return SUCCESS; }
void GATTServApp_InitCharCfg(uint16_t handle, gattCharCfg_t *cfg) { (void)handle; (void)cfg; }
uint16_t GATTServApp_ReadCharCfg(uint16_t handle, gattCharCfg_t *cfg)
{ assert(handle==4); return (cfg==key_ccc ? keyboard_enabled : consumer_enabled) ? GATT_CLIENT_CFG_NOTIFY : 0; }
void tmos_memcpy(void *dst,const void *src,uint32_t size) { memcpy(dst,src,size); }
bStatus_t HidDev_ReadAttrCB(uint16_t conn, gattAttribute_t *attr, uint8_t *value, uint16_t *len, uint16_t offset, uint16_t max, uint8_t method)
{ (void)conn; (void)attr; (void)value; (void)len; (void)offset; (void)max; (void)method; return SUCCESS; }
bStatus_t HidDev_WriteAttrCB(uint16_t conn,gattAttribute_t *attr,uint8_t *value,uint16_t len,uint16_t offset,uint8_t method)
{ (void)conn; (void)attr; (void)value; (void)len; (void)offset; (void)method; return SUCCESS; }
uint8_t HidDev_Report(uint8_t id,uint8_t type,uint8_t len,uint8_t *data)
{
    assert(type==HID_REPORT_TYPE_INPUT);
    if (notify_status) return notify_status;
    if (!(id==1 ? keyboard_enabled : consumer_enabled)) return bleNoResources;
    if(id==1) { assert(keyboard_enabled && len==8); ++key_packets; memcpy(sent_keyboard,data,len); }
    else { assert(id==2 && consumer_enabled && len==2); ++consumer_packets; memcpy(sent_consumer,data,len); }
    return SUCCESS;
}
static unsigned wake_count;
static void on_wake(void) { ++wake_count; }
int main(void)
{
    uint16_t len=2; uint8_t ccc[]={1,0};
    assert(hid_service_init()==SUCCESS);
    assert(attrs[9].permissions==(GATT_PERMIT_READ|GATT_PERMIT_ENCRYPT_WRITE));
    assert(attrs[13].permissions==(GATT_PERMIT_READ|GATT_PERMIT_ENCRYPT_WRITE));
    hid_service_set_wake(on_wake);
    hid_service_ready(); assert(wake_count==1 && hid_service_has_pending());
    hid_service_poll(10);
    assert(key_packets==1 && consumer_packets==0 && !pending.count);
    /* Earlier code stayed forever at Consumer zero and delayed these arrows. */
    hid_service_key(2,true,20); hid_service_poll(20);
    assert(key_packets==2 && sent_keyboard[2]==0x50 && !pending.count);
    hid_service_key(2,false,25); hid_service_poll(25);
    assert(key_packets==3 && sent_keyboard[2]==0 && !pending.count);
    hid_service_key(4,true,30); hid_service_poll(30);
    assert(key_packets==4 && sent_keyboard[2]==0x4f && !pending.count);
    hid_service_key(4,false,35); hid_service_poll(35);
    /* Enable Consumer later; unrelated keyboard report must not be repeated. */
    consumer_enabled=1;
    assert(report_callback(2,1,GATT_CLIENT_CHAR_CFG_UUID,HID_DEV_OPER_ENABLE,&len,ccc)==SUCCESS);
    assert(wake_count>=2 && hid_service_has_pending());
    hid_service_poll(40); assert(key_packets==5 && consumer_packets==1);
    hid_service_key(0,true,45); hid_service_key(0,false,45); hid_service_poll(45);
    assert(key_packets==5 && consumer_packets==3 && sent_consumer[0]==0);
    /* Transient buffer failure keeps pending press and release until accepted. */
    notify_status=bleMemAllocError;
    hid_service_key(2,true,50); hid_service_poll(50);
    assert(pending.count && hid_diagnostics.retries==1);
    hid_service_key(2,false,55); notify_status=SUCCESS; hid_service_poll(55);
    assert(key_packets==7 && sent_keyboard[2]==0 && !pending.count);
    /* A consumer-only subscriber is not held up by missing keyboard CCC either. */
    keyboard_enabled=0; hid_service_ready(); hid_service_poll(60);
    hid_service_key(0,true,65); hid_service_key(0,false,65); hid_service_poll(65);
    assert(consumer_packets==6 && !pending.count);
    assert(hid_diagnostics.unsubscribed && hid_diagnostics.unchanged);
    /* Battery-restart style: CCC restored by SDK and live encryption becomes
     * ready without calling hid_service_ready from a pairing callback. */
    hid_service_disconnect(); link_secure=0;
    hid_service_poll(100); keyboard_enabled=consumer_enabled=1;
    {
        unsigned before_key=key_packets, before_consumer=consumer_packets;
        link_secure=1; hid_service_poll(110);
        assert(key_packets==before_key+1 && consumer_packets==before_consumer+1);
        hid_service_key(2,true,120); hid_service_poll(120);
        assert(sent_keyboard[2]==0x50 && key_packets==before_key+2);
        hid_service_key(2,false,130); hid_service_poll(130);
        assert(sent_keyboard[2]==0 && key_packets==before_key+3);
    }
    puts("PASS: keyboard-only/consumer-only subscriptions, no unrelated report blocking, unchanged suppression, retry/release");
    return 0;
}
