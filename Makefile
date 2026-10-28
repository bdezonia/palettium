CC = cc
CFLAGS = -std=c89 -O2 -Wall -Wextra -Werror -Iinclude -Ihardware/sim_bank

.PHONY: all test clean
all: raster-test banked-test blit-test win16-guard
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
clean:
	rm -f raster-test banked-test blit-test win16-guard raster-demo.ppm

win16-guard: common/rgfx.c tests/test_win16_geometry.c include/rgfx.h
	$(CC) $(CFLAGS) common/rgfx.c tests/test_win16_geometry.c -o $@
