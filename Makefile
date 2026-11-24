CC = cc
CFLAGS = -std=c89 -O2 -Wall -Wextra -Werror -Iinclude -Ihardware/sim_bank

.PHONY: all test clean
all: tests/exes/raster-test tests/exes/banked-test tests/exes/blit-test tests/exes/win16-guard tests/exes/hardware-test tests/exes/format-test tests/exes/hardware-format-test
tests/exes/raster-test: common/rgfx.c tests/test_raster.c include/rgfx.h | tests/exes
	$(CC) $(CFLAGS) common/rgfx.c tests/test_raster.c -o $@
tests/exes/banked-test: common/rgfx.c hardware/sim_bank/sim_bank.c tests/test_banked.c include/rgfx.h hardware/sim_bank/sim_bank.h | tests/exes
	$(CC) $(CFLAGS) common/rgfx.c hardware/sim_bank/sim_bank.c tests/test_banked.c -o $@
tests/exes/blit-test: common/rgfx.c hardware/sim_bank/sim_bank.c tests/test_blit.c include/rgfx.h hardware/sim_bank/sim_bank.h | tests/exes
	$(CC) $(CFLAGS) common/rgfx.c hardware/sim_bank/sim_bank.c tests/test_blit.c -o $@
test: all
	./tests/exes/raster-test
	./tests/exes/banked-test
	./tests/exes/blit-test
	./tests/exes/win16-guard
	./tests/exes/hardware-test
	./tests/exes/format-test
	./tests/exes/hardware-format-test
	python3 tests/test_build_maintenance.py
	$(MAKE) -f Makefile.gdiinfo test
	$(MAKE) -f Makefile.win31diag test
	$(MAKE) -f Makefile.win31text test
	$(MAKE) -f Makefile.cursor test
# This is the project-wide clean, not merely the original seven raster tests.
# All Makefile.* host binaries and the Win16 staging objects live under
# tests/exes; DOS outputs live under build/. Older Win16 probe scripts run
# in windows/win31 and emit intermediate OBJ files alongside their sources.
# Never delete checked-in Win16 linker recipes (for example PALPROBE.LNK).
clean:
	rm -rf tests/exes build
	rm -f windows/win31/*.obj windows/win31/*.OBJ \
	      windows/win31/*.o windows/win31/*.O \
	      windows/win31/*.dll windows/win31/*.DLL \
	      windows/win31/*.exe windows/win31/*.EXE \
	      windows/win31/*.drv windows/win31/*.DRV \
	      windows/win31/*.map windows/win31/*.MAP \
	      windows/win31/*.err windows/win31/*.ERR \
	      windows/win31/*.EXP windows/win31/*.exp \
	      windows/win31/*.EXPORTS.txt \
	      ./*.obj ./*.OBJ ./*.o ./*.O ./*.map ./*.MAP

tests/exes/win16-guard: common/rgfx.c tests/test_win16_geometry.c include/rgfx.h | tests/exes
	$(CC) $(CFLAGS) common/rgfx.c tests/test_win16_geometry.c -o $@

tests/exes/hardware-test: common/rgfx.c common/rg_hardware.c hardware/sim_bank/sim_bank.c tests/test_hardware.c include/rgfx.h include/rg_hardware.h hardware/sim_bank/sim_bank.h | tests/exes
	$(CC) $(CFLAGS) common/rgfx.c common/rg_hardware.c hardware/sim_bank/sim_bank.c tests/test_hardware.c -o $@

tests/exes/format-test: common/rgfx.c hardware/sim_bank/sim_bank.c tests/test_formats_unified.c include/rgfx.h hardware/sim_bank/sim_bank.h | tests/exes
	$(CC) $(CFLAGS) common/rgfx.c hardware/sim_bank/sim_bank.c tests/test_formats_unified.c -o $@

tests/exes/hardware-format-test: common/rgfx.c common/rg_hardware.c tests/test_hardware_formats.c | tests/exes
	$(CC) $(CFLAGS) common/rgfx.c common/rg_hardware.c tests/test_hardware_formats.c -o $@

tests/exes:
	mkdir -p $@
