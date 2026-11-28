#!/bin/sh
# Opt-in native USER repaint experiment, separate from validated PALXHOOK.
set -eu
cd "$(dirname "$0")/../.."
PAL_ENH_REPAINT_BUILD=1 sh windows/win31/build-palddi.sh
