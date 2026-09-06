#!/usr/bin/env python3
"""Check Saturn IP header, ISO9660 PVD and relative CUE references."""
import argparse
from pathlib import Path
import re
import hashlib
import json

def directory_files(data):
    pvd=data[16*2048:17*2048];root=pvd[156:190]
    sector=int.from_bytes(root[2:6],'little');size=int.from_bytes(root[10:14],'little')
    directory=data[sector*2048:sector*2048+size];files={};offset=0
    while offset<len(directory):
        length=directory[offset]
        if not length:offset=(offset//2048+1)*2048;continue
        r=directory[offset:offset+length];assert len(r)==length and length>=34
        offset+=length
        if r[25]&2:continue
        name=r[33:33+r[32]].decode('ascii').split(';')[0]
        lba=int.from_bytes(r[2:6],'little');size=int.from_bytes(r[10:14],'little')
        assert lba==int.from_bytes(r[6:10],'big') and size==int.from_bytes(r[14:18],'big')
        assert name not in files and (lba*2048+size)<=len(data)
        files[name]=data[lba*2048:lba*2048+size]
    return files

def verify(folder):
    folder=Path(folder)
    root=Path(__file__).resolve().parents[1]
    makefile=(root/'Makefile').read_text()
    def setting(name):
        return re.search(r'^'+name+r'\s*:=\s*(.+)$',makefile,re.M).group(1).strip()
    stem=setting('SH_PROGRAM')
    iso=folder/(stem+'.iso'); cue=folder/(stem+'.cue')
    data=iso.read_bytes()
    assert len(data)%2048==0, 'ISO is not sector-aligned'
    assert data[:16]==b'SEGA SEGASATURN ', repr(data[:16])
    assert setting('IP_VERSION').encode() in data[:256], 'Version header mismatch'
    assert setting('IP_TITLE').encode() in data[:256], 'Title mismatch'
    assert b'U' in data[0x40:0x50], 'Missing NTSC-USA area'
    assert data[16*2048+1:16*2048+6]==b'CD001', 'Missing ISO9660 PVD'
    names=re.findall(r'FILE\s+"([^"]+)"',cue.read_text())
    assert names==[iso.name], f'Unexpected CUE references: {names}'
    assert 'MODE1/2048' in cue.read_text(), 'Unexpected CUE track mode'
    files=directory_files(data)
    expected={p.name:p for p in (root/'cd').iterdir() if p.is_file()}
    assert set(files)==set(expected), 'CD root file set mismatch'
    main=(root/'main.c').read_text()
    m=re.search(r'CDFS_FILE_CAPACITY\s*=\s*(\d+)',main)
    assert m, 'CDFS_FILE_CAPACITY missing from main.c'
    capacity=int(m.group(1))
    assert len(files)+2<=capacity, f'Saturn CDFS file list capacity exceeded: {len(files)+2}>{capacity}'
    for name,path in expected.items():assert files[name]==path.read_bytes(),f'CD file mismatch: {name}'
    assert 'A.BIN' in files and len(files['A.BIN'])>0, 'Missing boot executable'
    assert files['A.BIN']==(root/'build'/(stem+'.bin')).read_bytes()
    record={'version':setting('IP_VERSION'),'title':setting('IP_TITLE'),'iso_sha256':hashlib.sha256(data).hexdigest(),
            'sectors':len(data)//2048,'root_files':len(files),'cdfs_capacity':capacity,
            'all_cd_files_identical':True,'boot_executable_identical':True,
            'usa_area':True,'boot_test':False}
    (root/'docs/iso-validation.json').write_text(json.dumps(record,indent=2)+'\n')
    print('PASS: Saturn IP, USA area, version/title, ISO9660, relative CUE')
    print(f'PASS: all {len(files)} CD files match their build inputs, including A.BIN')
    print(f'PASS: CDFS entries {len(files)+2}/{capacity}')
    print('This is a structural check, not an emulator or hardware boot test.')

if __name__=='__main__':
    p=argparse.ArgumentParser();p.add_argument('folder',type=Path)
    verify(p.parse_args().folder)
