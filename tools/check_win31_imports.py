#!/usr/bin/env python3
"""Read Win16 NE MODULE REFERENCE table (not exports, not DOS strings).

A Windows 3.1 system display driver is imported while USER is starting.
A static import of USER/MessageBox forces an early, failing USER.DLL load.
Reject any pre-USER dependency rather than trusting a successful link.
"""
import pathlib
import struct
import sys


def imports(path):
    data = pathlib.Path(path).read_bytes()

    def u16(pos):
        if pos < 0 or pos + 2 > len(data):
            raise ValueError("truncated Win16 NE structure")
        return struct.unpack_from("<H", data, pos)[0]

    def u32(pos):
        if pos < 0 or pos + 4 > len(data):
            raise ValueError("truncated Win16 NE structure")
        return struct.unpack_from("<I", data, pos)[0]

    if len(data) < 0x40 or data[:2] != b"MZ":
        raise ValueError("missing MZ header")
    ne = u32(0x3c)
    if ne + 0x3a > len(data) or data[ne:ne + 2] != b"NE":
        raise ValueError("not a Win16 NE file")
    refs = u16(ne + 0x1e)
    modtab = ne + u16(ne + 0x28)
    imptab = ne + u16(ne + 0x2a)
    if refs > 256 or modtab + refs * 2 > len(data):
        raise ValueError("invalid NE module-reference table")
    names = []
    for i in range(refs):
        off = imptab + u16(modtab + i * 2)
        if off < imptab or off >= len(data):
            raise ValueError("bad NE imported-name offset")
        n = data[off]
        if n == 0 or off + 1 + n > len(data):
            raise ValueError("bad NE imported-name length")
        try:
            names.append(data[off + 1:off + 1 + n].decode("ascii").upper())
        except UnicodeDecodeError as exc:
            raise ValueError("non-ASCII Win16 import name") from exc
    return names


def check(path):
    names = imports(path)
    print("NE imported modules:", ", ".join(names) or "(none)")
    bad = [name for name in names if name not in ("KERNEL", "KERNEL.EXE")]
    if "USER" in names or "USER.DLL" in names:
        if b"Stack Overflow!" in pathlib.Path(path).read_bytes():
            print("DIAGNOSIS: Watcom's default Win16 __STK overflow handler "
                  "appears to be linked (Stack Overflow! string); "
                  "it can bring in USER.MessageBox.", file=sys.stderr)
    if bad:
        print("FAIL: early Windows 3.1 display driver imports a module that "
              "may not yet be initialized:", ", ".join(bad), file=sys.stderr)
        return 1
    print("PASS: no USER/GDI or other pre-startup module dependencies")
    return 0


if __name__ == "__main__":
    if len(sys.argv) != 2:
        print("usage: check_win31_imports.py PALTRIAL.DRV", file=sys.stderr)
        raise SystemExit(2)
    try:
        raise SystemExit(check(sys.argv[1]))
    except (OSError, ValueError) as exc:
        print("FAIL:", exc, file=sys.stderr)
        raise SystemExit(2)
