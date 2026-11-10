#ifndef REMOTE_BLE_MANAGER_H
#define REMOTE_BLE_MANAGER_H
#include <stdint.h>
#include "CONFIG.h"
typedef struct {
    uint32_t heartbeat;
    uint32_t errors;
    uint8_t last_status;
    uint8_t role_state;
    uint16_t conn_interval; /* 1.25ms units; actual host-negotiated value */
    uint16_t conn_latency;
    uint16_t conn_timeout; /* 10ms units */
    uint32_t param_requests;
    uint32_t security_requests;
} ble_diagnostics_t;
extern volatile ble_diagnostics_t ble_diagnostics;
void ble_manager_init(void);
void ble_manager_read_diagnostics(uint8_t out[32]);
uint16_t ble_manager_connection(void);
void ble_manager_param_updated(uint16_t handle, uint16_t interval, uint16_t latency, uint16_t timeout);
void ble_manager_pairing_reset(void);
void ble_manager_bond_event(uint8_t state, uint8_t status);
void ble_manager_state_changed(gapRole_States_t state, gapRoleEvent_t *event);
/* Called from TMOS task context with debounced SW1..SW6 bits. */
void ble_manager_buttons(uint8_t down);
#endif
