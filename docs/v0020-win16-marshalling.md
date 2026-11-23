# Palettium v0020: Win16 DDI pointer boundary

* A real Win16 GDI `BitBlt` signature now exists in `palddi20.c`, with operands passed as `LPVOID` far pointers. The module is **object-only** until the full Win16 display driver can be linked and initialized safely. It is **not installable**.
* `pal_ddl_blit.c` converts two WORDs (selector, offset) into an exact far-address token and routes calls to the *existing* `PAL_ScreenHost`. It does **not** dereference untrusted pointers. Thus it only accepts operands matching the bound screen `PDEVICE` far address; memory BITMAPs are not yet supported.
* Pattern-dependent ROP3 operations are rejected through the existing shared GDI bridge. Screen-to-screen pattern-independent ROP3 operations use the device's `ops.blit` override if installed.
* The host C tests check the exact bit layout of the address, invalid selectors/offsets, null and dormant hosts, a successful overlapping copy, unsupported pattern ROPs, and shutdown.
* The Win16 compilation probe does not imply runtime correctness. After build validation, our next focus is true two-stage `Enable` and the remaining required GDI services, based on the original Windows 3.1 DDK samples. The old `PALGDI.DRV` is still **not installable** and has incorrect ordinals.

Commands:

```
make -f Makefile.far20 test
sh windows/win31/build-palddi20.sh
```

All hardware backends remain OS-independent. This patch makes no ET4000-specific changes.
