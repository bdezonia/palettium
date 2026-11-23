#!/bin/sh
# Compiles Win16 DDI signature and pointer marshalling object only.
# Not an installable DRV: Enable, drawing and VDD integration are incomplete.
set -eu
cd "$(dirname "$0")"
command -v wcc >/dev/null
mkdir -p ../../tests/exes
wcc -bt=windows -mc -bd -zu -zq -i=../../include -i=. \
    -fo=../../tests/exes/palddi20.obj palddi20.c
echo 'PASS: PALDDI20 Win16 DDI object compiled (NOT installable).'
