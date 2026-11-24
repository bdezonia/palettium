#!/usr/bin/env python3
"""Host-side tests for the reproducible NE export validator."""
import importlib.util
from pathlib import Path

path = Path(__file__).resolve().parents[1] / "tools" / "check_win31_exports.py"
spec = importlib.util.spec_from_file_location("checker", path)
module = importlib.util.module_from_spec(spec)
spec.loader.exec_module(module)

sample = """LIBRARY PALSTAGE
EXPORTS
 BITBLT @1
 COLORINFO @2
 DISABLE @4
 ENABLE @5
 OUTPUT @8
 PIXEL @9
 REALIZEOBJECT @10
 STRBLT @11
 SCANLR @12
 EXTTEXTOUT @14
 SETPALETTE @22
 GETPALETTE @23
 SETPALTRANS @24
 GETPALTRANS @25
 UPDATECOLORS @26
"""
assert module.parse_exports(sample) == module.EXPECTED
assert module.parse_exports("ENABLE @1\nDISABLE @2\n") != module.EXPECTED
assert module.parse_exports("ENABLE @5\nDISABLE @4\nBITBLT @9\n") != module.EXPECTED
assert module.parse_exports("garbage") == {}
for bad in ("PIXEL @9\nPIXEL @9\n", "PIXEL @9\nOUTPUT @9\n"):
    try:
        module.parse_exports(bad)
        raise AssertionError("duplicate was accepted")
    except ValueError:
        pass
print("PASS: Win31 export checker accepts valid ordinals and rejects invalid ones")
