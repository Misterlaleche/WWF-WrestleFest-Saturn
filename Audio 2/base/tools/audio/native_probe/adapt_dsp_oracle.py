"""Explicit test-only hardware-format adapter; never edits pinned MAME sources."""
import hashlib,json
from pathlib import Path
def generate(source:Path,out:Path):
 original=source.read_bytes();text=original.decode()
 changes=[('u32 const NOFL  = (IPtr[3] >> 15) & 0x01;  //????',
           'u32 const NOFL  = (IPtr[3] >>  8) & 0x01;  // SCSP hardware format'),
          ('ADDR += ADRS_REG & 0x0FFF;',
           'ADDR += util::sext(ADRS_REG, 12);  // SCSP signed address register')]
 for before,after in changes:
  if text.count(before)!=1:raise ValueError('Pinned DSP oracle changed: '+before)
  text=text.replace(before,after)
 out.mkdir(parents=True,exist_ok=True);adapted=out/'scspdsp-hardware.cpp';adapted.write_text(text)
 report=dict(upstream=str(source),upstream_sha256=hashlib.sha256(original).hexdigest(),
             adapted_sha256=hashlib.sha256(adapted.read_bytes()).hexdigest(),
             explicit_adaptations=changes,upstream_modified=False,
             reference='Beetle fdf11045c5989471645a39987b3916d31053068f scsp.inc; separate Beetle PCM tests required')
 (out/'dsp-oracle-adapter.json').write_text(json.dumps(report,indent=2)+'\n')
 return adapted
