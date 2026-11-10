# Changelog

## 0.6.10-latency30

- Request connection latency 30 instead of 4: about one wake per 0.93s instead
  of every 150ms while connected and idle. Key presses are not delayed.
- Add APP_CONN_TIMEOUT (5s) with a build-time check against Apple's accessory
  connection-parameter limits.

## 0.6.9-battery-cal

- Add APP_BATTERY_CAL_SCALE, a per-board gain correction for the internal
  battery measurement. Set to 1.0586 for the reference board (meter 2979mV,
  reported 2814mV).

## 0.6.8-battery

- Measure the battery every 12 hours instead of every 30 minutes, resuming the
  schedule across reconnects.
- Start connected measurements 10s after connecting and keep the best of three
  readings 2s apart, rejecting radio-burst voltage dips.
- Change the reported level only after two consecutive measurements agree.

## 0.6.2-low-tx

- Configure BLE transmit power at -5dBm throughout the SDK/application build.
- Add live/boot DC/DC and analog/peripheral diagnostic readback.
- Preserve the previous connection/sleep behavior. RF range/current validation pending.

## 0.6.1-curve

- Replace guessed battery percentages with a traceable Energizer curve estimate.
- Advertise Battery Service and attempt one service-cache refresh per boot.
- Battery reporting confirmed working on hardware.

## 0.6.0-battery

- Add real internal voltage measurement, cached Battery Service and sparse sampling.
- Gate unused peripherals and request idle connection latency four.

## 0.5.0-gpio-sleep

- Add GPIO wake, event-driven input sampling, BLE sleep and crystal settling.
- Bound advertising and remove the normal periodic diagnostic heartbeat.

## 0.4.0-wch-base

- Restore official WCH pairing/reconnect procedures and working Android TV controls.
