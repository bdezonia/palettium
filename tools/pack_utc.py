#!/usr/bin/env python3
"""Pack specified repo-relative files with real UTC ZIP extended timestamps.

Usage: python3 tools/pack_utc.py pal16.zip PATCH-v0016.txt ...
Run in the repository root; never pass compiled binaries or ROMs.
"""
import datetime
import pathlib
import struct
import sys
import time
import zipfile
from zoneinfo import ZoneInfo


def pack(output, filenames):
    now = int(time.time())
    utc = datetime.datetime.fromtimestamp(now, datetime.timezone.utc)
    # DOS ZIP header has NO timezone. Store Chicago wall-clock time so
    # legacy unzip (used by the project maintainer) displays local file time.
    # Extended 0x5455 records the exact UTC mtime for timezone-aware tools.
    # Do not encode UTC clock time in the DOS header: that creates a six-hour
    # skew on extraction during CST (five hours during CDT).
    local = datetime.datetime.fromtimestamp(now, ZoneInfo("America/Chicago"))
    stamp = (local.year, local.month, local.day, local.hour, local.minute,
             local.second - (local.second % 2))
    with zipfile.ZipFile(output, 'w', compression=zipfile.ZIP_DEFLATED) as zf:
        for filename in filenames:
            path = pathlib.Path(filename)
            if path.is_absolute() or '..' in path.parts or not path.is_file():
                raise ValueError('invalid relative source file: ' + filename)
            zi = zipfile.ZipInfo(path.as_posix(), stamp)
            zi.create_system = 3
            zi.external_attr = (0o100644 << 16)
            zi.extra = struct.pack('<HHBI', 0x5455, 5, 1, now)
            zf.writestr(zi, path.read_bytes(), compress_type=zipfile.ZIP_DEFLATED)
        assert zf.testzip() is None
    print('Wrote', output, 'UTC mtime', utc.isoformat(),
          'DOS local clock', local.isoformat())

if __name__ == '__main__':
    if len(sys.argv) < 3:
        sys.exit('Usage: pack_utc.py OUTPUT.zip FILE [FILE...]')
    pack(sys.argv[1], sys.argv[2:])
