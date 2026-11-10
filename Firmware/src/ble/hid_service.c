/* GATT declaration/registration pattern and HidDev callbacks are from
 * WCH HID_Consumer/Profile/hidconsumerservice.c. New report-only descriptor:
 * keyboard ID 1 (8 bytes), Consumer ID 2 (one 16-bit usage). */
#include "CONFIG.h"
#include "hiddev.h"
#include "hid_service.h"
#include "hid_reports.h"
#include "ble_manager.h"
#include <string.h>
#define UUID(name, value) static const uint8_t name[] = {LO_UINT16(value), HI_UINT16(value)}
UUID(service_uuid, HID_SERV_UUID);
UUID(info_uuid, HID_INFORMATION_UUID);
UUID(control_uuid, HID_CTRL_PT_UUID);
UUID(map_uuid, REPORT_MAP_UUID);
UUID(report_uuid, REPORT_UUID);
static const gattAttrType_t service = {ATT_BT_UUID_SIZE, service_uuid};
static uint8_t read_props = GATT_PROP_READ;
static uint8_t control_props = GATT_PROP_WRITE_NO_RSP;
static uint8_t input_props = GATT_PROP_READ | GATT_PROP_NOTIFY;
static uint8_t info[] = {0x11, 0x01, 0, HID_FLAGS_REMOTE_WAKE};
static uint8_t control;
uint8_t hidProtocolMode = HID_PROTOCOL_MODE_REPORT;
static uint8_t descriptor[] = {
    0x05,0x01, 0x09,0x06, 0xa1,0x01, 0x85,0x01,
    0x05,0x07, 0x19,0xe0, 0x29,0xe7, 0x15,0x00, 0x25,0x01,
    0x75,0x01, 0x95,0x08, 0x81,0x02,
    0x75,0x08, 0x95,0x01, 0x81,0x01,
    0x19,0x00, 0x29,0x65, 0x15,0x00, 0x25,0x65,
    0x75,0x08, 0x95,0x06, 0x81,0x00, 0xc0,
    0x05,0x0c, 0x09,0x01, 0xa1,0x01, 0x85,0x02,
    0x15,0x00, 0x26,0xff,0x03, 0x19,0x00, 0x2a,0xff,0x03,
    0x75,0x10, 0x95,0x01, 0x81,0x00, 0xc0
};
uint16_t hidReportMapLen = sizeof(descriptor);
static uint8_t keyboard[8], consumer[2];
static uint8_t key_ref[] = {1, HID_REPORT_TYPE_INPUT};
static uint8_t consumer_ref[] = {2, HID_REPORT_TYPE_INPUT};
static gattCharCfg_t key_ccc[GATT_MAX_NUM_CONN], consumer_ccc[GATT_MAX_NUM_CONN];
#define ATTR(uuid, permissions, value) {{ATT_BT_UUID_SIZE, uuid}, permissions, 0, (uint8_t *)(value)}
static gattAttribute_t attrs[] = {
    ATTR(primaryServiceUUID, GATT_PERMIT_READ, &service),
    ATTR(characterUUID, GATT_PERMIT_READ, &read_props),
    ATTR(info_uuid, GATT_PERMIT_ENCRYPT_READ, info),
    ATTR(characterUUID, GATT_PERMIT_READ, &control_props),
    ATTR(control_uuid, GATT_PERMIT_ENCRYPT_WRITE, &control),
    ATTR(characterUUID, GATT_PERMIT_READ, &read_props),
    ATTR(map_uuid, GATT_PERMIT_ENCRYPT_READ, descriptor),
    ATTR(characterUUID, GATT_PERMIT_READ, &input_props),
    ATTR(report_uuid, GATT_PERMIT_ENCRYPT_READ, keyboard),
    ATTR(clientCharCfgUUID, GATT_PERMIT_READ | GATT_PERMIT_ENCRYPT_WRITE, key_ccc),
    ATTR(reportRefUUID, GATT_PERMIT_READ, key_ref),
    ATTR(characterUUID, GATT_PERMIT_READ, &input_props),
    ATTR(report_uuid, GATT_PERMIT_ENCRYPT_READ, consumer),
    ATTR(clientCharCfgUUID, GATT_PERMIT_READ | GATT_PERMIT_ENCRYPT_WRITE, consumer_ccc),
    ATTR(reportRefUUID, GATT_PERMIT_READ, consumer_ref)
};
static hidRptMap_t reports[2];
static gattServiceCBs_t service_callbacks = {HidDev_ReadAttrCB, HidDev_WriteAttrCB, NULL};
static hidDevCfg_t configuration = {0, HID_FLAGS_REMOTE_WAKE};
static hid_reports_t pending;
static bool suspended, secure_seen;
static uint8_t force_reports = 3;
static uint32_t last_poll_ms;
static void (*wake_task)(void);
void hid_service_set_wake(void (*wake)(void)) { wake_task = wake; }
bool hid_service_has_pending(void) { return pending.count != 0; }
volatile hid_diagnostics_t hid_diagnostics;
volatile uint32_t hid_dropped_actions;
static uint8_t report_callback(uint8_t id, uint8_t type, uint16_t uuid,
                               uint8_t operation, uint16_t *len, uint8_t *data)
{
    (void)uuid;
    if (type != HID_REPORT_TYPE_INPUT || (id != 1 && id != 2)) return ATT_ERR_ATTR_NOT_FOUND;
    if (operation == HID_DEV_OPER_READ) {
        *len = id == 1 ? 8 : 2;
        tmos_memcpy(data, id == 1 ? keyboard : consumer, *len);
    } else if (operation == HID_DEV_OPER_ENABLE) {
        force_reports |= (uint8_t)(1U << (id-1));
        hid_dropped_actions += pending.count;
        hid_reports_release(&pending, last_poll_ms);
        if (wake_task) wake_task();
    } else if (operation == HID_DEV_OPER_WRITE) return ATT_ERR_WRITE_NOT_PERMITTED;
    return SUCCESS;
}
static void event_callback(uint8_t event)
{
    /* Match the official HID Consumer application: no application-side
     * suspend gate. WCH advertises RemoteWake and controls the link. */
    (void)event;
}
static hidDevCB_t callbacks = {report_callback, event_callback, NULL, ble_manager_state_changed};
uint8_t hid_service_init(void)
{
    uint8_t status;
    hid_reports_reset(&pending);
    HidDev_Register(&configuration, &callbacks);
    GATTServApp_InitCharCfg(INVALID_CONNHANDLE, key_ccc);
    GATTServApp_InitCharCfg(INVALID_CONNHANDLE, consumer_ccc);
    status = GATTServApp_RegisterService(attrs, GATT_NUM_ATTRS(attrs), GATT_MAX_ENCRYPT_KEY_SIZE, &service_callbacks);
    reports[0] = (hidRptMap_t){attrs[8].handle, attrs[9].handle, 1, HID_REPORT_TYPE_INPUT, HID_PROTOCOL_MODE_REPORT};
    reports[1] = (hidRptMap_t){attrs[12].handle, attrs[13].handle, 2, HID_REPORT_TYPE_INPUT, HID_PROTOCOL_MODE_REPORT};
    HidDev_RegisterReports(2, reports);
    return status;
}
void hid_service_key(unsigned key, bool down, uint32_t now)
{
    /* Zero-capacity disconnected policy: counted, never silently replayed. */
    if (!remote_hid_secure() || suspended) { if (down) ++hid_dropped_actions; return; }
    hid_reports_key(&pending, key, down, now);
}
void hid_service_poll(uint32_t now)
{
    unsigned work;
    last_poll_ms = now;
    if (!remote_hid_secure()) { secure_seen = false; return; }
    if (suspended) return;
    if (!secure_seen) {
        /* Encryption restoration need not arrive as a fresh pairing callback. */
        hid_service_ready(); secure_seen = true;
    }
    /* Skip redundant phases in this same callback; never wait on a CCC that
     * the host has not enabled. Each iteration advances or stops on backpressure. */
    for (work = 0; work < HID_PENDING_FRAMES * 2U; ++work) {
        uint8_t id, len, status, bit, *previous;
        const uint8_t *body;
        uint32_t age;
        if (!hid_reports_peek(&pending, now, &id, &body, &len)) break;
        age = now - pending.queue[pending.head].at;
        if (age > hid_diagnostics.max_queue_age_ms) hid_diagnostics.max_queue_age_ms = age;
        previous = id == 1 ? keyboard : consumer;
        bit = (uint8_t)(1U << (id-1));
        if (!(force_reports & bit) && memcmp(previous, body, len) == 0) {
            ++hid_diagnostics.unchanged;
            hid_reports_ack(&pending);
            continue;
        }
        status = HidDev_Report(id, HID_REPORT_TYPE_INPUT, len, (uint8_t *)body);
        hid_diagnostics.last_status = status;
        if (status != SUCCESS) {
            /* Only WCH's authoritative HID context can establish that this
             * report is unsubscribed. Never discard before trying the sender. */
            if (status == bleNoResources && !remote_hid_report_enabled(id)) {
                unsigned i; bool nonzero = false;
                for (i = 0; i < len; ++i) nonzero |= body[i] != 0;
                if (nonzero) ++hid_dropped_actions;
                ++hid_diagnostics.unsubscribed;
                hid_reports_ack(&pending);
                continue;
            }
            ++hid_diagnostics.retries; break;
        }
        ++hid_diagnostics.accepted;
        force_reports &= (uint8_t)~bit;
        tmos_memcpy(previous, body, len);
        hid_reports_ack(&pending);
    }
    hid_dropped_actions += pending.dropped; pending.dropped = 0;
}
void hid_service_disconnect(void)
{
    hid_dropped_actions += pending.count;
    hid_reports_reset(&pending); suspended = secure_seen = false; force_reports = 3;
    memset(keyboard, 0, sizeof(keyboard)); memset(consumer, 0, sizeof(consumer));
}

void hid_service_ready(void)
{
    /* Reconnection may restore CCCs without a new CCC write. Start every
     * encrypted link with explicit all-up reports, even in that case. */
    hid_service_disconnect();
    hid_reports_release(&pending, last_poll_ms);
    if (wake_task) wake_task();
}

bool hid_service_is_suspended(void) { return suspended; }
