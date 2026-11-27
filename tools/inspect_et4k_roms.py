#!/usr/bin/env python3
"""Read-only identification and signature audit of 86Box Tseng ET4000 ROM ZIP.
No ROM execution, mutation, extraction, or hardware access. Python 3 standard library.
"""
import hashlib
import re
import sys
import zipfile

PROBE_4K = bytes.fromhex('b8 aa 55 2b ff b9 00 08')
PROBE_BYTE = bytes.fromhex('b0 55 88 07')
MODE_TABLE_FRAGMENT = bytes.fromhex('2d 2e 2f 30 38')

def findall(data, seq):
    return [m.start() for m in re.finditer(re.escape(seq), data)]

def loclist(data, seq):
    return ','.join('0x%04X' % n for n in findall(data, seq)) or '-'

def inspect(name, data):
    valid = len(data) >= 3 and data[:2] == b'\x55\xaa' and data[2] * 512 == len(data) and sum(data) % 256 == 0
    ident = re.search(rb'(?:V\d+\.\d+X)', data[:0x210])
    date = re.search(rb'\d\d/\d\d/\d\d', data[:0x210])
    return {
        'name': name, 'size': len(data), 'rom': bool(valid),
        'sha256': hashlib.sha256(data).hexdigest(),
        'version': ident.group().decode() if ident else '-',
        'date': date.group().decode() if date else '-',
        'vram_text': loclist(data, b'256KB'),
        'probe4k': loclist(data, PROBE_4K),
        'probe_byte': loclist(data, PROBE_BYTE),
        'mode_table': loclist(data, MODE_TABLE_FRAGMENT),
    }

def main(path):
    with zipfile.ZipFile(path) as z:
        for info in sorted(z.infolist(), key=lambda i: i.filename.lower()):
            if info.is_dir():
                continue
            d = z.read(info)
            r = inspect(info.filename, d)
            print('%s: %d bytes; %s; version=%s date=%s' % (
                r['name'], r['size'], 'valid PC option ROM checksum' if r['rom'] else 'not a 32KB PC BIOS', r['version'], r['date']))
            print('  sha256=%s' % r['sha256'])
            if r['rom']:
                print('  VRAM display text=%s; 4K test-pattern writer=%s; byte test writer=%s; extended mode-list fragment=%s' % (
                    r['vram_text'], r['probe4k'], r['probe_byte'], r['mode_table']))

if __name__ == '__main__':
    if len(sys.argv) != 2:
        sys.exit('Usage: python3 inspect_et4k_roms.py et4kroms.zip')
    main(sys.argv[1])
