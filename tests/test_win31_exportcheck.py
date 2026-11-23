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
 DISABLE @4
 ENABLE @5
"""
assert module.parse_exports(sample) == {"BITBLT": 1, "DISABLE": 4, "ENABLE": 5}
assert module.parse_exports("ENABLE @1\nDISABLE @2\n") != module.EXPECTED
assert module.parse_exports("ENABLE @5\nDISABLE @4\nBITBLT @9\n") != module.EXPECTED
assert module.parse_exports("garbage") == {}
print("PASS: Win31 export checker accepts valid ordinals and rejects invalid ones")
