#!/usr/bin/env python3
"""Exercise the real builder and loader on both sides of the CD-header boundary.
Verifies IDs/payload against SPRITES.PAK, old-format compatibility, hot reads,
fallback reads, exact file size and insufficient-buffer rejection.
"""
from pathlib import Path
import re,subprocess,sys,tempfile
root=Path(__file__).resolve().parents[2]
pack=root/'audit/runtime18-cd/SPRITES.PAK'
source=pack.read_bytes()
make=(root/'Makefile').read_text()
files=[]
for name in ('CORE','R18_HOST'):
 files+=re.search(r'^'+name+r'=(.*)$',make,re.M).group(1).split()
with tempfile.TemporaryDirectory(prefix='wf-hot-boundary-') as temp:
 temp=Path(temp);test=temp/'test_hot'
 subprocess.run(['cc','-std=c11','-O1','-Wall','-Wextra','-Werror','-Iinclude',*files,'tests/r18/test_hot_sprites.c','-o',str(test)],cwd=root,check=True)
 for count in (1204,2044,2045,2195,4092):
  ids=[i*(17 if count<=2195 else 13) for i in range(count)];path=temp/'ids.txt';path.write_text(''.join(f'{i}\n' for i in ids))
  output=temp/'HOTSPR.BIN'
  subprocess.run([sys.executable,str(root/'tools/build_r18_hot_sprites.py'),str(path),str(pack),str(output)],check=True)
  header_bytes=4096 if count<=2044 else 8192
  header=bytearray(header_bytes);header[:4]=b'R18H';header[4:6]=count.to_bytes(2,'big');header[6:8]=(128).to_bytes(2,'big')
  for i,tile in enumerate(ids):header[8+i*2:10+i*2]=tile.to_bytes(2,'big')
  expected=bytes(header)+b''.join(source[tile*128:(tile+1)*128] for tile in ids)
  assert output.read_bytes()==expected,('header/IDs/pixel payload',count)
  subprocess.run([str(test),str(pack),str(output)],check=True)
 # Reject lists whose IDs cannot fit even the extended header.
 path.write_text(''.join(f'{i}\n' for i in range(4093)))
 result=subprocess.run([sys.executable,str(root/'tools/build_r18_hot_sprites.py'),str(path),str(pack),str(output)],capture_output=True)
 assert result.returncode!=0,'Oversized ID list accepted'
 print('PASS HOTSPR 1204/2044/2045/2195/4092: exact format/pixels, compatible reads and buffer boundaries')
