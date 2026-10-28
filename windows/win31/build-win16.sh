#!/bin/sh
set -eu
cd "$(dirname "$0")"
: "${WATCOM:?Set WATCOM to your Open Watcom installation root and set PATH appropriately}"
wcc -bt=windows -mc -zu -zq -i=../../include -fo=PALPROBE.OBJ palprobe.c
wlink @PALPROBE.LNK
printf 'Created %s/PALPROBE.DLL (toolchain probe only, NOT a display driver)\n' "$PWD"
