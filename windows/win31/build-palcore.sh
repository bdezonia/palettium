#!/bin/sh
# Build cross-platform Palettium raster + hardware + ET4000AX code as Win16 DLL.
# This is a compatibility probe, NOT a GDI display driver. Never install it.
set -eu
cd "$(dirname "$0")"
: "${WATCOM:?Set WATCOM and PATH for Open Watcom v2}"
command -v wcl >/dev/null || { echo 'Open Watcom wcl not on PATH' >&2; exit 1; }

# WCL automatically supplies the Win16 DLL startup/runtime; see LibMain.
# Separate output names prevent overwriting the original PALPROBE test.
wcl -bt=windows -mc -bd -zu -zq -i=../../include \
    -i=../../hardware/tseng/et4000ax \
    -fe=PALCORE.DLL -l=windows_dll \
    palcore.c ../../common/rgfx.c ../../common/rg_hardware.c \
    ../../hardware/tseng/et4000ax/et4k.c

if [ ! -s PALCORE.DLL ]; then
    echo 'ERROR: PALCORE.DLL missing or empty' >&2
    exit 1
fi
printf '%s\n' 'Built PALCORE.DLL: Win16 core link probe (NOT a display driver).'
printf '%s\n' 'Inspect its NE exports before treating the self-test as callable.'
