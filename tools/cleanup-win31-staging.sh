#!/bin/sh
# Remove obsolete v0015/v0020 prototypes now superseded by palddi.c.
# Safe to rerun. This script does not touch the currently working ET4000
# backend or any original Win16 runtime/core test programs.
set -eu
cd "$(dirname "$0")/.."
for path in \
    windows/win31/palddi15.c \
    windows/win31/palddi20.c \
    windows/win31/palgdi.c \
    windows/win31/build-palddi15.sh \
    windows/win31/build-palddi20.sh \
    windows/win31/build-palgdi.sh \
    windows/win31/PALDDI15.DRV \
    windows/win31/PALDDI15.EXPORTS.txt \
    windows/win31/PALGDI.DRV \
    windows/win31/PALGDI.EXPORTS.txt \
    tests/exes/palddi20.obj \
    Makefile.far20 \
    makefiles/Makefile.far20
 do
    if [ -e "$path" ]; then rm -f "$path"; echo "removed $path"; fi
 done
# Leave older historical documentation in place; new instructions are in
# docs/v0021-driver-cleanup.md.
echo 'Cleanup complete. Review git status --short before committing.'
