#!/usr/bin/env python3
"""Strict structural verification for a CLEAN-R01 single-data-track Saturn BIN/CUE."""
import argparse,hashlib,json,re,sys
from pathlib import Path
sys.path.insert(0,str(Path(__file__).resolve().parent))
from iso_to_bin_exact import sector

def setting(mk,name):return re.search(r'^'+re.escape(name)+r'\s*:=\s*(.+)$',mk,re.M).group(1).strip()
def directory_files(data):
    pvd=data[16*2048:17*2048];assert pvd[1:6]==b'CD001';root=pvd[156:190];sec=int.from_bytes(root[2:6],'little');size=int.from_bytes(root[10:14],'little');d=data[sec*2048:sec*2048+size];files={};off=0
    while off<len(d):
        ln=d[off]
        if not ln:off=(off//2048+1)*2048;continue
        r=d[off:off+ln];off+=ln
        if r[25]&2:continue
        name=r[33:33+r[32]].decode('ascii').split(';')[0];lba=int.from_bytes(r[2:6],'little');sz=int.from_bytes(r[10:14],'little')
        assert lba==int.from_bytes(r[6:10],'big');assert sz==int.from_bytes(r[14:18],'big');assert name not in files;assert lba*2048+sz<=len(data)
        files[name]={'lba':lba,'size':sz,'data':data[lba*2048:lba*2048+sz]}
    return files
def verify(root,iso,binary,cue,build_dir='audit/runtime18-build',makefile='Makefile.saturn-disc',image_dir='audit/runtime18-cd'):
    root=Path(root);iso=Path(iso);binary=Path(binary);cue=Path(cue);build_dir=Path(build_dir);mk=(root/makefile).read_text();data=iso.read_bytes();raw=binary.read_bytes();ct=cue.read_text()
    assert len(data)%2048==0 and data[:16]==b'SEGA SEGASATURN '
    ip=(root/build_dir/'IP.BIN').read_bytes()
    assert data[:len(ip)] == ip, 'ISO boot area differs from SDK IP.BIN'
    assert data[0xf0:0xf4] == int(setting(mk,'IP_1ST_READ_ADDR'),0).to_bytes(4,'big')
    assert data[0xe4:0xe8] == int(setting(mk,'IP_MASTER_STACK_ADDR'),0).to_bytes(4,'big')
    assert data[0xec:0xf0] == int(setting(mk,'IP_SLAVE_STACK_ADDR'),0).to_bytes(4,'big')
    assert setting(mk,'IP_VERSION').encode() in data[:256];assert setting(mk,'IP_TITLE').encode() in data[:256];assert b'U' in data[0x40:0x50]
    assert data[16*2048+1:16*2048+6]==b'CD001'
    assert ct==f'FILE "{binary.name}" BINARY\n  TRACK 01 MODE1/2352\n    INDEX 01 00:00:00\n'
    assert len(raw)==len(data)//2048*2352
    for lba in range(len(data)//2048):
        expected=sector(data[lba*2048:(lba+1)*2048],lba);actual=raw[lba*2352:(lba+1)*2352];assert actual==expected,f'raw sector mismatch {lba}'
    files=directory_files(data);expected={p.name:p for p in (root/image_dir).iterdir() if p.is_file() and p.name!='A.BIN'};expected['A.BIN']=root/build_dir/(setting(mk,'SH_PROGRAM')+'.bin')
    assert set(files)==set(expected),f'CDFS root mismatch missing={set(expected)-set(files)} extra={set(files)-set(expected)}'
    manifest=[]
    for name,p in sorted(expected.items()):
        b=p.read_bytes();assert files[name]['data']==b,f'CDFS payload mismatch {name}';manifest.append({'name':name,'lba':files[name]['lba'],'bytes':len(b),'sha256':hashlib.sha256(b).hexdigest()})
    return {'status':'PASS','iso_sha256':hashlib.sha256(data).hexdigest(),'bin_sha256':hashlib.sha256(raw).hexdigest(),'cue_sha256':hashlib.sha256(ct.encode()).hexdigest(),'sectors':len(data)//2048,'cdfs_root_files':len(files),'all_sector_edc_ecc_exact':True,'all_cdfs_payloads_exact':True,'a_bin_equals_linked_binary':True,'manifest':manifest,'hardware_test':False,'emulator_test':False}
def main():
    p=argparse.ArgumentParser();p.add_argument('--root',required=True,type=Path);p.add_argument('--iso',required=True,type=Path);p.add_argument('--bin',required=True,type=Path);p.add_argument('--cue',required=True,type=Path);p.add_argument('--build-dir',default='audit/runtime18-build');p.add_argument('--makefile',default='Makefile.saturn-disc');p.add_argument('--image-dir',default='audit/runtime18-cd');p.add_argument('--json',type=Path);a=p.parse_args();r=verify(a.root,a.iso,a.bin,a.cue,a.build_dir,a.makefile,a.image_dir);txt=json.dumps(r,indent=2);print(txt);a.json and a.json.write_text(txt+'\n')
if __name__=='__main__':main()
