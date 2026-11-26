#!/bin/sh
# Optional diagnostic application, independent of the display driver build.
set -eu
cd "$(dirname "$0")/../.."
command -v wcl >/dev/null
mkdir -p tests/exes
wcl -bt=windows -ms -zq -l=windows -fe=tests/exes/PALENH.EXE \
    windows/win31/palenh.c
[ -s tests/exes/PALENH.EXE ]
printf '%s\n' 'Built tests/exes/PALENH.EXE (read-only VDD API discovery, not a driver).'
