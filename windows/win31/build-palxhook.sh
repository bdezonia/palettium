#!/bin/sh
# Separate passive observer experiment; existing binaries remain unchanged.
set -eu
cd "$(dirname "$0")/../.."
PAL_ENH_HOOK_BUILD=1 sh windows/win31/build-palddi.sh
