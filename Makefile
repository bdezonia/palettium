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
clean:
	rm -f tests/exes/raster-test tests/exes/banked-test tests/exes/blit-test tests/exes/win16-guard tests/exes/hardware-test tests/exes/format-test tests/exes/hardware-format-test tests/exes/raster-demo.ppm

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
