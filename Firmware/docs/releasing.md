# Preparing a GitHub release

The original project code is MIT licensed. Keep LICENSE, THIRD_PARTY_NOTICES.md
and LICENSES together with source/binary distributions. The official SDK remains
a separately fetched dependency pinned in sdk.lock.json. Firmware links its WCH
libraries and is intended for supported WCH hardware.

## Validate and package

```sh
python tools/setup_sdk.py
python tools/build.py test
python tools/build.py release
python tools/build.py debug
python tools/build.py diagnostic
python tools/package_release.py
```

Assets appear under build/releases/<firmware-version>:

- Source ZIP: the complete public source tree, documentation and license files.
- Firmware ZIP: HEX/BIN for release/debug/diagnostic, input/image manifests and notices.
- Individual release/diagnostic HEX/BIN files for convenient flashing.
- SHA256SUMS.txt for the upload assets.

The packager refuses stale build manifests or mismatched image checksums. Build
outputs, recovery archives, the SDK, editor state and historical docs are excluded
from the source ZIP. Generated debug ELF/MAP files stay local; their symbols can
contain workstation paths. Local recovery/history files have not been deleted.
Do not upload older combined SDK/recovery archives as the public source release.

## Publish manually

Use this Firmware directory (or the extracted source ZIP) as the repository root.
The existing workspace has no parent Git repository. Create an empty GitHub
repository, then initialize/commit locally if needed. Review git status before
pushing. Keep vendor/, build/, recovery/ and docs/archive/ ignored. Do not add the
nested SDK repository as an accidental Git submodule. The setup script recreates it.

Upload the freshly generated assets to a GitHub release after reviewing the
hardware checklist. Use a prerelease designation until the -5dBm range/current
candidate has been accepted. The version in config/app_config.h should match the
release title/tag and Device Information firmware revision. No packaging command
pushes commits, creates releases or uploads data.

CI runs host tests, not physical acceptance or proprietary target-toolchain
builds. Local validation cannot establish that the future GitHub workflow has
passed; inspect its run after publishing. Record actual hardware results in the
release notes, including any unmeasured limitations.
