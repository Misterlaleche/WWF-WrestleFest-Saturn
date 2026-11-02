from pathlib import Path
import hashlib,json,sys
root=Path(__file__).resolve().parent
manifest=json.loads((root/'FUENTES-SHA256.json').read_text(encoding='utf-8'))
expected={row['path'] for row in manifest['files']}
actual={p.relative_to(root).as_posix() for p in root.rglob('*') if p.is_file() and '__pycache__' not in p.parts and p.name!='FUENTES-SHA256.json'}
errors=[]
if actual!=expected: errors.append({'missing':sorted(expected-actual),'extra':sorted(actual-expected)})
for row in manifest['files']:
 p=root/row['path']
 if not p.is_file(): continue
 b=p.read_bytes()
 if len(b)!=row['bytes'] or hashlib.sha256(b).hexdigest()!=row['sha256']: errors.append(row['path'])
print(json.dumps({'passed':not errors,'files':len(expected),'errors':errors},ensure_ascii=False,indent=2))
sys.exit(bool(errors))
