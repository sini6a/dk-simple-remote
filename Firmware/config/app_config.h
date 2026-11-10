#ifndef REMOTE_APP_CONFIG_H
#define REMOTE_APP_CONFIG_H

#define APP_DEVICE_NAME "DK MiniRemote"
#ifndef APP_DIAGNOSTICS
#define APP_DIAGNOSTICS 0
#endif
#define APP_FIRMWARE_VERSION "0.6.10-latency30"
/* GAP Appearance, advertised and in GATT: HID Keyboard (0x03C1).
 * Generic HID (0x03C0) shows as a gamepad on Android and may be filtered
 * from Android TV's accessory pairing list. */
#define APP_APPEARANCE 0x03C1U
/* Whole-firmware radio policy, force-included by tools/build.py so the
 * vendor HAL/MCU.c sees it too. SDK constants are encoded levels, not dBm. */
#ifndef APP_TX_POWER_DBM
#define APP_TX_POWER_DBM (-5)
#endif
#if APP_TX_POWER_DBM == -16
#define BLE_TX_POWER LL_TX_POWEER_MINUS_16_DBM
#elif APP_TX_POWER_DBM == -12
#define BLE_TX_POWER LL_TX_POWEER_MINUS_12_DBM
#elif APP_TX_POWER_DBM == -8
#define BLE_TX_POWER LL_TX_POWEER_MINUS_8_DBM
#elif APP_TX_POWER_DBM == -5
#define BLE_TX_POWER LL_TX_POWEER_MINUS_5_DBM
#elif APP_TX_POWER_DBM == -3
#define BLE_TX_POWER LL_TX_POWEER_MINUS_3_DBM
#elif APP_TX_POWER_DBM == -1
#define BLE_TX_POWER LL_TX_POWEER_MINUS_1_DBM
#elif APP_TX_POWER_DBM == 0
#define BLE_TX_POWER LL_TX_POWEER_0_DBM
#else
#error Unsupported APP_TX_POWER_DBM: choose -16, -12, -8, -5, -3, -1 or 0
#endif
#define APP_SAMPLE_MS 5U
#define APP_DEBOUNCE_MS 20U
#define APP_PAIR_HOLD_MS 5000U
#define APP_BUTTON_COUNT 6U
#define APP_CONN_MIN 12U /* 15ms, units of 1.25ms */
#define APP_CONN_MAX 24U /* 30ms, host has final say */
/* Idle connection events the remote may skip: at 30ms it wakes about every
 * 0.93s instead of 150ms, the main battery saving while connected. Key presses
 * still go out on the next event (15-30ms). The host has final control; if it
 * refuses, its own parameters remain. Without a 32kHz crystal, verify an
 * overnight connection; lower to 15-20 if the link drops. */
#define APP_CONN_LATENCY 30U
/* Supervision timeout, 10ms units (500 = 5s). Checked in ble_manager.c against
 * Apple's accessory limits, which are the strictest common host rules. */
#define APP_CONN_TIMEOUT 500U
/* Battery: 12 hours between measurements, only while connected. A measurement
 * starts APP_BATTERY_SETTLE_MS after connecting (link setup traffic is over),
 * takes APP_BATTERY_SAMPLES readings APP_BATTERY_SPACING_MS apart and keeps the
 * highest, rejecting radio-burst dips. Times must be multiples of 5ms. */
#define APP_BATTERY_PERIOD_MS 43200000UL
#define APP_BATTERY_SETTLE_MS 10000UL
#define APP_BATTERY_SPACING_MS 2000UL
#define APP_BATTERY_SAMPLES 3U
/* Per-board gain correction of the CH583 internal battery measurement, in
 * 1/10000 (10000 = none). The internal ~1.05V reference is uncalibrated. This
 * board: meter 2979mV at VDD vs 2814mV reported -> 2979/2814 = 1.0586. */
#ifndef APP_BATTERY_CAL_SCALE
#define APP_BATTERY_CAL_SCALE 10586UL
#endif
#define APP_RECONNECT_WINDOW_MS 30000UL
#define APP_ADVERTISING_WINDOW_MS 120000UL
#define APP_ADVERTISING_INTERVAL 320U /* 200ms, units of 625us */
/* Android TV defaults: consumer Power, keyboard arrows/Enter.
 * Set to 1 for Consumer Channel Increment/Decrement on SW2/SW6. */
#ifndef APP_PROGRAM_BUTTONS
#define APP_PROGRAM_BUTTONS 0
#endif
#if APP_PROGRAM_BUTTONS
#define APP_VERTICAL_PAGE 0x000c
#define APP_UP_USAGE 0x009c
#define APP_DOWN_USAGE 0x009d
#else
#define APP_VERTICAL_PAGE 0x0007
#define APP_UP_USAGE 0x0052
#define APP_DOWN_USAGE 0x0051
#endif
/* Long-press. A shorter press sends the button's own key as a tap on release,
 * so these buttons do not auto-repeat as arrows. Holding for APP_LONG_PRESS_MS
 * sends exactly one tap of the long key; release and hold again for another:
 * SW2 (Up) Volume Up, SW3 (Left) Back, SW5 (Right) Home, SW6 (Down) Volume Down.
 * Virtual keys 6..9 follow the six physical buttons in the keymap. */
#define APP_LONG_PRESS_MS 600U
/* Holding SW1 (Power) alone for APP_LONG_PRESS_MS sends Power held down for
 * this fixed time: an Android long-press (power menu, >=500ms) but below the
 * 3.5s very-long-press. A short press remains a Power tap (standby). */
#define APP_POWER_HOLD_MS 1500U
#define APP_LONG_PRESS_MASK ((1U << 1) | (1U << 2) | (1U << 4) | (1U << 5))
#define APP_KEY_BACK 6U
#define APP_KEY_HOME 7U
#define APP_KEY_VOLUME_UP 8U
#define APP_KEY_VOLUME_DOWN 9U
#define APP_KEY_COUNT 10U
#define APP_LONG_KEY(button) ((button) == 1U ? APP_KEY_VOLUME_UP : \
    (button) == 2U ? APP_KEY_BACK : (button) == 4U ? APP_KEY_HOME : APP_KEY_VOLUME_DOWN)
#ifndef APP_KEY_PAGES
#define APP_KEY_PAGES {0x000c, APP_VERTICAL_PAGE, 0x0007, 0x0007, 0x0007, APP_VERTICAL_PAGE, \
    0x000c, 0x000c, 0x000c, 0x000c}
#endif
#ifndef APP_KEY_USAGES
/* Virtual: Consumer AC Back 0x0224, AC Home 0x0223, Volume Increment 0x00e9,
 * Volume Decrement 0x00ea (Android KEYCODE_BACK/HOME/VOLUME_UP/VOLUME_DOWN). */
#define APP_KEY_USAGES {0x0030, APP_UP_USAGE, 0x0050, 0x0028, 0x004f, APP_DOWN_USAGE, \
    0x0224, 0x0223, 0x00e9, 0x00ea}
#endif
/* No runtime UART/USB. BLE HAL owns retained-RAM sleep and wake deadlines. */
#endif
