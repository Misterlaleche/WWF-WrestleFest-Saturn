#!/usr/bin/env python3
"""Reproduce complete native TAG and Royal matches, exact pixels and no combat CD.

Run tools/build_assets.py first. Royal requires the supplied ROYINIT.BIN in
--asset-dir. Hardware/frame cadence and visual/audio output remain separate
Saturn validation gates; host elapsed time is never treated as console FPS.
The default cache lease is 581632 bytes. Legacy --cache-slots values remain
counts of 8 KiB allocation units, not counts of the pack's 4/8 KiB blocks.
"""
from pathlib import Path
import argparse, collections, csv, hashlib, json, os, re, struct, subprocess, sys
root=Path(__file__).resolve().parents[1]
p=argparse.ArgumentParser(description=__doc__)
p.add_argument('--asset-dir',type=Path,default=root/'cd')
p.add_argument('--build-dir',type=Path,default=root/'build/native-tests')
p.add_argument('--evidence-dir',type=Path,default=root/'evidence')
cache_args=p.add_mutually_exclusive_group()
cache_args.add_argument('--cache-bytes',type=lambda value:int(value,0),
    help='Cache arena bytes (default 581632); actual slots follow the WFSZ block size.')
cache_args.add_argument('--cache-slots',type=int,
    help='Legacy allocation units of 8192 bytes (1..128), independent of WFSZ block size.')
p.add_argument('--skip-build',action='store_true')
a=p.parse_args();assets=a.asset_dir.resolve();build=a.build_dir.resolve();evidence=a.evidence_dir.resolve()
if a.cache_slots is not None and not 1<=a.cache_slots<=128:p.error('--cache-slots must be 1..128 legacy 8 KiB units')
cache_bytes=a.cache_bytes if a.cache_bytes is not None else (a.cache_slots*8192 if a.cache_slots is not None else 581632)
if not 8192<=cache_bytes<=1048576:p.error('--cache-bytes must be 8192..1048576')
for name in ('ROM68K.BIN','SPRITES.WFP','SCENES.WFP','FONT8.BIN','ROYINIT.BIN'):
    if not (assets/name).is_file():p.error(f'missing required asset: {assets/name}')
with (assets/'SPRITES.WFP').open('rb') as stream:header=stream.read(32)
if len(header)!=32:p.error('truncated WFSZ header')
magic,version,block_bytes,raw_bytes,blocks,_,_,_=struct.unpack('>4s7I',header)
if magic!=b'WFSZ' or version!=1 or block_bytes not in (4096,8192) or raw_bytes!=8388608 or blocks!=raw_bytes//block_bytes:
    p.error('unsupported WFSZ sprite geometry')
actual_cache_slots=min(cache_bytes//block_bytes,256)
if not a.skip_build:subprocess.run([sys.executable,str(root/'tools/build_runtime_tests.py'),'--build-dir',str(build)],check=True)
evidence.mkdir(parents=True,exist_ok=True)
run=build/'run';run.mkdir(parents=True,exist_ok=True)
(run/'audit/wp1980-host').mkdir(parents=True,exist_ok=True)
for src in assets.iterdir():
    dst=run/src.name
    if src.is_file():
        if dst.is_symlink():dst.unlink()
        if not dst.exists():dst.symlink_to(src)
summary={}
for mode in ('tag','royal'):
    env=os.environ.copy();env.pop('WF_JO_TEST_CACHE_SLOTS',None)
    env.update(WF_JO_TEST_CACHE_BYTES=str(cache_bytes),
      WF_JO_TEST_ACTOR_DUMP=str(evidence/(mode+'-terminal-actors.bin')),
      WF_JO_TEST_COMMANDS=str(evidence/(mode+'-command-digests.csv')),
      WF_JO_TEST_STATS=str(evidence/(mode+'-asset-frame-stats.csv')))
    if mode=='tag':
        env.update(WP1979_SEED='1085',WF_PROBE_PAD_SCHEDULE=str(root/'tests/native/tag-rom-v4-input.csv'),
                   WF_PROBE_FRAMES='20000',WF_PROBE_REQUIRE_COMPLETE='1',WF_PROBE_REQUIRE_RESIDENT='1')
        args=[str(build/'tag_probe'),str(assets/'ROM68K.BIN')]
    else:args=[str(build/'royal_probe'),str(assets/'ROM68K.BIN'),'30000','1',str(assets/'ROYINIT.BIN')]
    log=evidence/(mode+'-hw1-core-replay.log')
    with log.open('w') as output:
        result=subprocess.run(args,cwd=run,env=env,stdout=output,stderr=subprocess.STDOUT)
    text=log.read_text();print(text.splitlines()[-1] if text else 'Empty native log')
    if result.returncode:raise SystemExit(f'{mode} native fixture failed ({result.returncode}); {log}')
    stats_line=next((line for line in text.splitlines() if line.startswith('JO_ASSETS ')),None)
    assert stats_line,f'{mode}: missing resident asset statistics'
    native_geometry={key:int(value) for key,value in re.findall(r'(\w+)=(\d+)',stats_line)}
    assert all(native_geometry.get(key)==value for key,value in
        {'block_bytes':block_bytes,'cache_bytes':cache_bytes,'actual_cache_slots':actual_cache_slots}.items()), \
        f'{mode}: native fixture geometry differs from the requested lease/header; relink the fixture: {stats_line}'
    match=re.search(r'(?:MATCH COMPLETE frame=|original Royal gameover frame=)(\d+)',text)
    assert match,f'{mode}: no genuine match endpoint reached'
    frame=int(match.group(1));assert frame=={'tag':9101,'royal':8474}[mode],(mode,frame)
    actor=(evidence/(mode+'-terminal-actors.bin')).read_bytes()
    assert len(actor)==2412
    digest=hashlib.sha256(actor).hexdigest()
    # TAG reference changed only after the original20CCC branches were restored.
    expected_actors={'tag':'08015d9988c77a0d9bfa5b6001675dbd3cd1d3b3420052423e7bf32c50ea194a',
                     'royal':'656f7934c1c1b1cbac5afef25e4d17c8503c050c011effc734a0e00e1fb6f884'}
    assert digest==expected_actors[mode],digest
    rows=list(csv.DictReader((evidence/(mode+'-asset-frame-stats.csv')).open()))
    misses=[int(row['block_misses']) for row in rows]
    assert not any(int(row['backing_reads']) for row in rows),f'{mode} caused combat disc reads'
    summary[mode]={'endpoint_frame':frame,'actor_bytes':len(actor),'actor_sha256':digest,
        'block_bytes':block_bytes,'cache_bytes':cache_bytes,'actual_cache_slots':actual_cache_slots,
        'cache_slots':actual_cache_slots,'cache_usable_bytes':actual_cache_slots*block_bytes,
        'legacy_cache_8k_units':a.cache_slots,
        'block_misses':sum(misses),'frames_with_misses':sum(v>0 for v in misses),
        'miss_histogram':dict(sorted(collections.Counter(misses).items())),
        'scene_block_decodes':sum(int(row['scene_decodes']) for row in rows),'backing_reads':0}
(evidence/'native-replay-summary.json').write_text(json.dumps(summary,indent=2)+'\n')
print('PASS: native TAG9101 / Royal8474, original Royal terminal bytes, no combat backing-store reads')
