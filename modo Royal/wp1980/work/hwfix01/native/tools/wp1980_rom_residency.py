#!/usr/bin/env python3
"""Place observed original ROM data pages in the two native memory leases."""
import argparse
import collections
import csv
import json
from pathlib import Path

def main():
    p=argparse.ArgumentParser()
    p.add_argument('--pages',type=Path,nargs='+',required=True)
    p.add_argument('--frame-traces',type=Path,nargs='+',required=True)
    p.add_argument('--output',type=Path,required=True)
    p.add_argument('--fast-pages',type=int,default=32)
    a=p.parse_args();pages=set();frequency=collections.Counter()
    if not 1<=a.fast_pages<=255:p.error('--fast-pages must be1..255; the linked HWRAM gate verifies its byte budget')
    for path in a.pages:
        pages.update(int(t,16 if t.startswith('0x')else 10) for t in path.read_text().splitlines()
                     if t and not t.startswith('#'))
    for path in a.frame_traces:
        for row in csv.DictReader(path.open()):
            frequency.update(int(x,16) for x in row['rom_pages'].split())
    if min(pages)<0 or max(pages)>511:raise SystemExit('Invalid original1KB page')
    fast=sorted(sorted(pages,key=lambda n:(-frequency[n],n))[:a.fast_pages])
    main_pages=sorted(pages-set(fast))
    if len(main_pages)>209 or len(fast)>255:raise SystemExit('Observed ROM pages exceed available residency')
    out=['/* WP1980 original Royal data-page residency; storage only.',' * Most frequently touched pages use HWRAM. Remaining hot pages use LWRAM.',' * Unused LWRAM slots retain the original cold-read fallback. */',
         '#ifndef WP1980_ROYAL_ROM_PAGES_H','#define WP1980_ROYAL_ROM_PAGES_H','#include <stdint.h>']
    for name,values in [('wp1980_rom_main_pages',main_pages),('wp1980_rom_fast_pages',fast)]:
        out.append('static const uint16_t '+name+'[]={')
        for i in range(0,len(values),16):out.append(' '+','.join(str(x)for x in values[i:i+16])+',')
        out.append('};')
    out.append('#endif');a.output.write_text('\n'.join(out)+'\n')
    report={'total_pages':len(pages),'lwr_main_pages':main_pages,'hwr_fast_pages':fast,
            'free_lwr_cold_slots':209-len(main_pages),
            'frequency_in_observed_frames':dict(sorted(frequency.items())),
            'sources':[str(x)for x in a.pages+a.frame_traces]}
    a.output.with_suffix('.json').write_text(json.dumps(report,indent=2)+'\n')
    print(f'Exact ROM page set:{len(pages)}; LWRAM:{len(main_pages)}; HWRAM:{len(fast)}; free cold slots:{209-len(main_pages)}')

if __name__=='__main__':main()
