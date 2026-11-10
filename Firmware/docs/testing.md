# Hardware acceptance

Host tests cover debounce/chords, report queues, SDK lifecycle parity, GATT
reads/notifications, sparse timers, ADC failure shutdown and power settings.
They do not prove real radio, converter or ADC performance.

Before treating a candidate as stable, test on both the phone and Android TV:

- Verify the flashed version through Device Information / Firmware Revision.
- Check all six keys, quick taps, held arrows, simultaneous keys, aborted reset
  chords and SW1+SW2 (Power + Up) held for five seconds. Check for stuck or duplicate keys.
- Check the first key after several minutes idle and a key held during boot.
- Repeat Bluetooth off/on and battery reinsertion ten times without clearing bonds.
- With an existing bond, wait 30 seconds disconnected; advertising should stop.
  Press a key and verify it restarts and reconnects. Unpaired discovery lasts 2min.
- Check the maximum intended range with the TV box in its cabinet and different
  remote orientations. Compare -5dBm with 0dBm if keys or reconnects fail.
- Read Battery Level repeatedly in nRF Connect. It should return cached values.
  Refresh services if missing; then consider a fresh pairing if necessary.

When equipment is available, profile connected idle, disconnected advertising,
quiet sleep and held-button current. Check low-battery operation, oscillator
frequency/startup, ADC voltage accuracy and runtime stack high-water. Record
the battery type, firmware version, host, accepted connection parameters and
RF setting with results. Save the working source and images before tuning.
