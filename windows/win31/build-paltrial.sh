#!/bin/sh
# Explicitly experimental Win3.1 Standard-mode test driver (not production).
set -eu
cd "$(dirname "$0")/../.."
PAL_TRIAL_BUILD=1 sh windows/win31/build-palddi.sh
