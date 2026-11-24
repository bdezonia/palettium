#!/usr/bin/env python3
"""Test the real NE parser against synthetically controlled module tables."""
import importlib.util
import pathlib
import struct
import tempfile

ROOT = pathlib.Path(__file__).resolve().parents[1]
spec = importlib.util.spec_from_file_location(
    "check_win31_imports", ROOT / "tools/check_win31_imports.py")
module = importlib.util.module_from_spec(spec)
spec.loader.exec_module(module)


def fake_ne(names):
    data = bytearray(512)
    data[:2] = b"MZ"
    struct.pack_into("<I", data, 0x3c, 0x80)
    data[0x80:0x82] = b"NE"
    struct.pack_into("<H", data, 0x80 + 0x1e, len(names))
    struct.pack_into("<H", data, 0x80 + 0x28, 0x50)
    struct.pack_into("<H", data, 0x80 + 0x2a, 0x70)
    cursor = 1
    for i, name in enumerate(names):
        raw = name.encode("ascii")
        struct.pack_into("<H", data, 0x80 + 0x50 + i * 2, cursor)
        data[0x80 + 0x70 + cursor] = len(raw)
        data[0x80 + 0x70 + cursor + 1:0x80 + 0x70 + cursor + 1 + len(raw)] = raw
        cursor += 1 + len(raw)
    return data


def main():
    with tempfile.TemporaryDirectory() as td:
        p = pathlib.Path(td) / "test.drv"
        for names, expect in ((["KERNEL"], 0), ([], 0),
                              (["KERNEL", "USER"], 1),
                              (["KERNEL", "USER.DLL"], 1),
                              (["KERNEL", "GDI"], 1)):
            p.write_bytes(fake_ne(names))
            assert module.imports(p) == names
            assert module.check(p) == expect, names
        p.write_bytes(b"MZ" + b"\0" * 128)
        try:
            module.imports(p)
            assert False, "invalid NE accepted"
        except ValueError:
            pass
    # The v0045 executable contained both the Stack Overflow! handler and
    # a USER import. The diagnostic is advisory; only the NE table controls
    # whether the display module passes the mandatory import audit.
    with tempfile.TemporaryDirectory() as td:
        p = pathlib.Path(td) / "stackdriver.drv"
        p.write_bytes(fake_ne(["KERNEL", "USER"]) + b"Stack Overflow!")
        assert module.check(p) == 1
    # The maintainer-supplied v0045 binary is not needed for test portability.
    print("PASS: Win16 NE import audit, no-USER bootstrap policy, corrupted files")


if __name__ == "__main__":
    main()
