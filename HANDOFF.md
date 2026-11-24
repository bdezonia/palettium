# Palettium v0038 handoff — GDI-owned PDEVICE staging (2026-11-23)

## Verified baseline

The maintainer validated v0037 on Ubuntu/Open Watcom on 2026-11-23:
`make clean && make test`, `Makefile.win31native test`,
`Makefile.win31link test`, `Makefile.screen17 test`, `Makefile.et4k test`,
`sh windows/win31/build-palddi.sh` all PASS; `wdump` showed the 13
expected NE exports. No compiler/linker warnings were shown. Earlier
`ET4KTEST INFO` on 86Box confirmed CRTC37h = 0Dh/0Eh/0Fh for 256/512/1024
KiB across BIOS V8.01X, V8.06X, V1.21X (9/9).
PALDIAG's protected-mode A000h read succeeded; `SYSTEM.INI [386Enh]`
reported `display=*vddvga` (configuration, not proof of loaded VDD).
Stock Windows 3.1 VGA VDD is documented incompatible with SVGA framebuffers
larger than 256 KiB; its handling of 640x400x8 is **not verified**.

## v0038 implementation

1. The **GDI-owned far PDEVICE** vs private near host PDEVICE gap is now
   addressed at the publication boundary. Existing
   `pal_screen_host_gdi_snapshot(host, token, &image)` checks the live DDI,
   physical layout/state and exact far-token identity, then copies the
   20-byte PDEVICE image *without* any BIOS, port or VRAM operations. It
   refuses inactive, malformed or mismatched state and leaves destination
   unchanged upon failure.
2. `palddi.c` replaces the unsafe unchecked `pal_win16_bind_host` placeholder
   with `pal_win16_publish_host`. The new Win16-only helper checks the actual
   destination far pointer (including 64K segment wrap), obtains the
   validated image, marshals bytes through an `unsigned char FAR *`, then
   exposes the DDI host. `Disable` now rejects unrelated PDEVICE tokens.
   The GDI-owned allocation is **not** treated as a near C pointer.
   Neither function is part of the 13 DDI exports. The publisher is
   **dormant** until a fully supported style=0 hardware lifecycle exists.
3. The Win3.x `GDIINFO` candidate's previously zero standard metrics have
   been populated with a coherent 96-DPI mapping, rounded physical mm
   dimensions, 100:100:141 aspect, style length 282, and low/high metric,
   low/high English, and twips mapping ratios. The record remains a
   candidate: no unsupported capability flags are set.
4. Existing ET4000AX banked-host regression now verifies live/stale/wrong
   PDEVICE export and no output mutation on refusal. The GDIINFO regression
   checks 640x400 and 640x480 mappings and 16-bit numerical bounds. The
   project `make test` runs GDIINFO checks; build-maintenance regression
   prevents restoring unchecked Win16 host binding.

## Testing

Assistant Linux host: `make clean && make test`, Win31 native/link,
screen17, ET4000/bitmap and all other auxiliary make test targets PASS.
Clean source-only patch over v0037 and `sha256sum -c` PASS; UTC extended
ZIP 0x5455 and America/Chicago DOS timestamps verified. **Open Watcom
Win16 compilation/link and any Windows 3.1 runtime test of v0038 are
not yet performed**. Maintainer commands:

```sh
cd ~/dev/palettium
unzip -o ~/Downloads/pal38.zip
make clean && make test
make -f Makefile.win31native test
make -f Makefile.win31link test
make -f Makefile.screen17 test
sh windows/win31/build-palddi.sh
wdump -x tests/exes/PALSTAGE.DRV
```

## Safety and next work

**PALSTAGE.DRV is NOT INSTALLABLE.** `Enable(style=0)` still returns zero.
No VDD switching, BIOS setting, GDI-owned session lifetime, cursor or text
service is activated. A successfully copied GDI PDEVICE is necessary but
not sufficient for GDI startup. Avoid installing the staging driver or
changing a working Windows VM. Next, prioritize an explicitly tested VDD
strategy, missing mandatory DDIs and a reversible GDI `Enable/Disable`
implementation. Keep common C89 rendering, the ET4000AX card backend and
four separate thin OS adapters (Win3.1, WfW3.11, Win95, Win98). Keep
fixed-width v#### tags, stable production names, no duplicate raster
implementations, `tests/exes` output, full `make clean`, and small UTC-
timestamped source-only ZIP patches.
