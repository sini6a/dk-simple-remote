# Third-party notices

The MIT license in this folder (LICENSE) covers original firmware code and
documentation. It does not replace third-party terms or the notices retained
in derived files.

## WCH CH583 SDK and adapted code

Source: https://github.com/openwch/ch583
Pinned revision: bd508ad7ceed48377619837051412a651952857f

The upstream repository provides an Apache-2.0 license; its full text is retained
in LICENSES/WCH-Apache-2.0.txt. Individual SDK files also carry WCH copyright and
use notices, including restriction to WCH-manufactured microcontrollers. Retain
those notices and consult the exact files when redistributing SDK components.
The SDK and its precompiled libraries are obtained separately by setup_sdk.py;
they are not included in the public project source package.

src/ble/wch_hiddev.c adapts the official HID_Consumer/Profile/hiddev.c, omitting
optional demo services and adding application observer/query hooks.
src/power/sleep.c adapts HAL/SLEEP.c for GPIO wake and RTC-timed crystal settling.
Both retain upstream copyright/use notices. Other BLE services follow WCH's
GATT declaration/CCC/notification patterns, with project-specific report maps,
real battery measurements and application behavior. Vendor source is not patched.

Firmware binaries link WCH libCH58xBLE.a and libISP583.a and compile selected
SDK drivers, startup and HAL code. Release packages retain this notice and the
upstream license; they are intended for the supported WCH CH583 hardware.

## Battery curve

Source: Energizer UltimateCR2032EU0920 product datasheet:
https://www.data.energizer.com/pdfs/ultimatecr2032EU0920.pdf

The source PDF is not redistributed. docs/data contains derived lookup samples
and extraction provenance, not a manufacturer accuracy or battery-life guarantee.
Energizer's product names identify the source; no affiliation is claimed.
