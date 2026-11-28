#!/bin/sh
set -eu
cd "$(dirname "$0")/../.."
command -v wcl >/dev/null
mkdir -p tests/exes
wcl -bt=windows -ms -zWs -zq -l=windows -fe=tests/exes/PALWATCH.EXE windows/win31/palwatch.c
test -s tests/exes/PALWATCH.EXE
printf '%s\n' 'Built tests/exes/PALWATCH.EXE (independent live hook monitor).'
