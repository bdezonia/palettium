#!/bin/sh
# Build Win16 private PDEVICE ABI probe; NOT installable as a display driver.
set -eu
cd "$(dirname "$0")"
: "${WATCOM:?Set WATCOM, PATH and INCLUDE for Open Watcom}"
command -v wcl >/dev/null
command -v wdump >/dev/null
wcl -bt=windows -mc -bd -zu -zq -l=windows_dll \
  -i=. -fe=PALDEV.DLL paldev.c pal_pdevice.c pal_gdiinfo.c
[ -s PALDEV.DLL ]
wdump -x PALDEV.DLL > PALDEV.EXPORTS.txt
grep -iq 'PALETTIUMPDEVICESELFTEST' PALDEV.EXPORTS.txt
cat PALDEV.EXPORTS.txt
printf 'PASS: PALDEV Win16 probe linked/exported (NOT a display driver).\n'
