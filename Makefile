CC = cc
CFLAGS = -std=c89 -O2 -Wall -Wextra -Werror -Iinclude

.PHONY: all test clean
all: raster-test
raster-test: common/rgfx.c tests/test_raster.c include/rgfx.h
	$(CC) $(CFLAGS) common/rgfx.c tests/test_raster.c -o $@
test: raster-test
	./raster-test
clean:
	rm -f raster-test raster-demo.ppm
