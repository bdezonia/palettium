# Palettium v0021 — Win31 DDI staging cleanup

This update **consolidates**, rather than creates another competing driver.

* `windows/win31/palddi.c` replaces `palddi15.c`, `palddi20.c`, and
  `palgdi.c`. It implements the existing Win16 `BitBlt` forwarding function
  and deliberately fail-closed `Enable` and `Disable` stubs.
* `windows/win31/build-palddi.sh` compiles the unified Win16 **object** into
  `tests/exes/palddi.obj`. This is **NOT** a working display driver.
* `Makefile.win31ddi` replaces the numbered `Makefile.far20` for the
  existing host-side marshalling regression.
* `tools/cleanup-win31-staging.sh` removes the superseded source, build
  scripts and obsolete generated binaries. It leaves historical docs and
  the other working smoke tests intact. Run it after unpacking.

## Apply

```sh
cd ~/dev/palettium
git status --short
unzip -o ~/Downloads/pal21.zip
sh tools/cleanup-win31-staging.sh
make -f Makefile.win31ddi test
make test
sh windows/win31/build-palddi.sh
file tests/exes/palddi.obj
git status --short
git diff --stat
git add -A
git commit -m 'Palettium v0021: consolidate Win31 DDI staging code'
git tag v0021
```

Do not install or rename `palddi.obj` as a `.DRV` or modify the VM's
`SYSTEM.INI`. `Enable()` still returns 0 by design. Export ordinals must
be explicitly assigned in the eventual production NE module: `BitBlt @1`,
`Disable @4`, `Enable @5`; source-level function names alone don't do that.

Keep generated test programs and objects out of Git using:

```
/tests/exes/
```

Historical milestone documentation remains for reference even though its
old build commands have been superseded.
