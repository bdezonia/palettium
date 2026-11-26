# Mandatory Windows 3.1 386 Enhanced-mode phase

Baseline: v0073 runtime-validated in Standard mode; v0074 text patch host-validated.
This diagnostic-only milestone neither enables Enhanced mode in Palettium nor
changes its display driver binaries. Enhanced mode is a mandatory deliverable,
not an optional stretch goal. Further raster performance work is deferred.

## Design evidence and selection

The installed DRV, VDD and Enhanced grabber are a cooperating set. The 3.1 DDK
compatibility section says not to mix a 3.0 VDD with an incompatible 3.1 grabber.
KB Q80901 explains the stock Win3.1 VGA VDD's >256 KiB incompatibility. The
Palettium 640x480x8 framebuffer uses 307200 bytes, so `*vddvga` is excluded
for this target. A 256000-byte mode is not automatically compatible.

Prefer the existing, matching 256-color ET4000 OEM/Microsoft SVGA driver set
as the first control experiment, rather than writing a whole VDD before
understanding the interface. Microsoft's 1993 generic SVGA update supports
ET4000; its notes report DOS screen-switching problems for some ET4000 boards
and suggest its ET4000-specific installation options. `VDDSVGA.386` and
`VGADIB.3GR` are a documented candidate pair, not an assertion that the
maintainer has them or that this pair works with Palettium. If a different
OEM driver is used, retain the exact pair from its installation. A successful
donor run establishes a usable baseline for that donor only. Do not ship any
proprietary binaries or change the user's working Standard-mode configuration.

The documented INT 2Fh/AX=1684h/BX=000Ah query returns a protected-mode VDD API
entry. PALENH records ES:DI and refuses to call it. A zero selector is unusable
(even with nonzero DI, per KB Q120079). A nonzero selector is only an address,
not proof of identity, compatibility or ownership. Runtime discovery and
configuration are independent evidence.

The DDK describes 4000h/4007h as enabling/disabling assisted save/restore,
including changes to trapping, and 4001h/4002h and 4005h/4006h notifications.
These are not pure queries. Do not use 4000h as a passive probe. Historical
investigation of VGA VDD source found that actual callback use may differ
from the documentation. The 3.1 display-driver private API 0Bh/0Ch also
requires attention: the VGA driver and SVGA256 driver use display-driver
registration services; the 0Ch address registration can prevent harmful
video-memory sharing in the stock VDD. The exact calling convention and
accepted parameters must be checked against the selected VDD and donor,
not inferred from a similar driver. Never call an unknown private API.

## v0075 evidence collection (one optional EXE, no driver rebuild)

Build from repository root:

```
make -f Makefile.win31enh test
sh windows/win31/build-palenh.sh
```

Use a separate, recoverable 86Box snapshot of the same ET4000AX (512 KiB).
First configure a known-good native ET4000 640x480 256-color donor using its
own setup procedure, with the matching VDD and grabber. Boot `WIN /B /3`.
Do not substitute PALTRIAL for the donor in Enhanced mode yet.
Run `PALENH.EXE` through Program Manager File > Run. Consent to write the
report, then consent to the API lookup if offered. It reads SYSTEM.INI and
GetWinFlags, records a closed pre-interrupt checkpoint, and only in actual
protected Enhanced mode issues the API discovery interrupt. It does not call
that address, read A000, use INT 10h, touch I/O ports or set video mode.
Return `C:\PALENH.TXT`, the relevant SYSTEM.INI entries, and the donor set's
names/version or source (ideally SHA-256 of local expanded DRV/VDD/3GR).
Keep `C:\PALBOOT.TXT` separate. If the query faults, recover the VM and return
the partial PALENH.TXT (ABOUT_TO_CALL with no VDD_API_ES_DI). If the donor
itself fails, do not change Palettium to compensate: find a matching donor.

Exercise donor: normal desktop, windowed DOS prompt, `MODE CO80` in the
window, Alt+Enter to full-screen and back, switch repeatedly, then exit to
DOS. Report screen corruption, palette changes, or failures. Record whether
these work before introducing Palettium. The exact machine and VDD binary
are not available to this environment, so no runtime compatibility claim.

## Implementation milestones, in order

1. Obtain donor and PALENH evidence. Establish one precise VDD/grabber
   contract. Examine the public DDK source and local donor API use (lookup,
   registration, mode/ownership notifications, teardown). Separate API
   discovery from authorization; never classify an INI name as sufficient.
