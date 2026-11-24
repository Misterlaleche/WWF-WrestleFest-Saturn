#!/usr/bin/env python3
"""Actual VDP2 edge timestamps versus COMREG and first status CONT writes."""
import argparse,bisect,csv,gzip,hashlib,json
from pathlib import Path
from analyze_timing import analyze

def analyze_cont(path,capture=None):
 rows=list(csv.DictReader((gzip.open(path,'rt') if str(path).endswith('.gz') else Path(path).open())))
 boot_count=0;start_receipt=None
 if capture:
  report=json.loads(Path(capture).read_text());symbols=report['symbols']
  address=symbols['jo_smpc_command']
  limit=min(v for v in symbols.values() if isinstance(v,int) and v>address)
  starts=[i for i,r in enumerate(rows) if r['kind']=='C' and int(r['value'])==3 and
          address<=int(r['master_pc'],16)<limit]
  assert starts,'No SDK SSHOFF handoff matched the exact ELF symbols'
  start=starts[0];boot_count=sum(r['kind']=='C' for r in rows[:start])
  start_receipt={'row':rows[start],'elf_sha256':report['elf_sha256'],
                 'symbol_range':[hex(address),hex(limit)],
                 'reason':'first SDK SSHOFF after BIOS boot, observed PC matched exact ELF function'}
  rows=rows[start:]
 outs=sorted(int(r['absolute_master']) for r in rows if r['kind']=='O')
 end=max(int(r['absolute_master']) for r in rows)
 detail=[];failures=[];counts={'peripheral_intback':0,'status_only_intback':0,'first_cont':0,'tail_unmeasured':0}
 for i,r in enumerate(rows):
  if r['kind']!='C' or int(r['value'])!=16:continue
  if not int(r['ireg1'])&8:counts['status_only_intback']+=1;continue
  counts['peripheral_intback']+=1
  vb=int(r['last_vb_master']);now=int(r['absolute_master']);hz=int(r['master_hz'])
  oidx=bisect.bisect_right(outs,vb)
  if vb<0 or oidx==len(outs):counts['tail_unmeasured']+=1;continue
  out=outs[oidx]
  cont=None
  for nxt in rows[i+1:]:
   if nxt['kind'] in ('C','B'):break
   if nxt['kind']=='I' and ((int(nxt['value'])^int(nxt['ireg0']))&128):
    cont=int(nxt['absolute_master']);break
  sample={'command_us_after_vb':(now-vb)*1e6/hz,'command_us_before_out':(out-now)*1e6/hz,
          'command_pc':r['master_pc'],'first_cont_us_before_out':None if cont is None else (out-cont)*1e6/hz}
  if (now-vb)*1000000 < 300*hz:failures.append({'reason':'command_inside_300us','row':r})
  if now>=out:failures.append({'reason':'command_after_out','row':r})
  if cont is None:
   if end>=out:failures.append({'reason':'no_first_cont_before_out','row':r})
   else:counts['tail_unmeasured']+=1
  else:
   counts['first_cont']+=1
   if cont>=out:failures.append({'reason':'first_cont_after_out','row':r})
  detail.append(sample)
 bounds={key:{'min':min(values),'max':max(values)} for key in
         ['command_us_after_vb','command_us_before_out','first_cont_us_before_out']
         if (values:=[r[key] for r in detail if r[key] is not None])}
 return {'basic_all_commands_including_bios':analyze(path),'counts':counts,'bounds':bounds,'failures':failures,
         'sdk_handoff':start_receipt,'bios_commands_before_sdk':boot_count,
         'csv_sha256':hashlib.sha256(gzip.decompress(Path(path).read_bytes()) if str(path).endswith('.gz') else Path(path).read_bytes()).hexdigest(),
         'hardware_verified':False,'clock':'actual emulated VDP2 edges, not guest deadlines'}

if __name__=='__main__':
 p=argparse.ArgumentParser();p.add_argument('csv',type=Path);p.add_argument('--output',type=Path);p.add_argument('--capture',type=Path)
 a=p.parse_args();result=analyze_cont(a.csv,a.capture);text=json.dumps(result,indent=2)+'\n'
 if a.output:a.output.write_text(text)
 print(text,end='')
