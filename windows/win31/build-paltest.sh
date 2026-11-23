#!/bin/sh
# Build the ordinary Win16 PALTEST.EXE application (not a display driver).
set -eu
cd "$(dirname "$0")"
: "${WATCOM:?Set WATCOM and PATH for Open Watcom v2}"
command -v wcl >/dev/null || {
    echo 'Open Watcom wcl not on PATH' >&2
    exit 1
}

if [ ! -s PALCORE.DLL ]; then
    echo 'PALCORE.DLL missing; first build it with build-palcore.sh.' >&2
    exit 1
fi

# Build a Windows 3.x NE application, as opposed to the Windows DLL target.
wcl -bt=windows -ms -zq -l=windows -fe=PALTEST.EXE paltest.c

if [ ! -s PALTEST.EXE ]; then
    echo 'ERROR: PALTEST.EXE missing or empty' >&2
    exit 1
fi
printf '%s\n' 'Built PALTEST.EXE (Win16 application; not a display driver).'
