#!/bin/sh
# Non-installable Win3.1 GDI ABI shell; never set display.drv=PALGDI.DRV.
set -eu
cd "$(dirname "$0")"
: "${WATCOM:?Set WATCOM, PATH, INCLUDE for Open Watcom}"
command -v wcl >/dev/null
command -v wdump >/dev/null
wcl -bt=windows -mc -bd -zu -zq -l=windows_dll \
    -fe=PALGDI.DRV palgdi.c
[ -s PALGDI.DRV ] || { echo 'ERROR: DLL/DRV missing' >&2; exit 1; }
wdump -x PALGDI.DRV > PALGDI.EXPORTS.txt
cat PALGDI.EXPORTS.txt
# Confirm exports irrespective of upper/lower case.
grep -iq '^[[:space:]]*ENABLE[[:space:]]' PALGDI.EXPORTS.txt || {
    echo 'ERROR: Enable export missing' >&2; exit 1;
}
grep -iq '^[[:space:]]*DISABLE[[:space:]]' PALGDI.EXPORTS.txt || {
    echo 'ERROR: Disable export missing' >&2; exit 1;
}
echo 'PASS: PALGDI.DRV exports Enable and Disable; NOT safe to install.'
