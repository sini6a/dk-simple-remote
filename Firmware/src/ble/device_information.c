/* Minimal standard Device Information Service. No borrowed PnP identifiers,
 * serial number or regulatory claims. Registration follows WCH's DIS example. */
#include "CONFIG.h"
#include "devinfoservice.h"
#include "app_config.h"
static const uint8_t service_uuid[] = {LO_UINT16(DEVINFO_SERV_UUID), HI_UINT16(DEVINFO_SERV_UUID)};
static const uint8_t model_uuid[] = {LO_UINT16(MODEL_NUMBER_UUID), HI_UINT16(MODEL_NUMBER_UUID)};
static const uint8_t firmware_uuid[] = {LO_UINT16(FIRMWARE_REV_UUID), HI_UINT16(FIRMWARE_REV_UUID)};
static const gattAttrType_t service = {ATT_BT_UUID_SIZE, service_uuid};
static uint8_t properties = GATT_PROP_READ;
static uint8_t model[] = APP_DEVICE_NAME;
static uint8_t firmware[] = APP_FIRMWARE_VERSION;
#define ATTR(uuid, value) {{ATT_BT_UUID_SIZE, uuid}, GATT_PERMIT_READ, 0, (uint8_t *)(value)}
static gattAttribute_t attrs[] = {
    ATTR(primaryServiceUUID, &service), ATTR(characterUUID, &properties), ATTR(model_uuid, model),
    ATTR(characterUUID, &properties), ATTR(firmware_uuid, firmware)
};
static bStatus_t read_value(uint16_t conn, gattAttribute_t *attr, uint8_t *value,
                            uint16_t *len, uint16_t offset, uint16_t max, uint8_t method)
{
    uint16_t size = attr->pValue == model ? sizeof(model)-1 : sizeof(firmware)-1;
    (void)conn; (void)method;
    if (offset > size) return ATT_ERR_INVALID_OFFSET;
    *len = MIN(max, size-offset); tmos_memcpy(value, attr->pValue+offset, *len);
    return SUCCESS;
}
static gattServiceCBs_t callbacks = {read_value, NULL, NULL};
bStatus_t DevInfo_AddService(void)
{
    return GATTServApp_RegisterService(attrs, GATT_NUM_ATTRS(attrs), GATT_MAX_ENCRYPT_KEY_SIZE, &callbacks);
}
