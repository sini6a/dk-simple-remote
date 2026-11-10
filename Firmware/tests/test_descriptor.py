"""Validate the actual HID descriptor's report lengths/collections/pages.
No vendor or transport mocks; parses HID short items, not C comments.
"""
from pathlib import Path
import re

source = (Path(__file__).resolve().parents[1] / "src/ble/hid_service.c").read_text()
array = re.search(r"static uint8_t descriptor\[\] = \{(.*?)\};", source, re.S).group(1)
blob = bytes(int(n, 16) for n in re.findall(r"0x([0-9a-fA-F]+)", array))
size = count = page = report = depth = 0
bits, pages = {}, set()
at = 0
while at < len(blob):
    tag = blob[at]; at += 1
    assert tag != 0xfe, "Unexpected long item"
    length = (0, 1, 2, 4)[tag & 3]
    assert at + length <= len(blob)
    value = int.from_bytes(blob[at:at+length], "little"); at += length
    kind, number = (tag >> 2) & 3, tag >> 4
    if kind == 1:
        if number == 0: page = value
        elif number == 7: size = value
        elif number == 8:
            report = value
            assert report in (1, 2)
        elif number == 9: count = value
    elif kind == 0:
        if number == 10: depth += 1
        elif number == 12: depth -= 1
        elif number in (8, 9, 11):
            assert depth > 0 and size and count and report
            bits[report, number] = bits.get((report, number), 0) + size * count
            if number == 8: pages.add((report, page))
        assert depth >= 0
assert depth == 0
assert bits == {(1, 8): 64, (2, 8): 16}, bits
assert (1, 7) in pages and (2, 12) in pages, pages
print("PASS: HID descriptor IDs, balanced collections, keyboard/consumer pages, 8-byte/2-byte inputs")
