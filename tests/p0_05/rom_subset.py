"""P0-05 test-only subset; not a general or independent 68000 emulator."""
import importlib.util
from pathlib import Path
_spec=importlib.util.spec_from_file_location('p004_subset',Path(__file__).resolve().parents[1]/'p0_04/rom_subset.py')
_base=importlib.util.module_from_spec(_spec);_spec.loader.exec_module(_base)
class CPU(_base.CPU):
    def step(self):
        start=self.pc
        if self.read(start,2)!=0x4e71:
            return super().step()
        # NOP has no register, flag or memory side effect.
        self.fetch();self.trace.append(start)
