# v0023: unified Win16 staging link

The previous milestones compiled Win16 GDI objects but did not **link** the
complete adapter and shared core into one NE module. This change adds a single
Win16 build that includes the Palettium renderer, GDI translation, screen host,
capability helpers, and the Win16 DDI entry points.

Run:

    make -f Makefile.win31link test
    sh windows/win31/build-palddi.sh

Outputs (all ignored by Git using `/tests/exes/`):

    tests/exes/PALSTAGE.DRV
    tests/exes/PALSTAGE.exports.txt
    tests/exes/*.obj

The export validator rejects any disagreement with the known initial ordinals:
BitBlt @1, Disable @4, Enable @5. All other GDI exports and their ordinals
must be verified against the DDK sample before a working driver is installed.

## Deliberately not installable

`Enable(style=1)` returns only a preliminary capability description.
`Enable(style=0)` deliberately returns failure. There is no active hardware
binding, full palette/brush/font implementation, or enhanced-mode screen
switching. **Never install PALSTAGE.DRV or change SYSTEM.INI to use it.**

The Win16 build is unverified until an actual Open Watcom run completes.
A link-time error indicates missing compiler/runtime assumptions or an invalid
C89/Win16 build; report the full output rather than trying unrelated libraries.

### Architecture note

The common renderer and hardware interface remain OS-agnostic. The module
entry, PDEVICE, GDIINFO and BitBlt DDI signature remain Windows-specific.
Do not add Tseng register code to this directory.
