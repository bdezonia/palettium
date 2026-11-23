#!/usr/bin/env python3
"""Fail closed if the 16-bit NE module is missing essential DDI ordinals.

Input must be `wdump -x file.DRV` text. No binary modification occurs.
"""
import re
import sys

EXPECTED = {"BITBLT": 1, "DISABLE": 4, "ENABLE": 5}


def parse_exports(output):
    found = {}
    for line in output.splitlines():
        match = re.match(r"^\s*([A-Za-z][A-Za-z0-9_]*)\s+@(\d+)\s*$", line)
        if match:
            found[match.group(1).upper()] = int(match.group(2))
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
    print("PASS: DDI export ordinals: BitBlt @1, Disable @4, Enable @5")


if __name__ == "__main__":
    main()
