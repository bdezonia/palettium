#!/bin/sh
# Experimental Enhanced desktop startup; never a replacement for PALTRIAL.
set -eu
cd "$(dirname "$0")/../.."
PAL_ENH_STARTUP_BUILD=1 sh windows/win31/build-palddi.sh
