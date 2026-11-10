# Vendor source provenance

`ch583/` is a git submodule pointing at the unmodified official
https://github.com/openwch/ch583 repository, pinned to
`bd508ad7ceed48377619837051412a651952857f` (2025-08-14, “Update EVT to v2.4”).
The parent repository stores only this pointer; no SDK files or libraries are
committed or redistributed here. BLE header version is
`CH58x_BLE_LIB_V2.1`; HAL CONFIG/MCU/SLEEP are V1.2. These component versions
are distinct from the EVT package version.

Preserve `ch583/LICENSE` and all per-file notices. The SDK is Apache-2.0
(original project code is under the root LICENSE); vendor source headers additionally state that modified/unmodified
software and binaries are for WCH-manufactured microcontrollers. Do not remove
those notices or assume every binary has independent source availability.
`libCH58xBLE.a` and `libISP583.a` are actual vendor archives, not generated mocks.

Vendor originals remain unchanged; application adaptations are kept in src/ble.
For a fresh checkout:

```powershell
git clone --recurse-submodules <this-repo>
# or, in an existing clone:
git submodule update --init
# or let the build tooling do it (also works from an unpacked source zip):
python tools/setup_sdk.py
```

Reproduction requires this exact revision, not a future `main`; do not run
`git submodule update --remote` unless deliberately upgrading the SDK (then
update `sdk.lock.json` to match).


HID adaptation: src/ble/wch_hiddev.c derives from
EVT/EXAM/BLE/HID_Consumer/Profile/hiddev.c at the revision above. Its WCH notice
is preserved. Changes remove demo battery/scan services and automatic advertising,
remove the uninitialized passcode/debug printf fallback, expose secure state,
and forward bond events to the application. The original GATT report callbacks,
notification allocation/freeing, role startup and bond-security handling are reused.
Battery Service is omitted until calibrated readings are available. Device
Information exposes model and firmware with no borrowed PnP/certification IDs.
Host tests use explicit transport mocks compiled against vendor declarations;
no mock is linked into firmware or substitutes for BLE/ISP libraries.

Latency pass: the adapted HID parameter-update callback additionally forwards
actual negotiated values to the application diagnostics. Vendor originals remain unchanged.

0.3.2 adds remote_hid_report_enabled to the adapted HID layer. It queries the
same SDK connection/report/registered attribute used by HidDev_Report; the app
calls it only to classify a failed send. A host test exercises the real adapted
sender/query implementation, with low-level transport mocks outside firmware.

0.3.4 gates the adapted sender on documented live link encryption, validates
physical link-event types, and forwards security callbacks after updating state.
Existing SDK bond/CCC persistence is retained; no undocumented exported symbol
is invoked and no new vendor library is substituted.

0.3.7 corrects the adapted readiness query to use physical LINK_CONNECTED and
LINK_ENCRYPTED without an advertising-role requirement. A separate read-only
subscription query exposes raw registered CCCs without forcing enablement.


0.4.0 restores the SDK lifecycle instead of adding to the prior experiments.
Thirteen function bodies are parity-checked by tests/check_wch_baseline.py.
Current differences: optional battery/scan demo calls omitted; bond observer
runs after SDK state changes; passcode fallback safely uses zero without logs;
parameter callback records diagnostics; application report/service tables are
custom. See docs/wch-baseline-audit.md. Prior experimental role/link gates and
connected-advertising policies documented above are superseded.
