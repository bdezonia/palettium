#!/usr/bin/env python3
"""Conservative PALXHOOK audit: every NE segment fixed, preloaded,
not discardable; data writable. This is not a runtime residency proof."""
import pathlib
import struct
import sys


def check(data):
    if data[:2] != b'MZ':
        raise ValueError('not MZ')
    ne = struct.unpack_from('<I', data, 0x3c)[0]
    if data[ne:ne+2] != b'NE':
        raise ValueError('not NE')
    count = struct.unpack_from('<H', data, ne+0x1c)[0]
    rel = struct.unpack_from('<H', data, ne+0x22)[0]
    start = ne + rel
    if not count or rel < 0x40 or start+8*count > len(data):
        raise ValueError('invalid segment table')
    for i in range(count):
        flags = struct.unpack_from('<H', data, start+8*i+4)[0]
        if flags & 0x0010 or not flags & 0x0040 or flags & 0xf000:
            raise ValueError('segment %d not fixed/preload/non-discardable: %04x' % (i+1, flags))
        if flags & 1 and flags & 0x0080:
            raise ValueError('segment %d data read-only' % (i+1))
    return count


if __name__ == '__main__':
    try:
        print('PASS: %d NE segments fixed, preload, non-discardable' %
              check(pathlib.Path(sys.argv[1]).read_bytes()))
    except (OSError, ValueError, IndexError, struct.error) as exc:
        sys.exit('FAIL: PALXHOOK residency audit: ' + str(exc))
