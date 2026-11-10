# CH583 BLE Remote

Six-button Bluetooth LE HID remote for Android TV, built for a custom CH583M
PCB powered by a CR2032. Firmware version: **0.6.2-low-tx**.

Features include keyboard/consumer controls, persistent pairing, GPIO wake,
BLE-controlled sleep, estimated battery reporting and configurable transmit
power. The latest candidate uses **-5dBm**; range and current consumption still
need hardware qualification. Pairing, buttons, reconnect and battery reporting
were confirmed on the preceding 0.6.1 version.

| Switch | GPIO | Short press | Hold (600 ms) |
|---|---|---|---|
| SW1 | PA13 | Power / standby | Power held 1.5 s (Android power menu) |
| SW2 | PB22 (rev2) / PB0 (rev1) | Up | Volume Up (one tap) |
| SW3 | PB4 | Left | Back (one tap) |
| SW4 | PB2 | Enter / OK | — |
| SW5 | PB1 | Right | Home (one tap) |
| SW6 | PB3 | Down | Volume Down (one tap) |

Hold SW1 + SW2 (Power + Up) for five seconds to clear bonds and reopen pairing.
Power and Up taps are sent on release, and the Up long-press (Volume Up) is
suppressed while the chord is held, so the pairing chord does not trigger them.
Arrow taps are also sent on release, because a hold becomes Volume/Back/Home
(APP_LONG_PRESS_MS). Each hold sends exactly one tap; release and hold again
for another. No key auto-repeats.

## Build

Requirements: Python 3.9+, Git, host GCC for tests, and WCH-compatible
`riscv-none-embed` GCC 8.2.0 for firmware. MounRiver Studio's legacy toolchain
is supported. See [development setup](docs/development.md).

```sh
python tools/setup_sdk.py
python tools/build.py test
python tools/build.py release
```

The SDK is fetched at the revision in [sdk.lock.json](sdk.lock.json); it is
not committed or included in the source archive. No Python packages are needed
for building or testing. Debug and diagnostic modes are also available.

## Flash and pair

Flash `build/release/ch583-remote-0.6.2-low-tx.hex` (or `.bin`) through the
board's established WCH USB ISP procedure. The USB bootloader requires the
board's boot/reset sequence; this firmware does not provide runtime USB.
Pair **CH583 Remote** through Android's normal Bluetooth settings.

Keep existing bonds during an ordinary firmware update. If new services are
missing, refresh discovery in nRF Connect before clearing the pairing.
Battery Service `0x180F` / Battery Level `0x2A19` reports a coarse estimate.
Android's system battery display depends on host support.

## Documentation

- [Hardware and supported board](docs/hardware.md)
- [Build, tests and SDK](docs/development.md)
- [Battery estimate and power settings](docs/power.md)
- [Hardware acceptance checklist](docs/testing.md)
- [Preparing a GitHub release](docs/releasing.md)
- [Changelog](CHANGELOG.md)

## License

Original firmware code is [MIT licensed](LICENSE). WCH-derived code and SDK
components retain their own terms; see [third-party notices](THIRD_PARTY_NOTICES.md).
The hardware design (outside `Firmware/`) is separately licensed CC BY-NC-SA 4.0;
see the [repository README](../README.md).
Firmware is intended for the supported WCH hardware. No battery-life or ADC
accuracy claim is made without measurements.
