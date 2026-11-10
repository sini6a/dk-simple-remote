"""Read-only consistency check against the current adjacent KiCad PCB.

Physical pad numbers are from the official CH583M QFN48 datasheet.
This checks connectivity labels, not routing, soldering or electrical quality.
"""
from pathlib import Path
import argparse
import re
import json

ROOT = Path(__file__).resolve().parents[1]


def parse(text):
    stack, root = [], []
    current = root
    for token in re.findall(r'"(?:\\.|[^"\\])*"|[()]|[^\s()]+', text):
        if token == "(":
            node = []
            current.append(node)
            stack.append(current)
            current = node
        elif token == ")":
            current = stack.pop()
        else:
            current.append(token[1:-1] if token.startswith('"') else token)
    return root[0]


def children(node, tag):
    return [item for item in node if isinstance(item, list) and item[0] == tag]


# Pad/net of SW2 per board revision; everything else is shared.
SW2 = {"rev1": ("24", "PB0"), "rev2": ("26", "PB22")}


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--board", choices=sorted(SW2), default="rev2")
    rev = parser.parse_args().board
    board = ROOT.parent / "KiCad" / "dk-simple-remote.kicad_pcb"
    # The adjacent KiCad PCB is the current revision (rev2); older boards are
    # checked against the connectivity captured from their fabrication files.
    reference_file = ROOT / "config" / ("hardware-reference.json" if rev == "rev2"
                                        else "hardware-reference-" + rev + ".json")
    if rev == "rev2" and board.exists():
        tree = parse(board.read_text(encoding="utf-8"))
        footprints = {}
        for footprint in children(tree, "footprint"):
            refs = [p[2] for p in children(footprint, "property") if p[1] == "Reference"]
            if not refs:
                continue
            pads = {}
            for pad in children(footprint, "pad"):
                nets = children(pad, "net")
                if nets:
                    # KiCad 10 stores (net "name"); older boards also include ID.
                    pads.setdefault(pad[1], set()).add(nets[0][-1])
            footprints[refs[0]] = pads
        print("Checking adjacent PCB:", board.name)
    else:
        reference=json.loads(reference_file.read_text())
        footprints={ref:{pad:set(nets) for pad,nets in pads.items()}
                    for ref,pads in reference['footprints'].items()}
        print(f"Board {rev}: validating {reference_file.name} only; physical board is not checked")
    expected = [("SW1", "45", "PA13"), ("SW2", *SW2[rev]),
                ("SW3", "20", "PB4"), ("SW4", "22", "PB2"),
                ("SW5", "23", "PB1"), ("SW6", "21", "PB3")]
    for switch, pad, net in expected:
        assert footprints["U1"][pad] == {net}, ("U1", pad, net)
        assert footprints[switch]["1"] == {net}, switch
        assert footprints[switch]["2"] == {"GND"}, switch
        print(f"PASS {switch}: U1 pad {pad} / {net}, switch to GND")
    for pad, net in [("11", "SWDCLK"), ("12", "SWDIO"),
                     ("15", "/MCU_DP"), ("16", "/MCU_DN"),
                     ("25", "/RST#"), *([("26", "/BOOT")] if rev == "rev1" else []),
                     ("3", "+3V3"), ("34", "/RF")]:
        assert footprints["U1"][pad] == {net}, (pad, net, footprints["U1"][pad])
        print(f"PASS U1 pad {pad}: {net}")
    assert footprints["BT1"]["1"] == footprints["U1"]["3"]
    assert footprints["BT1"]["2"] == {"GND"}
    assert footprints["L1"]["1"] == footprints["U1"]["1"] == footprints["U1"]["35"]
    assert footprints["L1"]["2"] == footprints["U1"]["2"]
    assert footprints["Y1"]["1"] == footprints["U1"]["31"]
    assert footprints["Y1"]["3"] == footprints["U1"]["32"]
    print("PASS battery supply/GND, DC/DC and crystal nets; PCB checks complete")


if __name__ == "__main__":
    main()
