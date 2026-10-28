#!/usr/bin/env python3
"""Compare the new disc's ISO files with the user's prior SFX-only disc."""
import hashlib
import json
from pathlib import Path

from iso_to_bin_exact import sector
from verify_disc_exact import directory_files

ROOT=Path(__file__).resolve().parents[1]
OLD=ROOT/'entregas/WrestleFest-Saturn-diagnostico-audio-20261026/wrestlefest-runtime18-audio-isolation.bin'
NEW=ROOT/'validation/local-20261028/eval-r05-build/results/wrestlefest-runtime18.bin'
OUT=ROOT/'validation/local-20261028/eval-r05-build/comparison-isolation.json'
EXPECTED_OLD='0eb192d42a0e9be59db7f88e846e407608baeaa8e1d0591e672c63bc8f8f9b5e'
EXPECTED_NEW='51354bdd232bdb9ca70b61385490f80e149bd65d3a96b7fbfe015da20b53ab75'

def extract(path, expected, check_sectors):
    raw=path.read_bytes()
    assert hashlib.sha256(raw).hexdigest()==expected and len(raw)%2352==0
    iso=bytearray(len(raw)//2352*2048)
    for lba in range(len(raw)//2352):
        block=raw[lba*2352:(lba+1)*2352];payload=block[16:2064]
        if check_sectors:assert sector(payload,lba)==block,('sector',lba)
        iso[lba*2048:(lba+1)*2048]=payload
    return directory_files(iso),len(raw)//2352

old,old_sectors=extract(OLD,EXPECTED_OLD,False)
new,new_sectors=extract(NEW,EXPECTED_NEW,True)
assert set(old)==set(new)
changed=[name for name in sorted(old) if old[name]['data']!=new[name]['data']]
assert changed==['A.BIN'],changed
assert hashlib.sha256(new['SCSP.ELF']['data']).hexdigest()=='780445651e1353d6544b8efc8ef6e975110862ecfe36f7f05c5085fbd71f0ac5'
result={'status':'PASS','old_bin_sha256':EXPECTED_OLD,'new_bin_sha256':EXPECTED_NEW,
        'old_sectors':old_sectors,'new_sectors':new_sectors,'cdfs_files':len(new),
        'changed_cdfs_payloads':changed,'new_all_sectors_edc_ecc_exact':True,
        'old_executable_sha256':hashlib.sha256(old['A.BIN']['data']).hexdigest(),
        'new_executable_sha256':hashlib.sha256(new['A.BIN']['data']).hexdigest(),
        'sfx_firmware_sha256':hashlib.sha256(new['SCSP.ELF']['data']).hexdigest(),
        'limits':['Compares data payloads; sector placement and disc size can differ.',
                  'Does not measure Saturn hardware speed or physical compatibility.']}
OUT.write_text(json.dumps(result,indent=2)+'\n')
print(json.dumps(result))
