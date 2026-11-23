#!/bin/sh
# Win16 BITMAP structure probe; NOT an installable graphics driver.
set -eu
cd "$(dirname "$0")"
: "${WATCOM:?Set WATCOM and Open Watcom environment}"
command -v wcl >/dev/null
command -v wdump >/dev/null
wcl -bt=windows -mc -bd -zu -zq -l=windows_dll \
  -i=. -fe=PALBMP.DLL palbmp.c pal_bitmap.c pal_pdevice.c pal_gdiinfo.c
[ -s PALBMP.DLL ]
wdump -x PALBMP.DLL > PALBMP.EXPORTS.txt
grep -iq 'PALETTIUMBITMAPSELFTEST' PALBMP.EXPORTS.txt
cat PALBMP.EXPORTS.txt
printf 'PASS: PALBMP Win16 exported probe (NOT a display driver).\n'
