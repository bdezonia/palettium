#!/bin/sh
# Win16 DDI ABI experiment; this binary is intentionally NOT installable.
set -eu
cd "$(dirname "$0")"
: "${WATCOM:?WATCOM must be set}"
command -v wcl >/dev/null
command -v wdump >/dev/null
# # Win16 startup is supplied by wcl. This probe does NOT assign DDI ordinals.
wcl -bt=windows -mc -bd -zu -zq -l=windows_dll \
    -fe=PALDDI15.DRV palddi15.c
[ -s PALDDI15.DRV ]
wdump -x PALDDI15.DRV > PALDDI15.EXPORTS.txt
cat PALDDI15.EXPORTS.txt
# This is a compile/link smoke test only; the shell's automatic export
# ordinals are NOT the production DDI ordinal map.
echo 'PALDDI15 compiled. Verify ordinals before using as a reference.'
echo 'NOT INSTALLABLE; Enable always fails.'
