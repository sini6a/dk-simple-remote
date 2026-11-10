.PHONY: all release debug diagnostic test sdk package rev1 boards
# BOARD=rev1 builds for the 2026-10-28 boards (SW2 on PB0); default rev2 (SW2 on PB22/BOOT).
BOARD ?= rev2
PYTHON ?= python
all: release
release:
	$(PYTHON) tools/build.py release --board $(BOARD)
debug:
	$(PYTHON) tools/build.py debug --board $(BOARD)
diagnostic:
	$(PYTHON) tools/build.py diagnostic --board $(BOARD)
test:
	$(PYTHON) tools/build.py test

sdk:
	$(PYTHON) tools/setup_sdk.py
rev1:
	$(MAKE) release debug diagnostic BOARD=rev1
# Build both revisions, then package them together.
boards:
	$(MAKE) release debug diagnostic BOARD=rev2
	$(MAKE) rev1
package:
	$(PYTHON) tools/package_release.py
