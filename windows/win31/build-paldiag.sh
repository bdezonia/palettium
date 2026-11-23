#!/bin/sh
# Build the OPTIONAL, non-invasive Windows 3.1 GUI diagnostic.
# This is a Win16 EXE (not PALSTAGE.DRV, and not a mode-setting test).
set -eu
cd "$(dirname "$0")/../.."
command -v wcl >/dev/null
mkdir -p tests/exes
wcl -bt=windows -ms -zq -l=windows \
    -fe=tests/exes/PALDIAG.EXE windows/win31/paldiag.c
[ -s tests/exes/PALDIAG.EXE ]
printf '%s\n' 'Built tests/exes/PALDIAG.EXE (Win16 GUI; read-only by default).'
