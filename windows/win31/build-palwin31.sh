#!/bin/sh
# Combined, still explicitly gated Standard/Enhanced trial.
set -eu
cd "$(dirname "$0")/../.."
PAL_UNIFIED_BUILD=1 sh windows/win31/build-palddi.sh
