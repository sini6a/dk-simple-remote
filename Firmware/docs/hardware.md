# Supported hardware

Custom CH583M QFN48 remote PCB, directly powered by a CR2032 at VDD33.
This firmware repository does not include the PCB layout or fabrication files.
config/hardware-reference.json (rev2) and config/hardware-reference-rev1.json
record each revision's relevant pad/net connections so standalone builds can
validate the reference without a local PCB. They are not a replacement for a
schematic or assembly/electrical inspection.

## Board revisions

Flash the firmware that matches the board; the only firmware-visible
difference is SW2.

| Revision | Identify by | SW2 | Build |
|---|---|---|---|
| rev1 | Fabricated 2026-10-28; solder jumpers JP1 (RESET) and JP2 (BOOT) | PB0, pad 24 | `python tools/build.py release --board rev1` (files end in `-rev1`) |
| rev2 | Current PCB; no JP1/JP2 | PB22 (BOOT), pad 26 | `python tools/build.py release` |

A rev2 image on a rev1 board leaves SW2 dead, and a rev1 image on a rev2 board
does the same; no other key or function is affected.

| Switch | MCU pad | GPIO |
|---|---|---|
| SW1 | 45 | PA13 |
| SW2 | 24 / 26 | rev1: PB0; rev2: PB22 (BOOT) |
| SW3 | 20 | PB4 |
| SW4 | 22 | PB2 |
| SW5 | 23 | PB1 |
| SW6 | 21 | PB3 |

All switches connect to ground and use MCU input pull-ups. Only these switch
pins are configured for falling-edge wake. On rev2, SW2 is on the BOOT pin (PB22): hold it
while inserting the battery to enter the WCH bootloader (on rev1, bridge JP2
instead). Its wake interrupt uses
the RB_PIN_INTX remap, so PB8 cannot also be used as an interrupt. USB, debug,
reset and clock/RF pins must remain compatible with the actual hardware.

The assembly has a 22uH DC/DC inductor and a YXC X201632MMB4SI 32MHz crystal
with 10pF specified load. C6/C7 are not fitted. Firmware leaves oscillator load
settings unchanged. DC/DC is enabled before switching to the 60MHz PLL clock;
the SDK sleep driver preserves the DC/DC enable bits.

Internal VBAT measurement uses the direct battery supply; no external divider
is required for this board. The ADC gain conversion includes the datasheet's
negative-gain DC offsets. Offset-test calibration and nominal reference scaling
are implemented, but absolute board accuracy is not calibrated.

Connectivity checks compare the six switches, supply, DC/DC, crystal, USB,
debug and boot/reset nets. They cannot verify component quality, soldering,
inductor DCR/current rating, RF layout or conversion efficiency. Measure actual
current and range before claiming battery lifetime or production qualification.
