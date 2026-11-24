#!/bin/sh
# Build Win16 PALLOAD.EXE: safe, opt-in loader for STAGED PALSTAGE.DRV.
set -eu
cd "$(dirname "$0")/../.."
command -v wcl >/dev/null
mkdir -p tests/exes
wcl -bt=windows -ms -zq -l=windows -i=windows/win31 \
    -fe=tests/exes/PALLOAD.EXE windows/win31/palload.c
[ -s tests/exes/PALLOAD.EXE ]
printf '%s\n' 'Built tests/exes/PALLOAD.EXE (Win16 driver-load probe, no mode changes).'
