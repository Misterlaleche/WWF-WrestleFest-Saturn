import argparse, hashlib, json, mmap, struct
from pathlib import Path

p = argparse.ArgumentParser()
p.add_argument('--archive', type=Path, required=True)
p.add_argument('--keys', type=Path, required=True)
p.add_argument('--bitmaps', type=Path, required=True)
p.add_argument('--out', type=Path, required=True)
a = p.parse_args()
keys = [r['key'] for r in json.loads(a.keys.read_text())['rows']]
if len(keys) != 127 or len(set(keys)) != 127:
    raise SystemExit('Expected 127 distinct TAG profile keys')
rows = []
with a.archive.open('rb') as f:
    data = mmap.mmap(f.fileno(), 0, access=mmap.ACCESS_READ)
    if data[524288+160:524288+164] != b'WTP2':
        raise SystemExit('Not a WTP2 profile archive')
    pos = struct.unpack_from('>I', data, 524288+168)[0]
    for i, key in enumerate(keys):
        got_key, hot_bytes, secondary_bytes = struct.unpack_from('>III', data, 524288+192+i*12)
        if got_key != int(key[1:], 16):
            raise SystemExit('Directory key mismatch: '+key)
        pos = (pos+32767)&~32767
        hot = data[pos:pos+hot_bytes]
        pos += hot_bytes
        pos = (pos+32767)&~32767
        crop = data[pos:pos+12964]
        pos += len(crop)
        pos = (pos+32767)&~32767
        secondary = data[pos:pos+secondary_bytes]
        pos += secondary_bytes
        if len(crop) != 12964 or hot[:4] != b'R18P' or secondary[:4] != b'R18S':
            raise SystemExit('Invalid member: '+key)
        hot_ids = set(struct.unpack_from('>6474H', hot, 24))
        required = (a.bitmaps/(key+'.ids.bin')).read_bytes()
        if len(required) != 8192:
            raise SystemExit('Invalid required bitmap: '+key)
        missing = [tile for tile in range(65536) if required[tile>>3] & (1 << (tile&7)) and tile not in hot_ids and not secondary[32+(tile>>3)] & (1 << (tile&7))]
        if missing:
            rows.append({'key':key,'missing_count':len(missing),'first_missing':missing[:16]})
digest = hashlib.sha256()
with a.archive.open('rb') as f:
    for chunk in iter(lambda: f.read(1024*1024), b''):
        digest.update(chunk)
result = {'status':'PASS' if not rows else 'FAIL','profiles_checked':len(keys),'profiles_with_missing_static_tiles':len(rows),'missing_static_tiles_total':sum(r['missing_count'] for r in rows),'rows':rows,'archive_sha256':digest.hexdigest(),'scope':'Original 127 static route bitmaps; no claim for dynamic RNG or unobserved poses; no native or physical validation'}
a.out.write_text(json.dumps(result,indent=2)+'\n')
print(json.dumps({k:v for k,v in result.items() if k!='rows'}))
if rows:
    raise SystemExit(1)
