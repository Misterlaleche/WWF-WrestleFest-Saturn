"""Fail-closed, test-only 68000 interpreter for the two audited referee leaves.

Not a general emulator, timing model, or independent Musashi certification.
Decodes original bytes, including branches/subroutine calls; unknown opcodes,
unaligned word accesses and unmapped memory raise instead of being skipped.
A-register comparisons retain 32 bits; only memory cycles use 24-bit addresses.
"""
MASK = 0xffffffff
RAM = 0x1c0000
REF = 0x1c11f4
STOP = 0x70000


def signed(value, bits):
    return (value & ((1 << bits) - 1)) - ((1 << bits) if value & (1 << (bits - 1)) else 0)


class CPU:
    def __init__(self, rom, ram, d):
        self.rom = bytes(rom)
        self.ram = bytearray(ram)
        self.stack = bytearray(0x1000)
        self.d = list(d)
        self.a = [REF] + [0] * 6 + [0x1d0800]
        self.pc = 0
        self.n = self.z = self.v = self.c = False
        self.trace = []
        self.rom_writes = []
        self.push(STOP)

    def read(self, address, size):
        at = address & 0xffffff
        if size > 1 and at & 1:
            raise ValueError(f'unaligned read {at:x}/{size}')
        if at + size <= len(self.rom):
            return int.from_bytes(self.rom[at:at + size], 'big')
        if RAM <= at and at + size <= RAM + len(self.ram):
            return int.from_bytes(self.ram[at-RAM:at-RAM + size], 'big')
        if 0x1d0000 <= at and at + size <= 0x1d1000:
            return int.from_bytes(self.stack[at-0x1d0000:at-0x1d0000 + size], 'big')
        raise ValueError(f'unmapped read {at:x}/{size}')

    def write(self, address, size, value):
        at = address & 0xffffff
        if size > 1 and at & 1:
            raise ValueError(f'unaligned write {at:x}/{size}')
        value &= (1 << (size * 8)) - 1
        if at + size <= len(self.rom):
            self.rom_writes.append([at, size, value])
            return
        if RAM <= at and at + size <= RAM + len(self.ram):
            self.ram[at-RAM:at-RAM+size] = value.to_bytes(size, 'big')
            return
        if 0x1d0000 <= at and at + size <= 0x1d1000:
            self.stack[at-0x1d0000:at-0x1d0000+size] = value.to_bytes(size, 'big')
            return
        raise ValueError(f'unmapped write {at:x}/{size}')

    def fetch(self, size=2):
        value = self.read(self.pc, size)
        self.pc += size
        return value

    def push(self, value):
        self.a[7] = (self.a[7] - 4) & MASK
        self.write(self.a[7], 4, value)

    def pop(self):
        value = self.read(self.a[7], 4)
        self.a[7] = (self.a[7] + 4) & MASK
        return value

    def ea(self, mode, reg, size):
        if mode == 0:
            return ('d', reg)
        if mode == 1:
            return ('a', reg)
        if mode == 2:
            return ('m', self.a[reg])
        if mode == 5:
            return ('m', (self.a[reg] + signed(self.fetch(), 16)) & MASK)
        if mode == 6:
            extension = self.fetch()
            if extension & 0x0700:
                raise ValueError('non-68000 index extension outside audited subset')
            index = (self.a if extension & 0x8000 else self.d)[(extension >> 12) & 7]
            index = signed(index, 32 if extension & 0x0800 else 16)
            return ('m', (self.a[reg] + index + signed(extension & 255, 8)) & MASK)
        if mode == 7 and reg == 1:
            return ('m', self.fetch(4))
        if mode == 7 and reg == 4:
            return ('i', self.fetch(4 if size == 4 else 2))
        raise ValueError(f'unsupported EA {mode}/{reg}, pc={self.pc:x}')

    def get(self, ea, size):
        kind, value = ea
        if kind == 'm':
            return self.read(value, size)
        if kind in ('d', 'a'):
            value = (self.d if kind == 'd' else self.a)[value]
        return value & ((1 << (size * 8)) - 1)

    def put(self, ea, size, value):
        kind, index = ea
        mask = (1 << (size * 8)) - 1
        value &= mask
        if kind == 'm':
            self.write(index, size, value)
        elif kind == 'd':
            self.d[index] = (self.d[index] & (MASK ^ mask)) | value
        elif kind == 'a':
            self.a[index] = (signed(value, 16) if size == 2 else value) & MASK
        else:
            raise ValueError('write to immediate')

    def logical(self, value, size):
        value &= (1 << (size * 8)) - 1
        self.z = value == 0
        self.n = bool(value & (1 << (size * 8 - 1)))
        self.v = self.c = False

    def arithmetic(self, dst, src, size, subtract=True):
        mask = (1 << (size * 8)) - 1
        sign = 1 << (size * 8 - 1)
        dst &= mask
        src &= mask
        result = (dst - src if subtract else dst + src) & mask
        self.logical(result, size)
        self.c = dst < src if subtract else dst + src > mask
        self.v = bool(((dst ^ src) if subtract else ~(dst ^ src)) & (dst ^ result) & sign)
        return result

    def condition(self, cc):
        return (True, False, not (self.c or self.z), self.c or self.z,
                not self.c, self.c, not self.z, self.z, not self.v, self.v,
                not self.n, self.n, self.n == self.v, self.n != self.v,
                not self.z and self.n == self.v, self.z or self.n != self.v)[cc]

    def step(self):
        start = self.pc
        op = self.fetch()
        self.trace.append(start)
        if op == 0x4e75:  # RTS
            self.pc = self.pop()
        elif op & 0xffc0 == 0x4e80:  # JSR
            target = self.ea((op >> 3) & 7, op & 7, 4)
            if target[0] != 'm':
                raise ValueError('nonmemory JSR')
            self.push(self.pc)
            self.pc = target[1]
        elif op == 0xe540:  # LSL.W #2,D0 in both referee dispatch tables
            value = self.d[0] & 0xffff
            result = (value << 2) & 0xffff
            self.put(('d', 0), 2, result)
            self.logical(result, 2)
            self.c = bool(value & 0x4000)
        elif op >> 12 == 6:  # Bcc/BSR, displacement relative to PC after opcode
            base = self.pc
            disp = signed(op & 255, 8) if op & 255 else signed(self.fetch(), 16)
            cc = (op >> 8) & 15
            if cc == 1:
                self.push(self.pc)
                self.pc = (base + disp) & MASK
            elif self.condition(cc):
                self.pc = (base + disp) & MASK
        elif op & 0xfff8 == 0x51c8:  # DBF (does not modify flags)
            base = self.pc
            disp = signed(self.fetch(), 16)
            reg = op & 7
            value = (self.d[reg] - 1) & 0xffff
            self.d[reg] = (self.d[reg] & 0xffff0000) | value
            if value != 0xffff:
                self.pc = (base + disp) & MASK
        elif op & 0xf100 == 0x7000:  # MOVEQ
            value = signed(op & 255, 8) & MASK
            self.d[(op >> 9) & 7] = value
            self.logical(value, 4)
        elif op & 0xf1c0 == 0x41c0:  # LEA
            target = self.ea((op >> 3) & 7, op & 7, 4)
            if target[0] != 'm':
                raise ValueError('nonmemory LEA')
            self.a[(op >> 9) & 7] = target[1]
        elif op >> 12 in (1, 2, 3):  # MOVE/MOVEA
            size = {1: 1, 2: 4, 3: 2}[op >> 12]
            src = self.ea((op >> 3) & 7, op & 7, size)
            value = self.get(src, size)
            dst = self.ea((op >> 6) & 7, (op >> 9) & 7, size)
            self.put(dst, size, value)
            if dst[0] != 'a':
                self.logical(value, size)
        elif op & 0xff00 == 0x0800:  # immediate BTST/BCHG/BCLR/BSET
            number = self.fetch()
            size = 4 if ((op >> 3) & 7) == 0 else 1
            target = self.ea((op >> 3) & 7, op & 7, size)
            value = self.get(target, size)
            mask = 1 << (number % (32 if size == 4 else 8))
            self.z = not bool(value & mask)
            action = (op >> 6) & 3
            if action:
                result = (value ^ mask, value & ~mask, value | mask)[action - 1]
                self.put(target, size, result)
        elif op & 0xff00 in (0x4200, 0x4a00):  # CLR/TST
            size = 1 << ((op >> 6) & 3)
            if size == 8:
                raise ValueError('unsupported CLR/TST size')
            target = self.ea((op >> 3) & 7, op & 7, size)
            value = self.get(target, size)
            if op & 0xff00 == 0x4200:
                value = 0
                self.put(target, size, value)
            self.logical(value, size)
        elif op & 0xff00 == 0x0c00:  # CMPI
            size = 1 << ((op >> 6) & 3)
            if size == 8:
                raise ValueError('unsupported CMPI size')
            value = self.fetch(4 if size == 4 else 2)
            target = self.ea((op >> 3) & 7, op & 7, size)
            self.arithmetic(self.get(target, size), value, size)
        elif op >> 12 == 11:  # CMP/CMPA (no EOR/CMPM)
            mode = (op >> 6) & 7
            if mode in (3, 7):
                size = 2 if mode == 3 else 4
                src = self.get(self.ea((op >> 3) & 7, op & 7, size), size)
                if size == 2:
                    src = signed(src, 16) & MASK
                self.arithmetic(self.a[(op >> 9) & 7], src, 4)
            elif mode <= 2:
                size = 1 << mode
                src = self.get(self.ea((op >> 3) & 7, op & 7, size), size)
                self.arithmetic(self.d[(op >> 9) & 7], src, size)
            else:
                raise ValueError(f'unsupported CMP family {op:04x}')
        elif op >> 12 == 13 and ((op >> 6) & 7) in (3, 7):  # ADDA
            size = 2 if ((op >> 6) & 7) == 3 else 4
            src = self.get(self.ea((op >> 3) & 7, op & 7, size), size)
            reg = (op >> 9) & 7
            self.a[reg] = (self.a[reg] + (signed(src, 16) if size == 2 else src)) & MASK
        elif op >> 12 == 5 and ((op >> 6) & 3) != 3:  # ADDQ/SUBQ
            size = 1 << ((op >> 6) & 3)
            target = self.ea((op >> 3) & 7, op & 7, size)
            if target[0] == 'a':
                raise ValueError('ADDQ/SUBQ address register outside audited subset')
            delta = (op >> 9) & 7 or 8
            self.put(target, size, self.arithmetic(self.get(target, size), delta, size, bool(op & 0x100)))
        else:
            raise ValueError(f'unsupported opcode {op:04x} at {start:06x}')

    def run(self, address, limit=2000, stop=STOP):
        self.pc = address
        while self.pc != stop:
            if len(self.trace) >= limit:
                raise ValueError(f'instruction budget exceeded at {self.pc:x}')
            self.step()
        return bytes(self.ram), list(self.d)
