#!/usr/bin/env python3
import hashlib, subprocess, sys, tempfile
from pathlib import Path
root=Path(sys.argv[1]); rom=Path(sys.argv[2])
sha=lambda p: hashlib.sha256(p.read_bytes()).hexdigest()
with tempfile.TemporaryDirectory(prefix='wp12r03-repro-') as td:
    td=Path(td); p=td/'preview'; g=td/'golden'
    p.mkdir(); g.mkdir()
    subprocess.run([sys.executable,str(root/'tools/build_r03_animated_preview.py'),str(rom),str(p),str(g)],check=True,stdout=subprocess.PIPE,stderr=subprocess.PIPE,text=True)
    # All generator outputs must match package byte-for-byte.  README is authored documentation.
    expected_preview={x.name:x for x in (root/'preview').iterdir() if x.is_file() and x.name!='README.md'}
    got_preview={x.name:x for x in p.iterdir() if x.is_file()}
    assert set(got_preview)==set(expected_preview),(sorted(set(got_preview)-set(expected_preview)),sorted(set(expected_preview)-set(got_preview)))
    for name,q in expected_preview.items(): assert sha(q)==sha(got_preview[name]),name
    expected_golden={x.name:x for x in (root/'golden').iterdir() if x.is_file()}
    got_golden={x.name:x for x in g.iterdir() if x.is_file()}
    assert set(got_golden)==set(expected_golden),(sorted(set(got_golden)-set(expected_golden)),sorted(set(expected_golden)-set(got_golden)))
    for name,q in expected_golden.items(): assert sha(q)==sha(got_golden[name]),name
print('WP12 R03 clean preview/golden regeneration: PASS')
