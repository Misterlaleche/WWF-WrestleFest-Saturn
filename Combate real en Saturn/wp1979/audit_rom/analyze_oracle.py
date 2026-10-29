#!/usr/bin/env python3
"""Independently validate the completed MAME trace and publish a compact audit."""
from pathlib import Path
from collections import Counter
import argparse,csv,hashlib,json,struct

def main():
    ap=argparse.ArgumentParser();ap.add_argument("directory",type=Path);a=ap.parse_args();p=a.directory
    meta=json.loads((p/"summary.json").read_text())
    actors=list(csv.DictReader((p/"actors.csv").open()))
    moves=list(csv.DictReader((p/"motion.csv").open()))
    assert meta["frames"]==1040 and meta["motion_observations"]==len(moves)>0
    assert meta["program_sha1"]=="a726c46f350723ea73158bc78055d17d418b14e2"
    assert len(actors)==4160
    for slot,ident in enumerate(meta["lineup"]):
        rows=[r for r in actors if int(r["slot"])==slot]
        assert [int(r["logic_frame"]) for r in rows]==list(range(1040))
        assert {int(r["id"]) for r in rows}=={ident}
        assert len({int(r["mame_frame"]) for r in rows})==1040
        assert all(int(r["active"])&0x8000 for r in rows)
    for r in moves:
        assert int(r["pass"])==1
        for axis in ("x","y"):
            d=(int(r[axis+"_after"])-int(r[axis+"_before"]))&0xffffffff
            if d>=0x80000000:d-=0x100000000
            assert d==int(r["d"+axis+"_expected"])==int(r["d"+axis+"_observed"])
    for image in p.glob("frame_*.png"):
        data=image.read_bytes();assert data[:8]==b"\x89PNG\r\n\x1a\n"
        assert struct.unpack(">II",data[16:24])==(320,240)
    expected_sizes={"ram":0x4000,"sprites":0x2000,"fg":0x1000,"bg":0x1000,"text":0x2000,"palette":0x10000,"scroll":8}
    for suffix,length in expected_sizes.items():
        files=list(p.glob("frame_*_"+suffix+".bin"));assert len(files)==9
        assert all(f.stat().st_size==length for f in files)
    relevant=[]
    for ident,name,mag in [(0,"Hogan walk",27),(0,"Hogan run",47),(1,"Warrior walk",29),(9,"Ted walk",27),(9,"Ted run",47),(7,"Perfect walk",30)]:
        samples=[r for r in moves if int(r["id"])==ident and int(r["magnitude"])==mag and int(r["angle"]) in (64,192)]
        assert samples
        assert {abs(int(r["dx_observed"])) for r in samples}=={mag*0x1400}
        relevant.append({"name":name,"samples":len(samples),"x_delta_16_16":mag*0x1400,"x_pixels_per_tick":mag*0x1400/65536})
    result={"status":"PASS","frames":1040,"actors":4160,"mode1_observations":len(moves),"mismatches":0,"width":320,"height":240,"native_snapshots":9,"fixture":meta["fixture"],"cadence":meta["refresh_hz_reference"],"character_motion_samples":relevant,"motion_states":dict(Counter(str(int(r["main_state"])&0x7fff) for r in moves)),"files_sha256":{f.name:hashlib.sha256(f.read_bytes()).hexdigest() for f in sorted(p.iterdir()) if f.is_file() and f.name!="validation.json"}}
    (p/"validation.json").write_text(json.dumps(result,indent=2)+"\n")
    print(json.dumps({k:v for k,v in result.items() if k!="files_sha256"},indent=2))
if __name__=="__main__":main()
