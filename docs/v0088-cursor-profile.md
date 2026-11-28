# v0088: Calculator cursor exclusion and low-overhead drawing counts

The maintainer reports Calculator is substantially faster with RenderTrace=0,
but still takes 2–3 seconds to lay out its buttons and the pointer flashes.
PALBOOT(20261128-021834).TXT confirms quiet mode, zero foreground/background
notifications and native repaint calls, successful shutdown, 29 SetCursor,
596 MoveCursor and 1382 CheckCursor calls. The final 32x32 mask has 898
transparent and 126 nontransparent pixels. Call counts are not physical
hide/show counts. The dominant drawing path is not yet established.

This patch preserves a visible cursor for identical packed mask/hotspot/size
requests, excludes only actual nontransparent mask coverage for rectangular
screen accesses, and narrows Pixel exclusion to the one accessed screen pixel.
The full conservative exclusion remains for ScanLR and UpdateColors. Rectangle
and polyline bounding boxes remain conservative; this patch does not alter the
line, fill, blit, text, video switching or palette algorithms. Real overlap,
including an opaque mask pixel that currently matches the background, still
requires hide/restore. No deferred batching or interrupt timing changes.

At Disable, the existing cursor snapshot now includes saturating 16-bit
counters of hide events by shape, full exclusion, overlap, and move; successful
shows; visible identical-shape reuse; and transparent-only rectangle overlaps
skipped. These count compositor events, not physical VRAM writes or exact
visible flashes. Foreground cursor discard and final unbind are not counted.
The existing trace-enabled exported DDI wrappers count Output calls by style
(scanline, rectangle, polyline, hint, other), BitBlt, text (including extent
queries), Pixel, FastBorder and DeviceBitmapBits, including failed calls.
Counts are session/module totals, not timings or Calculator-only counts;
FFFF means saturated. RenderTrace=0 does not suppress these shutdown totals.
No extra file operations during drawing or timer/mouse callbacks.

Repeat Calculator launch in a fresh Enhanced session with RenderTrace=0,
keeping the pointer outside the Calculator window. Observe speed and flicker,
then close and exit Windows to collect the totals. A separate fresh startup/
shutdown without Calculator provides a useful control if needed. Do not test
Paint yet. Native compiler and VM validation are pending.
