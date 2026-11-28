#!/usr/bin/env python3
"""Require the private diagnostic export in PALXHOOK v0080 only."""
import sys
from check_win31_exports import parse_exports

if __name__ == '__main__':
    if len(sys.argv) != 2:
        sys.exit('usage: check_win31_hook_export.py WDUMP_EXPORTS.txt')
    with open(sys.argv[1], encoding='latin1') as f:
        if parse_exports(f.read()).get('PALHOOKSTATUS') != 501:
            sys.exit('FAIL: PALHOOKSTATUS must be named export @501')
    print('PASS: PALHOOKSTATUS @501')
