#!/usr/bin/env python3
"""Fail closed if the 16-bit NE module is missing essential DDI ordinals.

Input must be `wdump -x file.DRV` text. No binary modification occurs.
"""
import re
import sys

EXPECTED = {"BITBLT": 1, "COLORINFO": 2, "DISABLE": 4,
            "ENABLE": 5, "OUTPUT": 8, "PIXEL": 9, "REALIZEOBJECT": 10, "STRBLT": 11, "SCANLR": 12, "EXTTEXTOUT": 14,
            "SETPALETTE": 22, "GETPALETTE": 23, "SETPALTRANS": 24,
            "GETPALTRANS": 25, "UPDATECOLORS": 26}


def parse_exports(output):
    found = {}
    seen_ordinals = {}
    for line in output.splitlines():
        match = re.match(r"^\s*([A-Za-z][A-Za-z0-9_]*)\s+@(\d+)\s*$", line)
        if match:
            name = match.group(1).upper()
            ordinal = int(match.group(2))
            if name in found or ordinal in seen_ordinals:
                raise ValueError("duplicate NE export name or ordinal: " + line.strip())
            found[name] = ordinal
            seen_ordinals[ordinal] = name
    return found


def main():
    if len(sys.argv) != 2:
        raise SystemExit("usage: check_win31_exports.py WDUMP_EXPORTS.txt")
    with open(sys.argv[1], encoding="latin1") as stream:
        found = parse_exports(stream.read())
    failures = [f"{name} expected @{ord_}, got {found.get(name)!r}"
                for name, ord_ in EXPECTED.items() if found.get(name) != ord_]
    if failures:
        raise SystemExit("FAIL: " + "; ".join(failures))
    print("PASS: 15 Win31 display-DDI exports including StrBlt @11, ExtTextOut @14, "
          "ScanLR @12, palette @22..26")


if __name__ == "__main__":
    main()
