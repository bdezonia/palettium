# Open Watcom 16-bit Windows DLL smoke test; run from this directory.
# Requires WATCOM, PATH and INCLUDE environment configured by Open Watcom.
# Assumes wcc and wlink are on PATH.
WCC = wcc
WLINK = wlink
all: PALPROBE.DLL
PALPROBE.OBJ: palprobe.c ../../include/rgfx.h
	$(WCC) -bt=windows -mc -zu -zq -i=../../include -fo=PALPROBE.OBJ palprobe.c
PALPROBE.DLL: PALPROBE.OBJ PALPROBE.LNK
	$(WLINK) @PALPROBE.LNK
clean:
	rm -f PALPROBE.OBJ PALPROBE.DLL PALPROBE.MAP
