/* Exercise the actual adapted vendor send/query functions. Only allocation,
 * registered-attribute lookup and radio delivery are host mocks. */
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "../src/ble/wch_hiddev.c"
uint16_t hidReportMapLen;
uint8_t hidProtocolMode = HID_PROTOCOL_MODE_REPORT;
static gattCharCfg_t keyboard_cfg[GATT_MAX_NUM_CONN], consumer_cfg[GATT_MAX_NUM_CONN];
static gattAttribute_t registered_keyboard, registered_consumer;
static uint8_t buffer[8], notification_status, allocation_failure;
static unsigned notifications, frees;
static uint8_t encrypted = 1, physically_connected = 1;
static uint16_t active_handle=7;
uint8_t linkDB_State(uint16_t handle,uint8_t requested)
{ assert(handle==active_handle); assert(requested==LINK_CONNECTED || requested==LINK_ENCRYPTED);
  return requested==LINK_CONNECTED ? physically_connected : encrypted; }
gattAttribute_t *GATT_FindHandle(uint16_t handle, uint16_t *returned)
{
    *returned=handle;
    if(handle==11) return &registered_keyboard;
    if(handle==21) return &registered_consumer;
    return NULL;
}
uint16_t GATTServApp_ReadCharCfg(uint16_t handle,gattCharCfg_t *table)
{
    unsigned i;
    for(i=0;i<GATT_MAX_NUM_CONN;i++) if(table[i].connHandle==handle) return table[i].value;
    return 0;
}
void *GATT_bm_alloc(uint16_t handle,uint8_t opcode,uint16_t size,uint16_t *actual,uint8_t flag)
{ (void)opcode; (void)actual; (void)flag; assert(handle==active_handle && size<=sizeof(buffer)); return allocation_failure ? NULL : buffer; }
void GATT_bm_free(gattMsg_t *msg,uint8_t opcode) { (void)msg; (void)opcode; ++frees; }
bStatus_t GATT_Notification(uint16_t handle,attHandleValueNoti_t *message,uint8_t authenticated)
{ (void)authenticated; assert(handle==active_handle && message->pValue==buffer); ++notifications; return notification_status; }
void tmos_memcpy(void *dst,const void *src,uint32_t len) { memcpy(dst,src,len); }
/* Unused lifecycle paths still need link definitions on Windows PE. Any
 * unexpected invocation fails, rather than pretending that a stack ran. */
