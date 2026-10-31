#!/usr/bin/env python3
"""Build and replay the native session. Controller schedules remain test-only."""
import argparse, hashlib, json, os, subprocess, sys, tempfile, zipfile
from pathlib import Path

def main():
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('--rom',type=Path,default=Path(os.environ['WF_ROM']) if os.environ.get('WF_ROM') else None)
    p.add_argument('--out',type=Path)
    p.add_argument('--sanitize',action='store_true')
    p.add_argument('--board',action='store_true',help='Use production ordered board phases and ACK latches')
    args=p.parse_args()
    root=Path(__file__).resolve().parents[1]
    (root/'build').mkdir(exist_ok=True)
    out=args.out.resolve() if args.out else Path(tempfile.mkdtemp(prefix='session-',dir=root/'build'))/'checks'
    out.mkdir(parents=True,exist_ok=False)
    env=dict(os.environ,UBSAN_OPTIONS='halt_on_error=1')
    def build(name):
        dest=out/name
        cmd=[sys.executable,str(root/'tools/build_session.py'),'--out',str(dest),
             '--probe',str(root/'tests'/(name+'.c'))]
        if args.sanitize:cmd+=['--sanitize']
        with (out/(name+'-build.log')).open('w') as log:
            subprocess.run(cmd,stdout=log,stderr=log,check=True,env=env)
        return dest/name
    unit=build('test_session')
    result=subprocess.check_output([str(unit)],text=True,env=env)
    (out/'unit.log').write_text(result);print(result,flush=True)
    extensions=build('session_extensions_probe')
    result=subprocess.check_output([str(extensions)],text=True,env=env)
    (out/'extensions.log').write_text(result);print(result,flush=True)
    records=[]
    if args.rom:
        with zipfile.ZipFile(args.rom) as z:
            rom=bytearray(0x80000);rom[::2]=z.read('31e14-0.ic18');rom[1::2]=z.read('31e13-0.ic19')
        assert hashlib.sha1(rom).hexdigest()=='a726c46f350723ea73158bc78055d17d418b14e2'
        # External evidence directory only; the ROM is never packaged in Git.
        image=out/'main.bin';image.write_bytes(rom);probe=build('session_board_probe' if args.board else 'session_probe')
        cases=[('tag',[0,0,1,0,1085,1,0]),('tag-p2',[0,0,1,1,1085,1,0]),
               ('demolition',[0,8,9,0,1085,1,0]),('demolition-p2',[0,8,9,1,1085,1,0]),
               ('royal-hogan',[1,0,0,0,1085,1,0]),
               ('royal-crush',[1,9,0,0,1085,1,0]),('attract-start',[1,0,0,0,1085,1,1]),
               ('tag-other-seed',[0,0,1,0,305419896,1,0]),
               ('royal-slow-events',[1,9,0,0,305419896,3,0]),
               ('match-resource-error',[0,0,1,0,1085,1,0])]
        if args.board:cases=[c for c in cases if c[0]!='royal-slow-events']
        for name,inputs in cases:
            dest=out/name;dest.mkdir()
            case_env=dict(env)
            if name=='match-resource-error':case_env['WF_SESSION_REJECT_MATCH_WORLD']='1'
            cmd=[str(probe),str(image),str(dest),*map(str,inputs)]
            with (dest/'run.log').open('w') as log:
                subprocess.run(cmd,stdout=log,stderr=log,check=True,env=case_env)
            log=(dest/'run.log').read_text();assert log.startswith('PASS'),log
            records.append(dict(name=name,inputs=inputs,result=log.strip(),
                           files={f.name:hashlib.sha256(f.read_bytes()).hexdigest() for f in sorted(dest.iterdir())}))
            print(log.strip(),flush=True)
    else:print('SKIP full session replay: supply --rom explicitly',flush=True)
    report=dict(status='PASS',board_schedule=args.board,sanitized=args.sanitize,full_replay=bool(args.rom),cases=records,
                scope='Native state/owner integration with controlled external bus, not Saturn rendering or timing')
    (out/'report.json').write_text(json.dumps(report,indent=2)+'\n')
    print('Evidence:',out,flush=True)
if __name__=='__main__':main()
