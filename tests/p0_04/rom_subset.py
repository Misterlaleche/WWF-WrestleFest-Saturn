"""P0-04 test-only extension. Not a general or independent 68000 emulator."""
import importlib.util
from pathlib import Path
_spec=importlib.util.spec_from_file_location('p003_subset',Path(__file__).resolve().parents[1]/'p0_03/rom_subset.py')
_base=importlib.util.module_from_spec(_spec);_spec.loader.exec_module(_base)
class CPU(_base.CPU):
    def step(self):
        start=self.pc;op=self.read(start,2)
        if op & 0xf1c0 != 0x0140:
            return super().step()
        # BCHG Dn,<ea>: LONG register / BYTE memory; only Z is affected.
        self.fetch();self.trace.append(start)
        mode=(op>>3)&7;reg=op&7
        size=4 if mode==0 else 1
        bit=self.d[(op>>9)&7] % (32 if size==4 else 8)
        dst=self.ea(mode,reg,size);old=self.get(dst,size)
        self.z=not bool(old & (1<<bit))
        self.put(dst,size,old ^ (1<<bit))
