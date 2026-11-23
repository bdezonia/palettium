#!/bin/sh
set -eu
# Execute from repo root: sh dos/build-et4k.sh
# WATCOM and PATH must already point to Open Watcom v2.
mkdir -p build/dos
wcl -bt=dos -ml -zq -i=include -i=hardware/tseng/et4000ax \
    -fe=build/dos/ET4KTEST.EXE \
    dos/et4k_demo.c common/rgfx.c common/rg_hardware.c \
    hardware/tseng/et4000ax/et4k.c
printf '%s\n' 'Built build/dos/ET4KTEST.EXE (16-bit DOS).'
