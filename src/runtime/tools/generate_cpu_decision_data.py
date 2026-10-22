#!/usr/bin/env python3
"""Extract only CPU decision data from the canonical arcade ROM."""
import hashlib,sys
from pathlib import Path
rom=Path(sys.argv[1]).read_bytes()
assert hashlib.sha1(rom).hexdigest()=="a726c46f350723ea73158bc78055d17d418b14e2"
spans=[(0x1f150,0x1f15c),(0x1f866,0x1f916),(0x219b4,0x24062)]
lines=['/* Generated from canonical ROM; regenerate with tools/generate_cpu_decision_data.py. */']
for start,end in spans:
 lines.append('static const uint8_t cpu_data_%x[]={\n'%start)
 for at in range(start,end,16):lines.append(','.join('0x%02x'%b for b in rom[at:min(at+16,end)])+',\n')
 lines.append('};\n')
text='\n'.join(lines)
out=Path(__file__).resolve().parents[1]/'generated/wf_cpu_decision_data.h'
if '--check' in sys.argv:assert out.read_text()==text
else:out.write_text(text)
