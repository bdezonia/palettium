#!/bin/sh
# Compile consolidated Win3.1 DDI objects; NOT an installable display driver.
set -eu
cd "$(dirname "$0")"
command -v wcc >/dev/null
mkdir -p ../../tests/exes
for source in palddi pal_enable pal_gdiinfo pal_pdevice; do
    wcc -bt=windows -mc -bd -zu -zq -i=../../include -i=. \
        -fo=../../tests/exes/"$source".obj "$source".c
done
for source in palddi pal_enable pal_gdiinfo pal_pdevice; do
    test -s ../../tests/exes/"$source".obj
done
echo 'PASS: Win16 DDI and capability-stage objects compiled (NOT installable).'
