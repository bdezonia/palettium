# Palettium ET4000 BIOS ROM audit — 2026-11-23

Input: user-supplied `et4kroms.zip` (86Box `roms/video/et4000/`). This report and the accompanying stdlib-only inspection script do **not** modify or redistribute ROM contents. Findings below are from static file inspection and 8086 disassembly, not executions of each BIOS.

## ROM inventory

All six 32,768-byte video BIOS images have a `55 AA 40` option-ROM header, claim a 32KB image, and have a full-ROM checksum of zero (modulo 256). The other two files (`tg_ksc5601.rom`, `kasan_ksc5601.rom`) are **524,288-byte font ROMs**, not BIOS programs.

| Image | Embedded version/date | Relevant information |
|---|---|---|
| `ET4000.BIN` | V8.01X, 04/07/93 | Common generic ET4000 BIOS, different mode checks from earlier BIOS series |
| `ET4000_V8_06.BIN` | V8.06X, 04/15/92 | Generic 8.06X ROM; note version numbering does not match the embedded dates' order |
| `Tseng_Labs_VGA-4000_BIOS_V1.1.bin` | V1.10X, 09/26/90 | Earlier VGA-4000 BIOS |
| `Tseng_Labs_VGA-4000_BIOS_V1.21.bin` | V1.21X, 02/12/91 | VGA-4000 BIOS; its VRAM-based mode gate differs from V8 series |
| `tgkorvga.bin` | V1.31X, 05/29/92 | TriGem/Korean BIOS |
| `et4000_kasan16.bin` | V8.02X, 06/13/90; Kasan 1.0a 05/01/91 | Kasan Korean BIOS |

## Finding 1 — CRTC register 37h used in VRAM-capacity logic

Generic `ET4000.BIN` around ROM file offset **0x0C43** uses:

```asm
call 0x0FE8       ; select color/mono CRTC port, DX=3D4h or 3B4h
mov  ah,37h       ; indexed CRTC register 37h
call 0x0D21       ; write index to DX, read data from DX+1
 test al,08h
 je ...
 and  al,03h
```

CRTC register 37h is **Video System Configuration 2** in the Tseng 1990 data book. The data book describes bits 0–1 as display-memory bus width and bits 2–3 as display-memory depth. Taken with the BIOS's own configured state, these encode its expected memory arrangement. This is **not** an unconditional direct VRAM byte-size register and could be BIOS/board-dependent. The BIOS capacity routine calls it at POST and uses other checks as well. A read-only query after the BIOS has configured hardware is a promising capacity *candidate*, not proof of installed memory size on all cards.

See Tseng ET4000 Data Book, CRTC indexed register 37h description.

## Finding 2 — POST uses write-based memory tests

All six 32KB BIOSes contain both a byte pattern writer (`mov al,55h; mov [bx],al`...) and a block pattern writer. For example, in `ET4000.BIN` at **0x0C2E**:

```asm
mov  ax,55AAh
sub  di,di
mov  cx,0800h
push cx
push di
rep  stosw        ; 2048 words = 4096 bytes written to ES:DI
pop  di
pop  cx
repe scasw       ; compare the test pattern
```

An earlier routine around **0x0C00** writes 55h/AAh and restores a saved byte. These are active probing techniques appropriate to firmware initialization, **not** safe routines to transplant into Palettium after Windows is running. At least the block-pattern routine overwrites tested memory.

## Finding 3 — V8 versus V1 BIOS mode admission differs

The generic V8.01X ROM at **0x0CB9** and V8.06X at **0x0C78** explicitly allow internal requests with `BL=2Dh` or `BL=2Fh` before their higher-memory gate. Other extended requests consult the register-37h capacity routine. The V1.21X ROM at **0x0BB2** and V1.10X near **0x0C40** have a different threshold; the V1.21X routine, for example, has no such `2Fh` exemption and instead imposes a >=2 memory-class check for extended modes >=2Ah (apart from other special cases).

These are paths from the actual INT10 mode-set dispatch (`V8.01X` calls its gate at ROM offset `0x1B46`, `V1.21X` at `0x1AC0`). This is strong evidence that **BIOS version influences mode eligibility**, even for the same ET4000 chipset. It is not by itself proof of the final resulting display geometry for each mode, nor of how all possible board configurations behave.

Practical implication: continue treating **BIOS mode-set confirmation** as authoritative and maintain a separate capability check; do not treat the generic BIOS's admission table as universal.

## Finding 4 — BIOS is separate from the Windows VDD

No uploaded file is a Windows `.386` virtual display driver. The ROMs cannot resolve the user's confirmed `SYSTEM.INI [386Enh] display=*vddvga` compatibility issue. That requires separately studying Windows 3.1 VDD and GDI behavior.

86Box's video implementation selects an option ROM based on the video device/BIOS configuration, and sets emulated VRAM capacity from the separate `memory` setting. Its current source names these exact filenames. Therefore we should identify **which BIOS was selected** in the user's VM before treating one specific code path as tested.

## Recommendation for Palettium

1. Keep v0029's conservative mode table and verified capacity requirement; do not use BIOS POST memory-write probes at Windows runtime.
2. Add an **optional, read-only ET4000 capacity-register inspection to the DOS test**, preserving the indexed CRTC register selection and reporting raw `37h` and decoded candidate memory class next to the user-selected capacity. Verify against 256, 512, and 1024KB emulated cards and multiple BIOS versions; never automatically override verified capacity based only on this heuristic.
3. Identify the loaded 86Box BIOS by configuration or C000h ROM identification. Compare `2Eh/2Fh` acceptance across at least one generic V8 and one VGA-4000 V1 ROM using disposable DOS VM snapshots.
4. Keep Windows 3.1 native driver integration gated until VDD handling and `Enable(style=0)` are correct. Do not assume DOS success makes protected-mode direct I/O or mode-switching safe.

## Reproducibility

`python3 inspect_et4k_roms.py et4kroms.zip` lists the 8 files, ROM checksums, version strings, and exact offsets of selected recurring patterns. Its signature offsets are evidence markers, not a full control-flow analysis. `audit_output.txt` contains the results for this upload. `objdump -D -b binary -m i8086 -M intel` was used to examine individual routines.
