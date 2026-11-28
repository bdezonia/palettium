#!/usr/bin/env python3
"""Source-level 16-bit instruction model for the small resident observer.
Not an assembler, CPU emulator, Watcom ABI test, or proof of residency.
Models the actual ASM source, including DOS vector get/set and stack frame.
"""
import importlib.util
import pathlib
import re
import struct
import subprocess
import sys
import tempfile

ROOT = pathlib.Path(__file__).resolve().parents[1]
SRC = ROOT / 'windows/win31/pal_hook.asm'


def parse(native=False):
    code, labels, data = [], {}, {}
    in_code = False
    enabled = True
    for line in SRC.read_text().splitlines():
        line = line.split(';')[0].strip().lower()
        if line == 'ifdef pal_native_repaint':
            enabled = native
            continue
        if line == 'else':
            enabled = not native
            continue
        if line == 'endif':
            enabled = True
            continue
        if not enabled or not line:
            continue
        if line.startswith('palhook_code segment'):
            in_code = True
        elif line == 'palhook_code ends':
            in_code = False
        elif ' dw ' in line:
            data[line.split()[0]] = 0x100 + 2*len(data)
        elif line.endswith(':') or ' proc ' in line:
            labels[line.split()[0].rstrip(':')] = len(code)
        elif line.startswith(('push ', 'pop ', 'mov ', 'cmp ', 'jne ',
                              'je ', 'jz ', 'jmp ', 'inc ', 'call ', 'or ',
                              'xor ', 'shl ', 'int ', 'ja ')) or line in (
                                  'pushf', 'popf', 'pushfd', 'popfd', 'pushad', 'popad', 'cld', 'ret', 'retf', 'retf 2'):
            code.append(line)
        elif in_code and not (line.startswith(('assume ', 'public ', 'extrn ')) or line.endswith(' endp')):
            raise AssertionError('unmodeled instruction: ' + line)
    return code, labels, data


CODE, LABELS, DATA = parse()


