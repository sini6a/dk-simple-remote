/* Read-only diagnostic service, appended AFTER HID so bonded HID handles
 * remain stable. No flash writes or control operations. */
#include "CONFIG.h"
#include "app_config.h"
#include "ble_manager.h"
#include "diagnostic_service.h"
#if APP_DIAGNOSTICS
static const uint8_t service_uuid[16] = {
    0x01,0x02,0xb6,0x83,0x05,0xcb,0x61,0x8e,0x52,0x4d,0x7b,0x3a,0x90,0x1f,0xac,0x64};
static const uint8_t state_uuid[16] = {
    0x02,0x02,0xb6,0x83,0x05,0xcb,0x61,0x8e,0x52,0x4d,0x7b,0x3a,0x90,0x1f,0xac,0x64};
static const uint8_t label_uuid[] = {0x01,0x29};
static const gattAttrType_t service = {16, service_uuid};
static uint8_t read_properties = GATT_PROP_READ;
static uint8_t read_buffer[32];
static uint8_t label[] = "Reconnect state (hex)";
static gattAttribute_t attrs[] = {
    {{ATT_BT_UUID_SIZE,primaryServiceUUID},GATT_PERMIT_READ,0,(uint8_t *)&service},
    {{ATT_BT_UUID_SIZE,characterUUID},GATT_PERMIT_READ,0,&read_properties},
    {{16,state_uuid},GATT_PERMIT_READ,0,read_buffer},
    {{ATT_BT_UUID_SIZE,label_uuid},GATT_PERMIT_READ,0,label}
};
static bStatus_t read_value(uint16_t conn,gattAttribute_t *attr,uint8_t *value,
                            uint16_t *len,uint16_t offset,uint16_t max,uint8_t method)
{
    uint16_t size;
    uint8_t *data;
    (void)conn; (void)method;
    if (attr->pValue == label) { data = label; size = sizeof(label)-1; }
    else {
        /* One coherent read even when ATT reads this in several chunks. */
        if (offset == 0) ble_manager_read_diagnostics(read_buffer);
        data = read_buffer; size = sizeof(read_buffer);
    }
    if (offset > size) return ATT_ERR_INVALID_OFFSET;
    *len = MIN(max,size-offset); tmos_memcpy(value,data+offset,*len);
    return SUCCESS;
}
static gattServiceCBs_t callbacks = {read_value,NULL,NULL};
uint8_t diagnostic_service_init(void)
{
    return GATTServApp_RegisterService(attrs,GATT_NUM_ATTRS(attrs),GATT_MAX_ENCRYPT_KEY_SIZE,&callbacks);
}
#else
uint8_t diagnostic_service_init(void) { return SUCCESS; }
#endif
