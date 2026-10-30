#!/usr/bin/env python3
"""Pack a single original paid-Royal initial condition for native live play."""
import argparse
import hashlib
import json
from pathlib import Path
import struct
import zlib

parser=argparse.ArgumentParser()
parser.add_argument("--capture",type=Path,required=True)
parser.add_argument("--program",type=Path,required=True)
parser.add_argument("--out",type=Path,required=True)
args=parser.parse_args()
raw=(args.capture/"entry_ram.bin").read_bytes()
regs=json.loads((args.capture/"entry_registers.json").read_text())
with (args.capture/"frames.bin").open("rb") as file:
    frame=file.read(106520)
program=args.program.read_bytes()
sha=lambda data:hashlib.sha256(data).hexdigest()
assert len(raw)==16384 and len(frame)==106520
assert sha(raw)=="1bbb45c418ce65ed13e481963c6a6c5dc73fb74a6736c6d471bd2b7b79ea9433"
assert sha(program)=="265ac89deeb7e708ac6fda724356bfd6af1c9012928a4e551e6e2faa424c2306"
capture,logic,mame,priority=struct.unpack_from(">4I",frame)
assert capture==logic==0 and regs["PC"]==0xf94
offset=16+16384
sprites=frame[offset:offset+8192];offset+=8192
fgmap=frame[offset:offset+4096];offset+=4096
bgmap=frame[offset:offset+4096];offset+=4096
text=frame[offset:offset+8192];offset+=8192
palette=frame[offset:offset+65536];offset+=65536
scroll=frame[offset:offset+8];offset+=8
assert offset==len(frame)
sections=[("entry_ram",raw),("entry_D0_D7",struct.pack(">8I",*(regs[f"D{i}"]for i in range(8)))),
          ("frame0_text_c0000",text),("frame0_sprites_c2000",sprites),
          ("frame0_fgmap_80000",fgmap),("frame0_bgmap_82000",bgmap),
          ("frame0_palette_180000",palette),("frame0_scroll_100000",scroll)]
payload=b"".join(data for _,data in sections)
header=bytearray(128)
struct.pack_into(">8s6I",header,0,b"ROYINIT1",1,128+len(payload),regs["PC"],priority,regs["SR"],zlib.crc32(payload))
header[32:64]=bytes.fromhex(sha(raw));header[64:96]=bytes.fromhex(sha(frame));header[96:128]=bytes.fromhex(sha(program))
blob=bytes(header)+payload
args.out.parent.mkdir(parents=True,exist_ok=True);args.out.write_bytes(blob)
metadata={"format":"ROYINIT1","bytes":len(blob),"sha256":sha(blob),
          "kind":"Single original initial condition; subsequent combat is the live native scheduler, not recorded frames",
          "entry_pc":regs["PC"],"entry_sr":regs["SR"],"entry_ram_sha256":sha(raw),
          "frame0_source_sha256":sha(frame),"program_sha256":sha(program),
          "video_source":{"capture":capture,"logic_frame":logic,"mame_frame":mame,"priority":priority},
          "continuation":"Original F8C helper: idempotent BSET76, CLR80, clear frame handshake, first RNG call once",
          "registers":regs,"sections":[]}
offset=128
for name,data in sections:
    metadata["sections"].append({"name":name,"offset":offset,"bytes":len(data),"sha256":sha(data)})
    offset+=len(data)
args.out.with_suffix(".json").write_text(json.dumps(metadata,indent=2)+"\n")
print(json.dumps({key:metadata[key] for key in ("format","bytes","sha256","kind")},indent=2))
