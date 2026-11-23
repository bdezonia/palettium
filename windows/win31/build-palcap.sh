#!/bin/sh
# Build a Win16 layout-probe DLL; never install it as a video driver.
set -eu
cd "$(dirname "$0")"
: "${WATCOM:?Set WATCOM and Open Watcom PATH first}"
command -v wcl >/dev/null
command -v wdump >/dev/null
wcl -bt=windows -mc -bd -zu -zq -l=windows_dll \
  -i=. -fe=PALCAP.DLL palcap.c pal_gdiinfo.c
[ -s PALCAP.DLL ]
wdump -x PALCAP.DLL
printf 'Built PALCAP.DLL (Win16 ABI probe; NOT a display driver).\n'
