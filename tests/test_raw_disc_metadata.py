#!/usr/bin/env python3
from pathlib import Path
import argparse,hashlib,json,re,subprocess,os
root=Path(__file__).resolve().parents[1]
parser=argparse.ArgumentParser(description='Exercise actual FS size/open/read and asset readers with a simulated sector boundary.')
parser.add_argument('--source',type=Path,default=root)
parser.add_argument('--assets',type=Path,default=root/'cd')
parser.add_argument('--output',type=Path,default=root/'build/raw-disc-metadata')
parser.add_argument('--baseline',action='store_true',help='Expect the pre-API source to fail the zero-read contract')
parser.add_argument('--mutant',choices=['size-plus-one','all-raw-names'])
args=parser.parse_args()
assert not(args.baseline and args.mutant)
src=args.source.resolve()
out=args.output.resolve();out.mkdir(parents=True,exist_ok=True)
fs=(src/'sdk/jo_engine/fs.c').read_text();assets=(src/'port/wf_jo_assets.c').read_text()
types=(src/'sdk/jo_engine/jo/types.h').read_text();tools=(src/'sdk/jo_engine/jo/tools.h').read_text()
def function(text,name):
    m=re.search(r'^(?:static\s+)?(?:__jo_force_inline\s+)?(?:unsigned\s+int|int|bool|void)\s+(?:\*\s*)?'+re.escape(name)+r'\([^;{]*?\)\s*\{',text,re.M)
    assert m,name
    depth=1;end=m.end()
    while depth:
        depth+=(text[end]=='{')-(text[end]=='}');end+=1
    return text[m.start():end]
names=['__jo_fs_get_file_id','__jo_fs_open','__jo_fs_close','__jo_fs_get_file_size',
       '__jo_fs_seek_sector','__jo_fs_read_sectors','jo_fs_read_raw','jo_fs_lz_next_raw_byte',
       'jo_fs_lz_read','jo_fs_open_id','jo_fs_open','jo_fs_fill_read_buffer','jo_fs_seek_forward',
       'jo_fs_read_next_bytes','jo_fs_close','jo_fs_set_read_sector_count']
if not args.baseline:names.append('jo_fs_get_raw_file_size')
functions=[function(fs,n)for n in names]+[function(tools,'jo_lz_size'),function(assets,'valid_range'),function(assets,'disc_size'),function(assets,'disc_read')]
if args.mutant=='size-plus-one':
    target='return ((int)__jo_fs_dir[id].size);';assert sum(f.count(target)for f in functions)==1
    functions=[f.replace(target,'return ((int)__jo_fs_dir[id].size + 1);')for f in functions]
if args.mutant=='all-raw-names':
    target='if(!strcmp(name,"ROM68K.BIN")||!strcmp(name,"SPRITES.WFP")||\n       !strcmp(name,"SCENES.WFP")||!strcmp(name,"SFX.DAT"))'
    assert sum(f.count(target)for f in functions)==1
    functions=[f.replace(target,'if(1)')for f in functions]
definitions=[]
for text,endname in [(types,'jo_file'),(fs,'jo_fs_dir_entry'),(fs,'jo_fs_handle'),(fs,'jo_fs_lz_state')]:
    m=re.search(r'typedef struct\s*\{[^}]*\}\s*'+endname+r';',text);assert m,endname;definitions.append(m[0])
program=(root/'tests/raw_disc_metadata.c.in').read_text().replace('@TYPES@','\n'.join(definitions))
constants=[]
for name in ['JO_READ_RETRY_COUNT','JO_READ_SPIN_WAIT']:
    m=re.search(r'^#\s*define\s+'+name+r'\s+[^\n]+',fs,re.M);assert m,name;constants.append(m[0])
program=program.replace('@READ_CONSTANTS@','\n'.join(constants))
program=program.replace('@PROTOTYPES@','\n'.join(f[:f.index('{')].strip()+';'for f in functions))
program=program.replace('@FUNCTIONS@','\n\n'.join(functions))
c=out/'actual_sources.c';c.write_text(program)
sha=lambda p:hashlib.sha256(p.read_bytes()).hexdigest()
report={'status':'RUNNING','baseline':args.baseline,'source_sha256':{str(p):sha(p)for p in [src/'sdk/jo_engine/fs.c',src/'port/wf_jo_assets.c',src/'sdk/jo_engine/jo/fs.h',src/'sdk/jo_engine/jo/types.h',src/'sdk/jo_engine/jo/tools.h',Path(__file__),root/'tests/raw_disc_metadata.c.in']},'runs':[]}
for raw in ([1]if args.mutant else[0,1]):
    binary=out/('metadata-'+str(raw));cmd=['cc','-std=c11','-O1','-g','-Wall','-Wextra','-Werror','-Wno-unused-function',
      '-fsanitize=address,undefined','-fno-omit-frame-pointer','-fno-pie','-no-pie','-I'+str(src/'port'),'-I'+str(src/'sdk/jo_engine'),
      '-DWF_JO_RAW_DISC_ASSETS='+str(raw),*(['-DBASELINE']if args.baseline else[]),str(c),'-o',str(binary)]
    p=subprocess.run(cmd,text=True,capture_output=True);(out/f'compile-{raw}.log').write_text(p.stdout+p.stderr)
    if p.returncode:print(p.stderr);p.check_returncode()
    p=subprocess.run([str(binary),str(args.assets.resolve())],text=True,capture_output=True,
        env=dict(os.environ,ASAN_OPTIONS='detect_leaks=0:abort_on_error=1',UBSAN_OPTIONS='halt_on_error=1'))
    (out/f'run-{raw}.log').write_text(p.stdout+p.stderr)
    expected=not(args.baseline and raw==1)and not args.mutant;assert(p.returncode==0)==expected,(raw,p.returncode,p.stdout,p.stderr)
    report['runs'].append({'raw':raw,'returncode':p.returncode,'expected_success':expected,'command':cmd,'binary_sha256':sha(binary),'output':p.stdout+p.stderr});print(raw,p.returncode,p.stdout.strip(),flush=True)
report['status']='EXPECTED_RED'if args.baseline or args.mutant else'PASS';report['mutant']=args.mutant;report['generated_source_sha256']=sha(c);report['source_drift']=[n for n,h in report['source_sha256'].items()if sha(Path(n))!=h];assert not report['source_drift']
(out/'report.json').write_text(json.dumps(report,indent=2)+'\n')
