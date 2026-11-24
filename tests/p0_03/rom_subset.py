"""Test-only opcode runner for $280DC and $1129E and their actual callers.

Extends the earlier P0-02 interpreter. Not independent CPU certification: no
cycle accuracy, A7 exception frames, interrupts or general device model. Unknown
opcodes/address spaces fail closed. No original ROM instructions are patched.
"""
import importlib.util
from pathlib import Path
_spec = importlib.util.spec_from_file_location('p002_rom_subset', Path(__file__).resolve().parents[1]/'p0_02/rom_subset.py')
_base = importlib.util.module_from_spec(_spec)
_spec.loader.exec_module(_base)
signed, MASK, RAM, STOP = _base.signed, _base.MASK, _base.RAM, _base.STOP


class CPU(_base.CPU):
    def ea(self, mode, reg, size):
        if mode == 3:
            at = self.a[reg]
            self.a[reg] = (at + (2 if reg == 7 and size == 1 else size)) & MASK
            return ('m', at)
        if mode == 4:
            self.a[reg] = (self.a[reg] - (2 if reg == 7 and size == 1 else size)) & MASK
            return ('m', self.a[reg])
        if mode == 7 and reg == 0:
            return ('m', signed(self.fetch(), 16) & MASK)
        if mode == 7 and reg == 2:
            at = self.pc
            return ('m', (at + signed(self.fetch(), 16)) & MASK)
        return super().ea(mode, reg, size)

    def step(self):
        start = self.pc
        op = self.read(start, 2)
        special = (op in (0x48e7, 0x4cdf, 0x003c, 0x023c) or
                   op & 0xffc0 == 0x4ec0 or op & 0xfff8 == 0x4840 or
                   op & 0xff00 in (0x0000, 0x0200, 0x0400, 0x0600, 0x0a00, 0x4400) or
                   op & 0xf1c0 in (0xc0c0, 0x81c0) or
                   op >> 12 in (8, 9, 12, 13, 14))
        if not special:
            return super().step()
        self.fetch(); self.trace.append(start)
        if op in (0x48e7, 0x4cdf):
            mask = self.fetch()
            if op == 0x48e7:  # MOVEM.L registers,-(A7), reversed register mask
                regs = self.d + self.a
                for i in range(16):
                    if mask & (1 << i): self.push(regs[15-i])
            else:  # MOVEM.L (A7)+,registers
                for i in range(16):
                    if mask & (1 << i):
                        value = self.pop()
                        (self.d if i < 8 else self.a)[i % 8] = value
        elif op in (0x003c, 0x023c):
            imm = self.fetch()
            value = int(self.c) | (int(self.v) << 1) | (int(self.z) << 2) | (int(self.n) << 3)
            value = value | imm if op == 0x003c else value & imm
            self.c, self.v, self.z, self.n = [bool(value & (1 << i)) for i in range(4)]
        elif op & 0xffc0 == 0x4ec0:
            dest = self.ea((op >> 3) & 7, op & 7, 4)
            if dest[0] != 'm': raise ValueError('JMP not address')
            self.pc = dest[1]
        elif op & 0xfff8 == 0x4840:
            reg = op & 7; value = self.d[reg]
            self.d[reg] = ((value << 16) | (value >> 16)) & MASK
            self.logical(self.d[reg], 4)
        elif op & 0xff00 == 0x4400:
            size = 1 << ((op >> 6) & 3)
            if size == 8: raise ValueError('NEG invalid size')
            dst = self.ea((op >> 3) & 7, op & 7, size)
            self.put(dst, size, self.arithmetic(0, self.get(dst, size), size))
        elif op & 0xff00 in (0x0000, 0x0200, 0x0400, 0x0600, 0x0a00):
            size = 1 << ((op >> 6) & 3)
            if size == 8: raise ValueError('immediate size')
            src = self.fetch(4 if size == 4 else 2)
            dst = self.ea((op >> 3) & 7, op & 7, size)
            old = self.get(dst, size); family = op & 0xff00
            if family in (0x0400, 0x0600):
                value = self.arithmetic(old, src, size, family == 0x0400)
            else:
                value = old | src if family == 0 else old & src if family == 0x0200 else old ^ src
                self.logical(value, size)
            self.put(dst, size, value)
        elif op & 0xf1c0 == 0xc0c0:  # MULU.W -> LONG
            src = self.get(self.ea((op >> 3) & 7, op & 7, 2), 2); reg = (op >> 9) & 7
            self.d[reg] = (self.d[reg] & 0xffff) * src
            self.logical(self.d[reg], 4)
        elif op & 0xf1c0 == 0x81c0:  # DIVS.W -> remainder:quotient
            den = signed(self.get(self.ea((op >> 3) & 7, op & 7, 2), 2), 16)
            reg = (op >> 9) & 7; num = signed(self.d[reg], 32)
            if not den: raise ValueError('division by zero: exception not modeled')
            quot = abs(num) // abs(den)
            if (num < 0) != (den < 0): quot = -quot
            if not -32768 <= quot <= 32767:
                raise ValueError('DIVS overflow flags outside audited input domain')
            rem = num - quot * den
            self.d[reg] = ((rem & 0xffff) << 16) | (quot & 0xffff)
            self.logical(quot, 2)
        elif op >> 12 in (9, 13):
            family = op >> 12; mode = (op >> 6) & 7; reg = (op >> 9) & 7
            if mode in (3, 7):
                size = 2 if mode == 3 else 4
                src = self.get(self.ea((op >> 3) & 7, op & 7, size), size)
                if size == 2: src = signed(src, 16)
                self.a[reg] = (self.a[reg] + (src if family == 13 else -src)) & MASK
            else:
                size = 1 << (mode & 3)
                if mode < 3:
                    src = self.get(self.ea((op >> 3) & 7, op & 7, size), size)
                    dst = ('d', reg)
                else:
                    dst = self.ea((op >> 3) & 7, op & 7, size)
                    src = self.get(('d', reg), size)
                self.put(dst, size, self.arithmetic(self.get(dst, size), src, size, family == 9))
        elif op >> 12 in (8, 12):
            mode = (op >> 6) & 7; reg = (op >> 9) & 7
            if mode in (3, 7): raise ValueError(f'unsupported arithmetic family {op:04x}')
            size = 1 << (mode & 3)
            if mode < 3:
                src = self.get(self.ea((op >> 3) & 7, op & 7, size), size); dst = ('d', reg)
            else:
                src = self.get(('d', reg), size); dst = self.ea((op >> 3) & 7, op & 7, size)
            old = self.get(dst, size)
            value = old | src if op >> 12 == 8 else old & src
            self.put(dst, size, value); self.logical(value, size)
        elif op >> 12 == 14:
            size_code = (op >> 6) & 3
            if size_code == 3: raise ValueError('memory shift not modeled')
            size = 1 << size_code; bits = size * 8; mask = (1 << bits) - 1
            kind = (op >> 3) & 3; left = bool(op & 0x100); reg = op & 7
            if kind == 2: raise ValueError('ROX needs X flag')
            count = (self.d[(op >> 9) & 7] & 63) if op & 0x20 else (((op >> 9) & 7) or 8)
            value = self.d[reg] & mask; carry = False; overflow = False
            for _ in range(count):
                old_sign = bool(value & (1 << (bits-1)))
                if left:
                    carry = old_sign; value = (value << 1) & mask
                    if kind == 3 and carry: value |= 1
                    if kind == 0: overflow |= old_sign != bool(value & (1 << (bits-1)))
                else:
                    carry = bool(value & 1); value >>= 1
                    if (kind == 3 and carry) or (kind == 0 and old_sign): value |= 1 << (bits-1)
            self.put(('d', reg), size, value); self.logical(value, size)
            self.c = carry; self.v = overflow
        else:
            raise ValueError(f'unimplemented {op:04x} at {start:x}')

    def execute_to(self, start, stop=STOP, limit=4000):
        self.pc = start
        for _ in range(limit):
            if self.pc == stop: return bytes(self.ram), list(self.d)
            self.step()
        raise ValueError(f'instruction budget exhausted at {self.pc:x}')
