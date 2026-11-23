# Palettium v0031 handoff — Win16 ET4000AX build repair and complete clean (2026-11-23 UTC)

## Maintainer-confirmed results through v0029

The maintainer has passed all Linux host tests and Open Watcom builds
through **v0029**. The 13-export Windows 3.1 `PALSTAGE.DRV` was linked and
verified with `wdump`. The DOS `ET4KTEST.EXE` was **actually run** in 86Box
with **256, 512 and 1024 KiB** ET4000AX configurations and appeared correct
in all three. This is encouraging direct hardware-model evidence, but not
Windows 3.1 display-driver initialization.

## v0030 blocker reported by maintainer

Applying `pal30.zip` and running the host Makefiles gave all PASS results.
The Open Watcom Win16 compile stopped at `pal_et4k_session.c`, because
`windows/win31/pal_et4k_session.h` includes `"et4k.h"`, but
`windows/win31/build-palddi.sh` lacked
`-i=hardware/tseng/et4000ax`. Subsequent unknown `ET4K_Port` and
`ET4K_State` errors were cascading header errors. A second audit found
that the Win16 source list did not include
`hardware/tseng/et4000ax/et4k.c`, so linking would have failed once
compilation proceeded. Consequently v0030 NE compilation/linking **did not
pass**. The subsequent `wdump` reported no PALSTAGE.DRV because compilation
stopped and the build script had removed its prior output.

## v0031 corrections

1. **No production filenames changed.** `windows/win31/build-palddi.sh`
   now passes the ET4000AX include directory to `wcc` and compiles the
   **existing** `hardware/tseng/et4000ax/et4k.c` alongside the Win16 port
   adapter and session coordinator. This closes the confirmed missing
   header and expected ET4K function-definition linker gap; it does not
   create a duplicate backend or alter the 13 DDI export ordinals.
2. Project-root `make clean` now removes **all generated** `tests/exes/`
   host executables, Win16 object files/NE staging driver/export report,
   `build/` (including DOS `ET4KTEST.EXE`), root-level toolchain object
   artifacts, and old Win16 probe objects/DLLs/executables in
   `windows/win31/`. It preserves checked-in source, headers, notes and
   linker `.LNK` files. It is safe to invoke repeatedly. `/build/` is
   ignored in Git.
3. `tests/test_build_maintenance.py` is now run by `make test`. It checks
   Win16 source/include closure and unique object filenames; it executes
   the project-wide clean **in a temporary test directory**, verifying
   generated-file deletion and source preservation. No cleanup test
   deletes files from the working repository.
4. README, Win31 README and patch notes updated. **No driver behavior,
   BIOS/VDD policy, PDEVICE ABI, or hardware-mode enabling changed.**

## Build and test

Apply `pal31.zip` over the **v0030** tree:

```sh
cd ~/dev/palettium
unzip -o ~/Downloads/pal31.zip
make clean
make test
make -f Makefile.win31native test
make -f Makefile.et4kmodes test
make -f Makefile.et4k test
make -f Makefile.win31link test
make -f Makefile.screen17 test
make -f Makefile.win31ddi test
make -f Makefile.bitmap test
sh windows/win31/build-palddi.sh
wdump -x tests/exes/PALSTAGE.DRV
```

`make clean` intentionally deletes the v0029 DOS executable and old test
binaries. Rebuild the DOS smoke test with `sh dos/build-et4k.sh` *only if*
needed; the maintainer already validated all three VRAM sizes with v0029.
All platform-neutral host regressions and the clean test pass in the
assistant's Linux environment. **Open Watcom v0031 Win16 compilation and
linking still need the maintainer's test.** If there is another Watcom
compiler or linker error, capture the first one, not the cascading errors.

## Remaining hardware/driver limitations

The v0030 Win16 native bridge and session logic remain **unactivated**.
`Enable(style=0)` still fails, and PALSTAGE.DRV is deliberately **NOT
INSTALLABLE**. `Enable(style=1)` remains a staging 640×480 candidate.
No Win16 VRAM-capacity discovery/configuration or operational GDI-owned
PDEVICE binding exists. The BIOS mode-set and A000 selector adapter are
not yet runtime-verified under Windows 3.1; 386-enhanced-mode VDD
coordination, text/cursor, huge BITMAP support and a complete DDI capability
audit also remain. Do not put PALSTAGE.DRV into SYSTEM.INI.

After the Win16 v0031 build is verified, the next appropriate step is a
**non-mode-changing**, recoverable Win3.1 runtime diagnostic of KERNEL's
A000 selector and BIOS current-mode readback (not an installable driver),
then actual PDEVICE ownership and VDD coordination. Keep the shared C89
renderer and reusable ET4000AX backend for Windows 3.1, WfW 3.11,
Windows 95 and Windows 98. Fixed-width checkpoint tags; source-only
incremental ZIP with correct UTC extended and local DOS ZIP timestamps.
