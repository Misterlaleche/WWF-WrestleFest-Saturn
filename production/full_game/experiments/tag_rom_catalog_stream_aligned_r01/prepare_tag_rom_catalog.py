#!/usr/bin/env python3
"""Build a single indexed R18Q/R18K TAG ROM catalog from locked original bytes.

This is host preparation only. The Saturn reader and native cadence tests must
pass before replacing any direct profile or promoting the catalog.
"""
import hashlib
import importlib.util
import json
import re
import struct
from pathlib import Path

WORK = Path('/Users/alejandro/Documents/Codex/2026-10-30/files-pasted-by-the-user-encargo/work')
ROM_FILE = WORK/'session235-tc81-neighbor2-correct-r01/cd/ROM68K.BIN'
GENERIC = WORK/'session46-restored-baseline/none-shared-journal-tag-r02/derived/wf_tag_generic_rom_pages.h'
CASES = WORK/'session12-coverage-plan/case-matrix.json'
OUT = WORK/'session294-tag-rom-catalog-stream-aligned-r01'
EXPECTED = {
    ROM_FILE:'c4199a83243feb5b8c49beddd9902ef4936fddb57e1deb76b950dcfdcf025299',
    GENERIC:'0befffbab418ead10d9edea156b1297253509178acc175cb4e2f732b13a0359c',
    CASES:'27a79bbd6106628feb92848b2308a3837a90413a1bcb878db604127d72609675',
}
FAST = frozenset((48,129,156,162,163,165,227,231,241,243,249,261,283,285,286,287,289,292,294,352,376,418,419))
# Natural T381 and T10B traces both request these seven shared ROM pages.
COMMON = frozenset((157,174,175,176,177,178,179))
HEADER = 2048
INDEX = 4096*16
ALIGN = 2048
STREAM_ALIGN = 32768

def sha(data): return hashlib.sha256(data).hexdigest()
def fnv(data):
    h = 2166136261
    for value in data: h = ((h ^ value) * 16777619) & 0xffffffff
    return h

def prepare():
    raw = {}
    for path, wanted in EXPECTED.items():
        data = path.read_bytes()
        if sha(data) != wanted: raise ValueError('source changed: '+str(path))
        raw[path] = data
    original = raw[ROM_FILE][:524288]
    if sha(original) != '265ac89deeb7e708ac6fda724356bfd6af1c9012928a4e551e6e2faa424c2306':
        raise ValueError('original ROM prefix changed')
    rows = [r for r in json.loads(raw[CASES])['resource_profiles'] if r['mode']=='TAG']
    if len(rows)!=127 or len({r['key'] for r in rows})!=127: raise ValueError('TAG key coverage changed')
    match = re.search(rb'\[209\]=\{([^}]+)',raw[GENERIC])
    if not match: raise ValueError('generic source shape changed')
    generic = set(map(int,re.findall(rb'\d+',match.group(1))))
    if len(generic)!=209: raise ValueError('generic source pages changed')
    universe = set().union(*(set(row['catalog_rom_pages']) for row in rows))
    fixed = generic-universe
    if len(fixed)!=20: raise ValueError('fixed page derivation changed')
    spec = importlib.util.spec_from_file_location('rom_pack',WORK/'repo/production/full_game/tools/prepare_rom_preload_pack.py')
    module = importlib.util.module_from_spec(spec);spec.loader.exec_module(module)
    target = OUT/'TAGROM.BIN'
    if target.exists(): raise ValueError('refusing to overwrite catalog')
    payload = bytearray(HEADER+INDEX)
    records = []
    for row in sorted(rows,key=lambda x:int(x['key'][1:],16)):
        key = int(row['key'][1:],16)
        main = sorted((set(row['catalog_rom_pages']) | fixed | COMMON)-FAST)
        fast = sorted(FAST)
        if not 0<key<=0xfff or not 0<len(main)<=225 or len(fast)!=23 or len(main)+len(fast)>264:
            raise ValueError('profile memory limit: '+row['key'])
        pages = main+fast
        profile = struct.pack('>4sHH',b'R18Q',len(main),len(fast))+struct.pack('>'+str(len(pages))+'H',*pages)
        parsed_key,_,_,parsed_pages = module.parse_profile(row['key']+'P.BIN',profile)
        if parsed_key!=key or tuple(pages)!=parsed_pages: raise ValueError('profile round trip')
        pack,info = module.make_pack(row['key']+'P.BIN',profile,original)
        if pack[:4]!=b'R18K' or pack[2048:]!=b''.join(original[p*1024:(p+1)*1024] for p in pages):
            raise ValueError('original ROM byte mismatch')
        p_off = len(payload);payload.extend(profile);payload.extend(bytes((-len(payload))%STREAM_ALIGN))
        r_off = len(payload);payload.extend(pack);payload.extend(bytes((-len(payload))%ALIGN))
        struct.pack_into('>4I',payload,HEADER+key*16,p_off,len(profile),r_off,len(pack))
        records.append(dict(key=row['key'],fighters=row['fighters'],main=len(main),fast=len(fast),
                            main_headroom=225-len(main),profile_sha256=sha(profile),pack_sha256=info['file_sha256'],
                            profile_offset=p_off,profile_bytes=len(profile),pack_offset=r_off,pack_bytes=len(pack)))
    struct.pack_into('>4s8I',payload,0,b'TRC1',1,HEADER,4096,16,HEADER+INDEX,len(payload),127,0x3619de1e)
    struct.pack_into('>I',payload,2044,fnv(payload[:2044]))
    target.write_bytes(payload)
    check=target.read_bytes()
    if sha(check)!=sha(payload): raise ValueError('catalog write mismatch')
    for row in records:
        k=int(row['key'][1:],16)
        po,pn,ro,rn=struct.unpack_from('>4I',check,HEADER+k*16)
        if (po,pn,ro,rn)!=(row['profile_offset'],row['profile_bytes'],row['pack_offset'],row['pack_bytes']):
            raise ValueError('index round trip: '+row['key'])
    result=dict(status='HOST_CATALOG_VERIFIED_ONLY',catalog_sha256=sha(check),catalog_bytes=len(check),
                source_sha256={str(p):v for p,v in EXPECTED.items()},fixed_pages=sorted(fixed),common_pages=sorted(COMMON),
                profiles=len(records),max_main=max(r['main'] for r in records),min_main_headroom=min(r['main_headroom'] for r in records),
                records=records,scope='Static catalog plus seven pages observed in two natural TAG routes; no native, late-bout, or physical fluency claim')
    (OUT/'manifest.json').write_text(json.dumps(result,indent=2)+'\n')
    print(json.dumps({k:v for k,v in result.items() if k!='records'}))

if __name__=='__main__': prepare()
