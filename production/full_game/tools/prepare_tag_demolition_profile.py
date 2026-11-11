#!/usr/bin/env python3
"""Build an isolated R18Q/R18K ROM profile from a natural TAG 8+9 trace."""
import argparse
import hashlib
import json
import re
from prepare_rom_preload_pack import make_pack, parse_profile
import struct
from pathlib import Path

EXPECTED_ARCHIVE = 'c4199a83243feb5b8c49beddd9902ef4936fddb57e1deb76b950dcfdcf025299'
EXPECTED_GENERIC = '0befffbab418ead10d9edea156b1297253509178acc175cb4e2f732b13a0359c'
FAST = frozenset((48,129,156,162,163,165,227,231,241,243,249,261,283,285,286,287,289,292,294,352,376,418,419))


def digest(data):
    return hashlib.sha256(data).hexdigest()


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    for name in ('archive','generic','native-report','out'):
        parser.add_argument('--'+name, type=Path, required=True)
    args = parser.parse_args()
    if args.out.exists():
        parser.error('output must be a new directory')
    archive = args.archive.read_bytes()
    if digest(archive) != EXPECTED_ARCHIVE:
        parser.error('unexpected original ROM archive')
    header = args.generic.read_bytes()
    if digest(header) != EXPECTED_GENERIC:
        parser.error('unexpected original generic page table')
    generic = set(map(int, re.findall(rb'\b\d+\b', header.split(b'=',1)[1])))
    if len(generic) != 209:
        parser.error('generic page count changed')
    report = json.loads(args.native_report.read_text())
    if report.get('status') != 'PASS' or report.get('cartridge') != 'none':
        parser.error('requires passing natural no-cartridge report')
    milestones = {row['name']: row['after'] for row in report['milestones']}
    start = milestones['tag-bout-3-start']
    if [start.get('wf_handoff_id'+str(i)) for i in (0,1,4,5)] != [0,7,8,9] or start.get('wf_tag_profile_key') != 0x381:
        parser.error('observed third bout is not the Demolition key T381')
    events = [row for row in report['rom_cache_events'] if row['epoch'] == start['epoch']]
    if not events or events[0]['frame'] > 1 or events[-1]['frame'] < 317:
        parser.error('observed ROM cache trace is too short')
    observed = set().union(*(set(row['pages']) for row in events)) - set(events[0]['pages'])
    main_pages = sorted((generic | observed) - FAST)
    fast_pages = sorted(FAST)
    if not main_pages or len(main_pages) > 225 or len(fast_pages) > 23 or len(main_pages)+len(fast_pages) > 264:
        parser.error('ROM cache capacity exceeded')
    if set(main_pages) & FAST:
        parser.error('overlap in R18Q profile')
    pages = main_pages + fast_pages
    profile = struct.pack('>4sHH', b'R18Q',len(main_pages),len(fast_pages)) + struct.pack('>'+str(len(pages))+'H', *pages)
    key, main, fast, parsed = parse_profile('T381P.BIN', profile)
    if key != 0x381 or main != len(main_pages) or fast != len(fast_pages) or list(parsed) != pages:
        parser.error('generated R18Q profile failed round trip')
    packed, record = make_pack('T381P.BIN', profile, archive[:524288])
    args.out.mkdir(parents=True)
    (args.out/'T381P.BIN').write_bytes(profile)
    (args.out/'T381R.BIN').write_bytes(packed)
    result = {'status':'HOST_ASSETS_VERIFIED','key':'T381','source_archive_sha256':EXPECTED_ARCHIVE,
              'source_generic_sha256':EXPECTED_GENERIC,'source_report':str(args.native_report),
              'observed_through_frame':events[-1]['frame'],'main_count':len(main_pages),'fast_count':len(fast_pages),
              'main_headroom':225-len(main_pages),'observed_pages_uncovered':sorted(observed-set(pages)),
              'profile_sha256':digest(profile),'rom_pack':record,
              'scope':'Host asset construction only; no native/physical speed claim and no later-fight page proof'}
    (args.out/'manifest.json').write_text(json.dumps(result,indent=2)+'\n')
    print(json.dumps({k:v for k,v in result.items() if k != 'rom_pack'}))


if __name__ == '__main__':
    main()