void ble_manager_param_updated(uint16_t a,uint16_t b,uint16_t c,uint16_t d) { assert(0); }
void ble_manager_bond_event(uint8_t a,uint8_t b)
{ if(b==SUCCESS && (a==GAPBOND_PAIRING_STATE_COMPLETE || a==GAPBOND_PAIRING_STATE_BONDED)) assert(remote_hid_secure()); }
uint8_t *tmos_msg_receive(tmosTaskID id) { assert(0); return NULL; }
bStatus_t tmos_msg_deallocate(uint8_t *p) { assert(0); return FAILURE; }
bStatus_t GAPRole_PeripheralStartDevice(uint8_t id,gapBondCBs_t *a,gapRolesCBs_t *b) { assert(0); return FAILURE; }
bStatus_t GAPBondMgr_PasscodeRsp(uint16_t h,uint8_t s,uint32_t code) { assert(0); return FAILURE; }
bStatus_t GAPRole_SetParameter(uint16_t p,uint16_t len,void *v) { (void)len; (void)v; assert(p==GAPROLE_ADVERT_ENABLED); return SUCCESS; }
bStatus_t GAP_SetParamValue(uint16_t p,uint16_t v) { (void)p; (void)v; return SUCCESS; }
bStatus_t GAPBondMgr_GetParameter(uint16_t p,void *v) { assert(p==GAPBOND_BOND_COUNT); *(uint8_t *)v=1; return SUCCESS; }
void GATTServApp_InitCharCfg(uint16_t h,gattCharCfg_t *v)
{ unsigned i; for(i=0;i<GATT_MAX_NUM_CONN;i++) if(v[i].connHandle==h) { v[i].connHandle=INVALID_CONNHANDLE; v[i].value=0; } }
tmosTaskID TMOS_ProcessEventRegister(pTaskEventHandlerFn cb) { assert(0); return INVALID_TASK_ID; }
bStatus_t GGS_AddService(uint32_t services) { assert(0); return FAILURE; }
bStatus_t GATTServApp_AddService(uint32_t services) { assert(0); return FAILURE; }
bStatus_t DevInfo_AddService(void) { assert(0); return FAILURE; }
bStatus_t tmos_set_event(tmosTaskID id,tmosEvents event) { assert(0); return FAILURE; }
bStatus_t GAPRole_TerminateLink(uint16_t h) { assert(0); return FAILURE; }
bStatus_t GAPBondMgr_SetParameter(uint16_t p,uint8_t l,void *v) { assert(0); return FAILURE; }
bStatus_t GATTServApp_ProcessCCCWriteReq(uint16_t h,gattAttribute_t *a,uint8_t *v,uint16_t l,uint16_t o,uint16_t c) { assert(0); return FAILURE; }
int main(void)
{
    unsigned i;
    uint8_t input[8]={0,0,0x50,0,0,0,0,0};
    gapRoleEvent_t link, ended;
    hidDevCfg_t cfg={0,HID_FLAGS_REMOTE_WAKE};
    hidRptMap_t map[2]={{10,11,1,HID_REPORT_TYPE_INPUT,HID_PROTOCOL_MODE_REPORT},
                        {20,21,2,HID_REPORT_TYPE_INPUT,HID_PROTOCOL_MODE_REPORT}};
    memset(&link,0,sizeof(link)); link.linkCmpl.connectionHandle=active_handle;
    link.gap.opcode=GAP_LINK_ESTABLISHED_EVENT;
    memset(&ended,0,sizeof(ended)); ended.gap.opcode=GAP_LINK_TERMINATED_EVENT;
    for(i=0;i<GATT_MAX_NUM_CONN;i++) { keyboard_cfg[i].connHandle=INVALID_CONNHANDLE; consumer_cfg[i].connHandle=INVALID_CONNHANDLE; }
    registered_keyboard.pValue=(uint8_t *)keyboard_cfg; registered_consumer.pValue=(uint8_t *)consumer_cfg;
    HidDev_Register(&cfg,NULL); HidDev_RegisterReports(2,map);
    hidDevGapStateCB(GAPROLE_CONNECTED,&link);
    assert(!remote_hid_secure()); /* A link alone does not authorize HID. */
    assert(HidDev_Report(1,HID_REPORT_TYPE_INPUT,8,input)==bleNotReady);
    keyboard_cfg[0].connHandle=active_handle; keyboard_cfg[0].value=GATT_CLIENT_CFG_NOTIFY;
    hidDevPairStateCB(active_handle,GAPBOND_PAIRING_STATE_COMPLETE,SUCCESS);
    assert(remote_hid_secure() && remote_hid_report_enabled(1));
    assert(HidDev_Report(1,HID_REPORT_TYPE_INPUT,8,input)==SUCCESS && notifications==1);
    assert(HidDev_Report(2,HID_REPORT_TYPE_INPUT,2,input)==bleNoResources);
    allocation_failure=1;
    assert(HidDev_Report(1,HID_REPORT_TYPE_INPUT,8,input)==bleMemAllocError);
    allocation_failure=0; notification_status=bleNoResources;
    assert(HidDev_Report(1,HID_REPORT_TYPE_INPUT,8,input)==bleNoResources && frees==1);
    notification_status=SUCCESS;
    hidDevGapStateCB(GAPROLE_WAITING,&ended);
    assert(!remote_hid_secure() && keyboard_cfg[0].value==0);
    /* Bonded reconnection uses the SDK BONDED callback and restored CCC slot. */
    active_handle=19; link.linkCmpl.connectionHandle=active_handle;
    hidDevGapStateCB(GAPROLE_CONNECTED,&link);
    assert(!remote_hid_secure());
    keyboard_cfg[2].connHandle=active_handle; keyboard_cfg[2].value=GATT_CLIENT_CFG_NOTIFY;
    hidDevPairStateCB(active_handle,GAPBOND_PAIRING_STATE_BONDED,SUCCESS);
    assert(remote_hid_secure());
    assert(HidDev_Report(1,HID_REPORT_TYPE_INPUT,8,input)==SUCCESS);
    assert(!memcmp(buffer,input,8));
    puts("PASS: official WCH connect/pair/disconnect/bonded-reconnect callbacks, CCC reset/restore and transport failures");
    return 0;
}
