#!/bin/sh
# Compile the consolidated Windows 3.1 DDI staging object.
# This is NOT a complete, installable display driver.
set -eu
cd "$(dirname "$0")"
command -v wcc >/dev/null
mkdir -p ../../tests/exes
wcc -bt=windows -mc -bd -zu -zq -i=../../include -i=. \
    -fo=../../tests/exes/palddi.obj palddi.c
[ -s ../../tests/exes/palddi.obj ]
echo 'PASS: consolidated Win16 DDI object compiled (NOT installable).'
