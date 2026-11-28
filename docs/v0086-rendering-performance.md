# v0086: drawing performance and application compatibility triage

The v0085 user reports that Enhanced-mode startup works, Calculator draws
buttons very slowly with repeated cursor flashing, and Paint opens black and
hangs when drawing a filled rectangle. The Standard-mode trace
PALBOOT(20261128-015503).TXT contains two UNIFIED STANDARD ROUTE entries,
flags 00000419, VRAM 0200, KEY 0000 and GATE BLOCKED. There is no physical
activation. The existing Standard gate requires TrialEnable=4401 (decimal)
in the single [Palettium] section of the guest SYSTEM.INI. No gate or
activation changes are warranted by this trace. Retest with the correct key.

Two pre-existing rendering inefficiencies (also present in v0083) are
addressed independently of the activation route:

- Shared GDI Output uses the fill operation for clipped solid COPY
  rectangles and scanlines, including disjoint border/interior boxes.
  Explicit fill overrides take precedence, but a default software fill
  cannot bypass an overridden pixel operation. Other ROP2s and hatch
  semantics retain the existing pixel path. This is not a new ET4000-only
  renderer or an acceleration-specific implementation.
- Win16 Output and text wrappers hold the existing compositor guard, but
  remove the cursor only when validated, screen-clipped drawing bounds
  intersect its saved-under rectangle. Memory-only targets are excluded
  from this test. Text uses the validated text and opaque bounds; scanline
  lists use their individual spans, and polylines a conservative bounding
  box. No interrupt handler, cursor core, or repaint algorithm changes.

Host validation compares 21,504 rectangle cases with the previous pixel
path (clipped/empty/skinny, distinct pen and brush, hollow and absent
objects), verifies fill dispatch and non-COPY fallback, and observes actual
text and Output wrappers avoiding writes at a nonoverlapping cursor pixel.
An overlapping scanline restores the saved-under color correctly. Existing
mono, banked screen, and all other host regression suites pass. Native
Watcom compilation and Windows runtime testing remain pending.

Paint remains unresolved. The current memory DDB validator explicitly
rejects storage exceeding 65,535 bytes or nonzero bmSegmentIndex. Large
color canvases may therefore hit unsupported huge-bitmap handling, but no
Enhanced-mode Paint trace has been supplied and the hang is not diagnosed.
Collect the trace from an isolated Paint session, preferably also compare
with the unmodified v0083 PALXRPT driver. Do not infer a huge-bitmap failure
from the Standard-mode startup log. Compare Calculator under the same
conditions, with the pointer away from the buttons.

Build and resource-bind PALWIN31.DRV as in v0085. Keep the previous binary
and use a disposable VM. Verify the Standard authorization before startup.
Test Calculator and Paint separately in Enhanced mode, preserving each
PALBOOT.TXT even if reset is needed. No new profile keys are introduced.