class CPU:
    def __init__(self):
        self.r = dict(ax=0x1111, bx=0x2222, cx=0x3333, dx=0x4444,
                      si=0x5555, di=0x6666, bp=0x7777, ds=0x8888,
                      es=0x9999, fs=0x1110, gs=0x2220, ss=0xaaaa, cs=0x1000, sp=0xe000)
        self.flags = 0x0847
        self.mem = {(0x2000, off): 0 for off in DATA.values()}
        self.vector = (0x3000, 0x4444)
        self.set_mode = 'normal'
        self.sets = 0
        self.callbacks = 0

    def address(self, a):
        if a in DATA:
            assert self.r['ds'] == 0x2000
            return self.r['ds'], DATA[a]
        if a == 'seen_bg[bx]':
            assert self.r['ds'] == 0x2000
            return self.r['ds'], DATA['seen_bg'] + self.r['bx']
        m = re.fullmatch(r'\[bp([+-]\d+)\]', a)
        if m:
            return self.r['ss'], (self.r['bp'] + int(m[1])) & 0xffff
        raise AssertionError(a)

    def get(self, a):
        if a in self.r:
            return self.r[a]
        if a == 'dgroup':
            return 0x2000
        if a.startswith('offset '):
            return LABELS[a[7:]]
        if a in DATA or '[' in a:
            return self.mem[self.address(a)]
        return int(a[:-1], 16) if a.endswith('h') else int(a)

    def put(self, a, v):
        v &= 0xffff
        if a in self.r:
            self.r[a] = v
        else:
            self.mem[self.address(a)] = v

    def push(self, v):
        self.r['sp'] -= 2
        self.mem[self.r['ss'], self.r['sp']] = v

    def pop(self):
        v = self.mem[self.r['ss'], self.r['sp']]
        self.r['sp'] += 2
        return v

    def arithmetic_flags(self, val, carry=False):
        self.flags = (self.flags & ~0x41) | (0x40 if val == 0 else 0) | int(carry)

    def run(self, label):
        pc = LABELS[label]
        for _ in range(300):
            line = CODE[pc]
            pc += 1
            parts = line.split(None, 1)
            op = parts[0]
            a = parts[1] if len(parts) > 1 else ''
            if op == 'push': self.push(self.get(a))
            elif op == 'pop': self.put(a, self.pop())
            elif op == 'pushf': self.push(self.flags)
            elif op == 'popf': self.flags = self.pop()
            # Model the stack sizes of the 386 save block; registers in
            # the legacy model are low words. Upper halves are not modeled.
            elif op == 'pushfd': self.push(0); self.push(self.flags)
            elif op == 'popfd': self.flags=self.pop(); self.pop()
            elif op == 'pushad':
                sp=self.r['sp']
                for k in ('ax','cx','dx','bx','sp','bp','si','di'):
                    self.push(0); self.push(sp if k=='sp' else self.r[k])
            elif op == 'popad':
                for k in ('di','si','bp','sp','bx','dx','cx','ax'):
                    v=self.pop(); self.pop()
                    if k!='sp': self.r[k]=v
            elif op == 'cld': self.flags &= ~0x400
            elif op in ('mov', 'cmp', 'or', 'xor', 'shl'):
                dst, src = map(str.strip, a.split(','))
                b = self.get(src)
                if op == 'mov': self.put(dst, b)
                else:
                    x = self.get(dst)
                    if op == 'cmp': self.arithmetic_flags((x-b)&0xffff, x<b)
                    else:
                        result = {'or': lambda: x|b, 'xor': lambda: x^b,
                                  'shl': lambda: x<<b}[op]() & 0xffff
                        self.put(dst, result)
                        self.arithmetic_flags(result)
            elif op == 'inc':
                self.put(a, self.get(a)+1)
                self.arithmetic_flags(self.get(a), bool(self.flags & 1))
            elif op in ('jne', 'je', 'jz', 'ja', 'jmp'):
                z, c = bool(self.flags & 0x40), bool(self.flags & 1)
                if {'jne': not z, 'je': z, 'jz': z, 'ja': not z and not c,
                    'jmp': True}[op]: pc = LABELS[a.split()[-1]]
            elif op == 'call':
                if a=='_pal_native_repaint_event':
                    assert self.r['ds']==0x2000 and not (self.flags & 0x400)
                    assert self.get('last_focus')==2
                    self.callbacks+=1
                    # A balanced far callback may clobber all ordinary registers.
                    for k in ('ax','bx','cx','dx','si','di','bp','ds','es','fs','gs'):
                        self.r[k]=0xdead
                    self.flags=0x42
                else: self.push(pc); pc = LABELS[a]
            elif op == 'ret': pc = self.pop()
            elif op == 'retf':
                ip, cs = self.pop(), self.pop()
                if a: self.r['sp'] += int(a)
                return cs, ip
            elif op == 'int':
                assert a == '21h'
                if self.r['ax'] == 0x352f:
                    self.r['es'], self.r['bx'] = self.vector
                elif self.r['ax'] == 0x252f:
                    self.sets += 1
                    if self.set_mode == 'normal':
                        self.vector = self.r['ds'], self.r['dx']
                    elif self.set_mode == 'interpose':
                        self.vector = (0x4000, 0x1234)
                    else: assert self.set_mode == 'ignore'
                else: raise AssertionError(line)
                # deliberately clobber flags, DS and BP to exercise wrappers
                self.flags, self.r['ds'], self.r['bp'] = 0x0001, 0x7770, 0x8880
            else: raise AssertionError(line)
        raise AssertionError('runaway')

    def farcall(self, name, arg=None):
        before = dict(self.r)
        if arg is not None: self.push(arg)
        self.push(0x1234); self.push(0x5678)
        assert self.run('_pal_hook_'+name) == (0x1234, 0x5678)
        if arg is not None: assert self.pop() == arg
        assert all(self.r[k] == v for k, v in before.items() if k != 'ax')
        return self.r['ax']

    def status(self, index):
        before = dict(self.r)
        self.push(index); self.push(0x1234); self.push(0x5678)
        assert self.run('palhookstatus') == (0x1234, 0x5678)
        assert all(self.r[k] == v for k,v in before.items() if k != 'ax')
        return self.r['ax']

    def notify(self, ax):
        self.r['ax'] = ax
        # Original interrupt frame, different from current live flags.
        self.push(0x3202); self.push(0x4321); self.push(0x1234)
        before = dict(self.r)
        live = self.flags
        assert self.run('hook_entry') == (0x3000, 0x4444)
        assert self.r == before and self.flags == live
        assert [self.pop(), self.pop(), self.pop()] == [0x1234, 0x4321, 0x3202]


