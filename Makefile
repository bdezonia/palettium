CC = cc
CFLAGS = -std=c89 -O2 -Wall -Wextra -Werror -Iinclude -Ihardware/sim_bank

.PHONY: all test clean
all: raster-test banked-test blit-test win16-guard hardware-test
raster-test: common/rgfx.c tests/test_raster.c include/rgfx.h
	$(CC) $(CFLAGS) common/rgfx.c tests/test_raster.c -o $@
banked-test: common/rgfx.c hardware/sim_bank/sim_bank.c tests/test_banked.c include/rgfx.h hardware/sim_bank/sim_bank.h
	$(CC) $(CFLAGS) common/rgfx.c hardware/sim_bank/sim_bank.c tests/test_banked.c -o $@
blit-test: common/rgfx.c hardware/sim_bank/sim_bank.c tests/test_blit.c include/rgfx.h hardware/sim_bank/sim_bank.h
	$(CC) $(CFLAGS) common/rgfx.c hardware/sim_bank/sim_bank.c tests/test_blit.c -o $@
test: all
	./raster-test
	./banked-test
	./blit-test
	./win16-guard
	./hardware-test
clean:
	rm -f raster-test banked-test blit-test win16-guard hardware-test raster-demo.ppm

win16-guard: common/rgfx.c tests/test_win16_geometry.c include/rgfx.h
	$(CC) $(CFLAGS) common/rgfx.c tests/test_win16_geometry.c -o $@

hardware-test: common/rgfx.c common/rg_hardware.c hardware/sim_bank/sim_bank.c tests/test_hardware.c include/rgfx.h include/rg_hardware.h hardware/sim_bank/sim_bank.h
	$(CC) $(CFLAGS) common/rgfx.c common/rg_hardware.c hardware/sim_bank/sim_bank.c tests/test_hardware.c -o $@
