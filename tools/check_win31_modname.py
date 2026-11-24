#!/usr/bin/env python3
"""Check the resident NE module name, not the external DRV filename."""
import pathlib
import struct
import sys


def module_name(data):
    if data[:2] != b'MZ':
        raise ValueError('not MZ')
    ne = struct.unpack_from('<I', data, 0x3c)[0]
    if data[ne:ne+2] != b'NE':
        raise ValueError('not NE')
    offset = struct.unpack_from('<H', data, ne+0x26)[0]
    if offset < 0x40:
        raise ValueError('invalid resident name offset')
    pos = ne + offset
    length = data[pos]
    if not length or pos+1+length+2 > len(data):
        raise ValueError('invalid resident name')
    if struct.unpack_from('<H', data, pos+1+length)[0] != 0:
        raise ValueError('first resident name is not module ordinal zero')
    return data[pos+1:pos+1+length].decode('ascii')


def main():
    try:
        name = module_name(pathlib.Path(sys.argv[1]).read_bytes())
        if name != 'DISPLAY':
            raise ValueError('expected DISPLAY, found ' + name)
    except (OSError, ValueError, IndexError, struct.error) as exc:
        print('FAIL: NE display module name:', exc, file=sys.stderr)
        return 1
    print('PASS: NE resident module name DISPLAY')
    return 0


if __name__ == '__main__':
    sys.exit(main())
