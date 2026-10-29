#!/usr/bin/env python3
"""Reject a linked Saturn image that crowds Yaul's runtime heap.

The SDK linker allows data up to06100000, but its TLSF control and runtime
allocations also start at ___end. A successful link alone is insufficient.
"""
import argparse,json,re
from pathlib import Path

p=argparse.ArgumentParser(description=__doc__)
p.add_argument('map',type=Path)
p.add_argument('--reserve',type=lambda s:int(s,0),default=16384)
p.add_argument('--json',type=Path)
a=p.parse_args()
matches=re.findall(r'0x([0-9a-fA-F]+)\s+___end\s*=',a.map.read_text())
assert len(matches)==1,'Missing/ambiguous linked ___end'
end=int(matches[0],16)
remaining=0x06100000-end
ok=0x06004000<=end<0x06100000 and remaining>=a.reserve
result=dict(status='PASS' if ok else 'FAIL',linked_end=hex(end),
            heap_available=remaining,heap_reserved=a.reserve,
            scope='HWRAM linked image and runtime heap; stack below06004000')
if a.json:a.json.write_text(json.dumps(result,indent=2)+'\n')
print(json.dumps(result))
raise SystemExit(0 if ok else 1)
