# v0077 — repository layout synchronization

Baseline: maintainer's reorganized `palsrc.zip` after successful v0076
PALX386 desktop startup (About dialog reported 386 Enhanced mode).
This release is build-system maintenance only, not a driver change.
The supplied archive passed ZIP verification. All 30 entries in the
v0074, v0075 and v0076 patch manifests matched the snapshot after
translating the moved makefile and patch-note paths.

## Layout contract

Run build and verification commands from the repository root. Makefiles
are in `makefiles/`, patch notes in `patch-notes/` and checksums in
`checksums/`. No root Makefile is introduced. Source directories and
`tests/exes/`/`build/` output locations are unchanged. Native build
scripts keep their existing invocation and location.

```
cd ~/dev/palettium
unzip -o ~/Downloads/pal77.zip
sha256sum -c checksums/CHECKSUMS-v0077.sha256
make -f makefiles/Makefile clean
make -f makefiles/Makefile test-all
```

The main `test` target retains its original coverage. `test-all` runs
it first, then runs the remaining standalone `makefiles/Makefile.*`
regression targets in sequence. All standalone makefiles in this
folder should expose a `test` target. The win31link target additionally
runs the text test as before. To run only one standalone suite:

```
make -f makefiles/Makefile.win31native test
make -f makefiles/Makefile.win31link test
```

Do not use `make -C makefiles`: compiler paths are root-relative.
The clean regression now constructs the relocated layout, checks
idempotent cleanup and confirms that notes, checksums and makefiles
survive. The obsolete staging cleanup script recognizes both old and
new paths for the retired Makefile.far20. Existing historical checksums
and release notes remain unchanged; these are release-specific records,
not manifests of the current tree. New patch manifests use paths relative
to the repository root. Historical documents may mention retired targets
or older paths; consult the current root README first.

## Validation

- Clean host build: PASS (C89, warnings as errors).
- Main suite and all 22 standalone regression makefiles: PASS.
- 38 generated files under tests/exes after the full run.
- Win16 host syntax checks, import/export checker fixtures and global
  cleanup test: PASS. Expected negative import-check fixtures print FAIL
  before the enclosing checker reports PASS.
- Shell syntax checks of all build/maintenance scripts: PASS.
- No changes to any C source, header, native linker recipe, or native
  driver build script/flags. No reference driver binaries are included.
- Open Watcom is unavailable in the assistant's host; native builds and
  actual VM tests have not been repeated for this maintenance release.

## Maintainer native build check

```
sh windows/win31/build-palddi.sh
sh windows/win31/build-paltrial.sh
sh windows/win31/build-palx386.sh
```

The build scripts still audit NE module name, KERNEL-only imports and
26 DDI exports. No new VM activation or DOS switching experiment is
requested for v0077. Existing runtime-validated v0076 binaries may be
retained. See v0076 startup notes for resource binding and activation
restrictions; translate old make and checksum paths as above.

## Next engineering milestone

Resume Enhanced-mode development once the reorganized baseline is
confirmed. Desktop startup is now demonstrated, but screen switching,
VDD notification chaining, deferred repaint, bank/palette/cursor ownership
and restoration are unvalidated. Keep the experimental Enhanced gate and
Standard-mode driver separate. Do not enable assisted save/restore before
a correct resident notification bridge and ownership protocol exist.
Preserve shared C89 raster/operation overrides, memory bitmap support,
independent resources/installation work, the four OS targets and future
hardware acceleration. Splash corruption is currently not reproducible;
monitor rather than changing unrelated startup code.
