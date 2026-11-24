#!/usr/bin/env python3
"""Extract local Windows display resources for a disposable-VM trial only.
No Microsoft resource bytes are distributed with Palettium.
"""
import argparse
import collections
import pathlib
import struct


def resources(path):
    data = pathlib.Path(path).read_bytes()
    def word(pos):
        return struct.unpack_from('<H', data, pos)[0]
    ne = struct.unpack_from('<I', data, 60)[0]
    if data[ne:ne+2] != b'NE':
        raise ValueError('not an NE executable')
    base = ne + word(ne+36)
    end = ne + word(ne+38)
    if base == end:
        return {}
    shift = word(base)
    def ident(value):
        if value & 0x8000:
            return value & 0x7fff
        pos = base + value
        size = data[pos]
        if pos + 1 + size > end:
            raise ValueError('invalid resource name')
        return data[pos+1:pos+1+size]
    result = {}
    pos = base+2
    while word(pos):
        typ = ident(word(pos))
        count = word(pos+2)
        pos += 8
        for _ in range(count):
            if pos+12 > end:
                raise ValueError('truncated resource table')
            offset, size, flags, name, _, _ = struct.unpack_from('<6H', data, pos)
            pos += 12
            start, length = offset << shift, size << shift
            if start+length > len(data):
                raise ValueError('truncated resource payload')
            key = (typ, ident(name))
            if key in result:
                raise ValueError('duplicate resource')
            result[key] = flags & 0x1070, data[start:start+length]
    return result


def encode_id(value):
    return (b'\xff'+struct.pack('<H',value) if isinstance(value,int)
            else value+b'\0')


def check_fonts(items):
    key = (b'OEMBIN', b'FONTS')
    if key not in items:
        raise ValueError('missing OEMBIN/FONTS resource')
    print('PASS: OEMBIN/FONTS resource present (%d bytes)' % len(items[key][1]))


def main():
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument('donor', help='local VGA.DRV')
    p.add_argument('--extract', type=pathlib.Path, help='write Microsoft-format Win16 RES')
    p.add_argument('--verify', help='resource-bound trial driver')
    a = p.parse_args()
    items = resources(a.donor)
    check_fonts(items)
    print('Donor resource count:', len(items))
    print('Types:', dict(collections.Counter(k[0] for k in items)))
    if a.extract:
        with a.extract.open('wb') as out:
            for (typ,name),(flags,payload) in items.items():
                out.write(encode_id(typ)+encode_id(name)+struct.pack('<HI',flags,len(payload))+payload)
        print('Wrote',a.extract)
    if a.verify:
        target = resources(a.verify)
        check_fonts(target)
        if items.keys() != target.keys():
            raise ValueError('resource identities differ')
        for key, (flags, payload) in items.items():
            newflags, newpayload = target[key]
            if flags != newflags or not newpayload.startswith(payload) or any(newpayload[len(payload):]):
                raise ValueError('resource differs: %r' % (key,))
        print('PASS: all %d resource identities, memory flags and payloads match donor (zero padding allowed)' % len(items))

if __name__ == '__main__':
    main()
