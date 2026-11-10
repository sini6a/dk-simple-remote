# Battery and power

The normal release uses GPIO wake and BLE-scheduled retained-RAM sleep. Button
sampling runs every 5ms only while needed for debounce, held keys or HID retries.
RTC-timed settling after GPIO/RTC wake guards crystal startup before BLE resumes.
Full 32KB RAM retention and SDK RF/internal-RC calibration remain enabled.

Unused UART0-3, SPI0/1, I2C, PWM and USB peripheral clocks are gated. ADC, input
buffer, temperature sensor and TouchKey are off between sample bursts. Battery
is measured once at boot (radio still off) and then every 12 hours while
connected. A connected measurement starts 10s after the link comes up, takes
three readings 2s apart and keeps the highest, so a radio-burst dip is rejected.
The 12-hour schedule resumes across reconnects; the timer stops when
disconnected, and active keys/reports defer sampling by 30s. Reads use cached
data. Estimates do not write flash or trigger automatic shutdown.

Bonded reconnect advertising lasts 30 seconds, unpaired discovery two minutes;
a button reopens it. The host decides whether to accept the requested 15-30ms
interval, latency 30 (wake about every 0.93s while idle) and a 5s
supervision timeout. Without a 32kHz crystal the internal RC clock times the
longer sleeps; verify an overnight connection and lower APP_CONN_LATENCY to
15-20 if the link drops. Transmit power defaults to -5dBm, configurable in
app_config.h. Lower RF output can reduce range and increase retries; it is not
a proportional reduction in total battery current. Restore 0dBm if needed.

## Battery estimate

Battery Service 0x180F exposes Battery Level 0x2A19. Its lookup is derived from
Energizer's blue background discharge curve at 21C with a 15kohm load and periodic
400ohm pulses, normalized to a conservative 2.2V cutoff. The graph has a long
voltage plateau, so inferred percentage can be substantially wrong for a different
cell/load/temperature or uncalibrated ADC reference. Values use 5% quantization. A
change needs two consecutive measurements that agree: any drop, or a rise of at
least 10 points. It then moves only to the less extreme of the two, so one bad
reading can neither lower nor raise the reported level.

The CH583 internal reference is nominal, so each board can read a few percent
off. Calibrate with APP_BATTERY_CAL_SCALE in app_config.h (1/10000 units): flash
the diagnostic build, measure VDD with a meter while connected, read ...0203 and
set the scale to 10000 x meter / reported, using the reported value from a
build with the scale set to 10000. The reference board needed 10586. A failed sample retains the
last valid value; an unavailable first reading returns an ATT error.

Source and exact extraction provenance: [curve metadata](data/energizer-cr2032-curve.json),
[CSV](data/energizer-cr2032-curve.csv), and
[Energizer datasheet](https://www.data.energizer.com/pdfs/ultimatecr2032EU0920.pdf).
No manufacturer PDF is redistributed. Lookup/provenance consistency is tested.

## Diagnostic reads

Diagnostic firmware adds read-only values under Battery Service. It also retains
a one-second reconnect heartbeat, so use the normal release for current tests.

- ...0203: millivolts of the last committed measurement (best of its burst), two little-endian bytes. B8 0B = 3000mV.
- ...0204: eight-byte power state, described below.

Both use UUID prefix 64ac1f90-3a7b-4d52-8e61-cb0583b6 (full power UUID:
64ac1f90-3a7b-4d52-8e61-cb0583b60204).

| Byte | Meaning |
|---|---|
| 0 | Format, 1 |
| 1 | Configured signed TX dBm (FB = -5) |
| 2 | SDK TX encoding (07 = -5dBm) |
| 3 | Live DC/DC flags: bit0 enable, bit1 pre-enable, bit2 charge |
| 4 | Boot DC/DC flags, same encoding |
| 5-6 | Peripheral clock-gate mask, little endian |
| 7 | Enables: bit0 ADC, bit1 buffer, bit2 temperature, bit3 TouchKey |

Usual idle power record: 01 FB 07 07 07 F0 3F 00. Readback verifies register
configuration, not actual RF output or electrical conversion efficiency.
Reconnect diagnostics remain in service ...0201 / characteristic ...0202;
these use the same UUID prefix. Device Information / Firmware Revision 0x2A26
identifies the flashed version. Refresh nRF Connect discovery if services are
missing. Firmware attempts one Service Changed indication after encryption per
boot; hosts must have subscribed for it to work. Fresh pairing is a fallback,
not a routine requirement. Android's native battery display is host-dependent.