def test_hook():
    c = CPU()
    assert c.status(0xffff) == 0xffff
    assert c.farcall('install') == 1
    assert c.status(0xffff) == 0x805a and c.status(6) == 0xffff
    assert c.vector == (0x1000, LABELS['hook_entry'])
    assert c.farcall('install') == 2 and c.sets == 1
    for ax in (0x1684, 0x4002, 0x4001, 0x4001, 0x4005, 0x4006,
               0x4000, 0x4007, 0x1600, 0x4002): c.notify(ax)
    assert [c.farcall('read', i) for i in range(7)] == [2,2,1,1,2,1,0]
    assert [c.status(i) for i in range(6)] == [2,2,1,1,2,1]
    c.mem[0x2000, DATA['seen_bg']] = 0xffff
    c.notify(0x4001)
    assert c.farcall('read', 0) == 0 and c.farcall('read', 4) == 1
    # Exhaustively verify dispatch on every AX value (including wrap).
    for ax in range(65536): c.notify(ax)
    assert [c.farcall('read', i) for i in range(6)] == [1,3,2,2,2,2]
    assert c.farcall('remove') == 1 and c.vector == (0x3000, 0x4444)
    assert c.status(0xffff) == 0xffff and c.status(0) == 0xffff
    assert c.farcall('remove') == 1 and c.sets == 2
    assert c.farcall('install') == 1
    assert [c.farcall('read', i) for i in range(6)] == [0]*6
    c.vector = (0x4000, 0x1234)
    assert c.farcall('remove') == 2 and c.sets == 3
    assert c.vector == (0x4000, 0x1234)
    c = CPU(); c.vector = (0,0)
    assert c.farcall('install') == 0 and c.sets == 0
    c = CPU(); c.set_mode = 'ignore'
    assert c.farcall('install') == 0 and c.sets == 1
    assert c.farcall('remove') == 1 and c.sets == 1
    c = CPU(); c.set_mode = 'interpose'
    assert c.farcall('install') == 2 and c.farcall('remove') == 2 and c.sets == 1
    c = CPU(); assert c.farcall('install') == 1
    c.set_mode = 'ignore'
    assert c.farcall('remove') == 2 and c.vector == (0x1000, LABELS['hook_entry'])
    print('PASS: source instruction model, all 65536 AX values, chain frame, counters, install/remove/conflict')


def test_ne():
    spec = importlib.util.spec_from_file_location('resident', ROOT/'tools/check_win31_resident.py')
    module = importlib.util.module_from_spec(spec); spec.loader.exec_module(module)
    b = bytearray(0x100); b[:2] = b'MZ'; struct.pack_into('<I', b, 0x3c, 0x40)
    b[0x40:0x42] = b'NE'; struct.pack_into('<H', b, 0x5c, 2)
    struct.pack_into('<H', b, 0x62, 0x40)
    struct.pack_into('<H', b, 0x84, 0x40); struct.pack_into('<H', b, 0x8c, 0x41)
    assert module.check(b) == 2
    for flags in (0x51, 0x01, 0x1041, 0x00c1):
        struct.pack_into('<H', b, 0x8c, flags)
        try: module.check(b)
        except ValueError: pass
        else: raise AssertionError(flags)
    print('PASS: NE residency audit positive and negative fixtures')


def test_export():
    with tempfile.TemporaryDirectory() as d:
        path = pathlib.Path(d)/'exports.txt'
        for line, good in [('PALHOOKSTATUS @501', True), ('PALHOOKSTATUS @502', False), ('', False)]:
            path.write_text(line+'\n')
            proc = subprocess.run([sys.executable, str(ROOT/'tools/check_win31_hook_export.py'), str(path)], capture_output=True)
            assert (proc.returncode == 0) == good
    print('PASS: private export checker positive and negative fixtures')


if __name__ == '__main__':
    test_hook(); test_ne(); test_export()
    CODE,LABELS,DATA=parse(native=True)
    test_hook()
    c=CPU(); assert c.farcall('install')==1
    for ax in (0x4001,0x4005,0x4006,0x1684,0x4002,0x4002): c.notify(ax)
    assert c.callbacks==2
    print('PASS: native callback dispatch, DS, DF, low register and chain-frame model')