2. Implement a Win3.x-only platform/VDD bridge using caller-owned context
   and a tested native register-preserving wrapper, with reversible lifecycle.
   Keep all VMM knowledge out of RG_Device and shared raster functions.
   Test the registration and teardown on an experimental build; preserve
   PALSTAGE fail-closed and original Standard path. A separate explicit
   Enhanced test option is needed before any hardware startup is allowed.
3. Model drawing permission/ownership transitions. Before loss of access,
   suspend screen cursor composition (restore its saved background while
   access is valid), prevent drawing into the wrong VM, and coordinate
   register and framebuffer preservation. On reacquisition, restore the
   known mode, DAC/palette state and independent ET4000 read/write banks or
   invalidate the cached bank pair before the first access. Revalidate the
   aperture; decide from VDD contract whether video memory is restored or
   repaint is needed. Do not blindly reset the mode and destroy saved VRAM.
   Keep ordinary memory bitmap rendering independent of screen ownership.
4. Implement only the required native callback/hook ABI, preserve chaining,
   register/segment state and interrupt safety. No DOS trace writes in
   callbacks; defer diagnostic counters and USER repaint requests to a safe
   context. Audit ordinal 500 repaint suppression and reenable behavior.
   Verify rollback on each startup failure; restore hooks and registration
   on exit. Maintain KERNEL-only NE import audit for the DRV.
5. Run disposable-VM startup/shutdown, windowed/full-screen DOS tests,
   focus changes, repeated switches, screen/palette restoration, cursor,
   bitmap and menu tests, and unchanged Standard-mode regression. A single
   successful Enhanced desktop is a milestone, not completion.
6. If the chosen donor VDD cannot satisfy the contract, implement a separate
   native ET4000 VDD component for bank/port and memory virtualization. Do
   not redirect to VESA or insert card-specific hacks in shared rendering.
   Reuse this OS bridge architecture in WfW; Win95/98 have different VDD
   architecture and will be addressed separately.

## Source audit (v0073/v0074)

- `pal_trial_gate.c`: current admission is protected Standard, key 4401,
  256 or 512 KiB; unchanged.
- `pal_vdd.c`: configuration-only classifier; unchanged.
- `pal_et4k_session.c`: publication transaction requires NOT_ENHANCED;
  `palddi.c` independently checks before adapter lookup. Both stay gated.
- `pal_et4k_win16.c`: BIOS and A000/port implementation lacks VDD coordination.
- `hardware/tseng/et4000ax/et4k.c`: separate cached read/write banks; must
  handle external state changes under Enhanced ownership. No need to change
  its drawing primitives to discover the VDD contract.
- `pal_cursor16.c`, `palddi.c`: interrupt mailbox and compositor; ownership
  transition needs explicit coordination. UserRepaintDisable currently no-op
  because no USER repaint request is issued.
- `PALENH.EXE`: separate ordinary Win16 GUI program; USER imports are fine
  here, but must never be added to the early display driver.

## References (historical, paraphrased)

- Microsoft Windows 3.1 DDK: display initialization, services/notifications,
  VDD/grabber compatibility:
  https://dos-help.soulsphere.org/ddag31qh.hlp/dispinit.html
  https://dos-help.soulsphere.org/ddag31qh.hlp/winint2fabout.html
  https://dos-help.soulsphere.org/ddag31qh.hlp/ovr_compat.html
- Microsoft KB Q80901, Q75008 (I/O trapping and virtualization):
  https://jeffpar.github.io/kbarchive/kb/080/Q80901/
  https://jeffpar.github.io/kbarchive/kb/075/Q75008/
- Microsoft KB Q120079 (zero ES on API discovery):
  https://www.betaarchive.com/wiki/index.php/Microsoft_KB_Archive/120079
- Ralf Brown interrupt list, 1684h:
  https://fd.lod.bz/rbil/interrup/windows/2f1684.html
- Microsoft 1993 256-color SVGA release notes:
  https://sierrahelp.com/Patches-Updates/Patches-Updates-Misc/Win31SVGAUpdate.html
- IBM example of the SVGA installation set:
  https://diarywind.com/blog/e/generic-svga-driver-for-ibmw31.html
- OS/2 Museum, Windows 3.x VDDVGA (source investigation):
  https://www.os2museum.com/wp/windows-3-x-vddvga/
- Ninji, independent SVGA driver/VDD investigation (secondary evidence):
  https://wuffs.org/blog/windows-3x-graphics
- Open Watcom C library reference, int86x/segread:
  https://open-watcom.github.io/open-watcom-v2-wikidocs/clib.html
