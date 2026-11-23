# Palettium v0035 handoff — Win3.1 VDD configuration advisory (2026-11-23 UTC)

## User-validated baseline

- v0031 `make clean`, all host C89 tests and Open Watcom `PALSTAGE.DRV`
  compile/link **PASS**, with the same 13 staging display-DDI exports.
- `ET4KTEST.EXE` displayed the expected graphics in 86Box DOS with 256,
  512 and 1024 KiB configured ET4000AX VRAM.
- v0032 first Win16 PALDIAG had GPF in `KRNL386.EXE 0001:2272` before a
  dialog; precise cause was not established. v0033 PALDIAG was successful:
  `GetModuleHandle("KERNEL")=010F`, `__A000H=FFFF:1027`,
  `GetWinFlags=00000C29` (386 Enhanced), optional BIOS INT 10h AH=0Fh
  returned mode `12h`, 80 columns, page 0.
- **v0034 PASS in Windows 3.1:** user's screenshots show PALDIAG STEP 5A
  announcing a single A000h:0000h byte read with selector `1027h`, and
  STEP 5B returning **00h** without a GP fault. This confirms one readable
  protected-mode A000h byte mapping in that VM, **NOT** physical identity,
  8-bit banking, writes, VDD compatibility or mode switching.

## New research finding

Microsoft KB80901 says Windows 3.1's standard VGA VDD demand-pages video
RAM and is incompatible with SVGA display drivers using **more than 256 KiB**.
The alternative is a manufacturer's VDD or bundled `VDDVGA30.386`, but
compatibility must be measured, not assumed. Historical VGA DDK research
also finds `VDDsetaddresses` / `DspDrvr_Addresses` service `0Ch` used in
VDD cooperation and important during DOS-window mode switches. References:
- https://www.betaarchive.com/wiki/index.php/Microsoft_KB_Archive/80901
- https://www.os2museum.com/wp/windows-3-x-vddvga/
- https://dos-help.soulsphere.org/ddag31qh.hlp/dispinit.html

## v0035 scope

- Same stable `windows/win31/paldiag.c` → `tests/exes/PALDIAG.EXE`.
  v0033 STEP 0–4 and v0034 explicitly confirmed STEP 5 are preserved.
  Declining STEP 5 now proceeds to **optional STEP 6** instead of exiting.
- STEP 6 uses only Win16 `GetPrivateProfileString` to read
  `SYSTEM.INI [386Enh] display=`. The result is only a **configuration
  observation**, NOT evidence that the listed VDD is currently loaded.
- New `windows/win31/pal_vdd.[ch]`: C89 Win3.x-only classifier. Recognizes
  `*vddvga` and `VDDVGA.386` as stock, `VDDVGA30.386` as legacy, and
  treats anything else as custom or unknown. In enhanced mode, if stock
  is configured and framebuffer >262144 bytes, returns
  `PAL_VDD_KNOWN_INCOMPATIBLE`. All other enhanced cases return
  `PAL_VDD_UNVERIFIED`; no config string certifies compatibility.
  640×480×8 uses 307200 bytes, 640×400×8 uses 256000 bytes. The latter
  does not imply that a stock VGA VDD supports banked packed SVGA.
- Host test compiles actual Win16 PALDIAG plus the classifier against mock
  Win16 APIs. Includes independent BIOS/VRAM/VDD opt-ins, missing export,
  truncation and static VDD classification cases. No actual VDD calls,
  hardware access, mode setting or configuration modification in STEP 6.
- Same 13 DDI exports; `PALSTAGE.DRV` is **NOT INSTALLABLE** and not modified.
  Never select it in SYSTEM.INI. Native Win16 ET4000AX driver adapter is
  still dormant; no new hardware activation or Windows VDD handshake.

## What to run

```sh
cd ~/dev/palettium
unzip -o ~/Downloads/pal35.zip
make -f Makefile.win31diag test
sh windows/win31/build-paldiag.sh
```

In snapshot-restorable 86Box Windows 3.1 **with the existing working
VGA driver**, replace the previous PALDIAG.EXE. Repeat STEP 0–3, optionally
skip BIOS STEP 4 and **decline STEP 5** (one-byte VRAM read has already
passed). Answer Yes at STEP 6 and record the entire dialog showing the
configured VDD. Do not edit SYSTEM.INI or switch modes for this test.
The `GetPrivateProfileString` Win16 application build and real Windows
readback must be validated by the maintainer; host mocks cannot prove either.

## Future priority

Confirm actual VDD configuration and a compatible VDD/grabber pairing, and
study the `VDDsetaddresses` service contract against historical DDK sources.
Then implement a safe, persistent GDI-owned PDEVICE and a controlled native
mode lifecycle with VDD coordination and rollback. Keep shared C89 renderer,
card-specific backend, four-OS architecture, fixed-width v#### tags, source-
only incremental ZIPs, correct UTC extra/Chicago-local DOS ZIP timestamps,
`tests/exes` output, comprehensive `make clean`, and strict distinction
between host/Open Watcom/86Box results.
