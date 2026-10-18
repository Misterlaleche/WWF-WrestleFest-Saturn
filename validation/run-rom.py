#!/usr/bin/env python3
"""Rebuild native probes and run independent ROM comparisons plus byte audits."""
import argparse,pathlib,subprocess,sys,re,json
p=argparse.ArgumentParser();p.add_argument('rom',type=pathlib.Path);p.add_argument('arcade_zip',type=pathlib.Path);p.add_argument('--cc',default='clang');p.add_argument('--out',type=pathlib.Path,default=pathlib.Path('validation/results/rom'));a=p.parse_args()
root=pathlib.Path(__file__).resolve().parents[1];r=root/'src/runtime';out=a.out.resolve();out.mkdir(parents=True,exist_ok=True);rom=a.rom.resolve();z=a.arcade_zip.resolve();results=[]
def run(name,cmd):
 q=subprocess.run([str(x) for x in cmd],cwd=r,stdout=subprocess.PIPE,stderr=subprocess.STDOUT,text=True);(out/(name+'.log')).write_text('COMMAND '+repr([str(x) for x in cmd])+'\n'+q.stdout+'\nEXIT '+str(q.returncode)+'\n');results.append({'test':name,'exit':q.returncode});assert q.returncode==0,name
core=re.search(r'^CORE=(.*)$',(r/'Makefile').read_text(),re.M).group(1).split();objs=[]
for source in core:
 obj=out/(source.replace('/','_')+'.o');run('compile-'+source.replace('/','_'),[a.cc,'-std=c11','-O2','-Wall','-Wextra','-Werror','-Iinclude','-c',source,'-o',obj]);objs.append(obj)
probes=[('tag','tests/r17/tag_oracle_driver.c','verify_tag_rom.py'),('royal','tests/royal_rom_probe.c','verify_royal_rom.py'),('active','tests/test_runtime17_active_rom.c','verify_active_rom.py'),('frame','tests/r17/active_frame_driver.c','verify_runtime17_frame_rom.py'),('render','tests/r17/render_shadow_oracle_driver.c','verify_render_shadow_rom.py')]
for name,source,script in probes:
 binary=out/('probe-'+name);run('link-'+name,[a.cc,'-std=c11','-O2','-Wall','-Wextra','-Werror','-Iinclude',*objs,source,'-o',binary]);run('oracle-'+name,[sys.executable,r/'tools'/script,rom,binary])
binary=out/'probe-register-edges'
run('link-register-edges',[a.cc,'-std=c11','-O2','-Wall','-Wextra','-Werror','-Iinclude',*[x for x in objs if x.name!='src_wf_r17_referee.c.o'],'tests/r17/register_edge_driver.c','-o',binary])
run('oracle-register-edges',[sys.executable,r/'tools/verify_register_edges_rom.py',rom,binary])
zip_scripts={'audit_runtime17_10e6a_7a00.py','audit_runtime17_8ecc_rom.py','audit_runtime17_9052_rom.py','audit_runtime17_899c_rom.py','audit_runtime17_262d2_rom.py'}
for script in sorted((r/'tools').glob('audit*rom.py')):
 run(script.stem,[sys.executable,script,z if script.name in zip_scripts else rom])
script=r/'tools/audit_runtime17_10e6a_7a00.py';run(script.stem,[sys.executable,script,z])
run('generated-native-source',[sys.executable,r/'tools/generate_participant_native.py',rom])
(out/'results.json').write_text(json.dumps(results,indent=2)+'\n');print('PASS independent ROM comparisons, byte audits and generated-source audit')
