# Development

Use Python 3.9 or later, Git and GCC. Run commands from the repository root.

```sh
python tools/setup_sdk.py
python tools/build.py test
```

The setup tool verifies the exact SDK revision in sdk.lock.json. It does not
replace a different checkout or discard local SDK modifications. The official
SDK is downloaded into ignored vendor/ch583. Its license/notices stay intact.

For target builds use MounRiver's xPack `riscv-none-embed-gcc` 8.2.0. The project
uses rv32imac/ilp32, the official startup/linker script and SDK binary libraries.
Do not substitute a generic RISC-V ISA or an unrelated precompiled example.
MounRiver Studio 3's legacy toolchain is detected under its standard Windows
installation. Otherwise put it on PATH or set CROSS_COMPILE to its full prefix:

```powershell
$env:CROSS_COMPILE = 'C:/path/to/toolchain/bin/riscv-none-embed-'
$env:HOST_CC = 'C:/path/to/host/gcc.exe'
python tools/build.py release
python tools/build.py debug
python tools/build.py diagnostic
```

Host GCC's runtime DLL directory must also be on PATH on Windows. Python has
no third-party build/test dependencies. A host test is a software/mock check,
not a radio or power measurement. GitHub CI runs the host tests and reference
connectivity check; target builds require the separately installed WCH toolchain.

Outputs are under build/<mode>. Versioned HEX/BIN files are flashable. ELF/MAP
files are local debugging artifacts. Fresh build manifests record inputs and
image checksums. Make targets wrap the same Python commands.

Configuration lives in config/app_config.h and config/board.h. APP_TX_POWER_DBM
accepts -16, -12, -8, -5, -3, -1 or 0. The build force-includes the application
configuration in vendor translation units so the SDK receives its encoded RF
setting. Host tests check the actual preprocessed vendor MCU.c for every level.
APP_PROGRAM_BUTTONS=1 replaces Up/Down with channel increment/decrement.

src/ble/wch_hiddev.c follows the official HID lifecycle. Thirteen function bodies
are compared mechanically against the pinned SDK, allowing only documented demo
service omissions and observer hooks. Pairing uses WAIT_FOR_REQ, an open filter
and SDK bond persistence. Avoid speculative security/reconnect loops.

Builds check an adjacent ../KiCad/dk-simple-remote.kicad_pcb when available. Standalone
checkouts validate config/hardware-reference.json instead, with a clear warning
that the physical board was not checked. See hardware.md before using another PCB.
