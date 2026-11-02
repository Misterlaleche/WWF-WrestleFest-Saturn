"""Close read-only diagnostics; this native prototype is not delivery."""
from pathlib import Path
import hashlib,importlib.util,json,struct,sys
here=Path(__file__).resolve().parent;run=here/(sys.argv[1] if len(sys.argv)>1 else 'cold-9600-tag')
spec=importlib.util.spec_from_file_location('cold',here.parent.parent/'emulator/cold_overlay.py')
c=importlib.util.module_from_spec(spec);spec.loader.exec_module(c)
contract=json.loads((here/'RUNTIME-CONTRACT.json').read_text())
master=c.Elf(here/contract['master_file']);module=c.Elf(here/'music-hot/MUSIC.DEBUG.ELF');transport=c.Elf(here/'music-hot/MUSIC.ELF')
sha=lambda data:hashlib.sha256(data).hexdigest()
assert sha(master.data)==contract['exact_master_sha256']
report=json.loads((run/'report.json').read_text())
assert report['elf']['sha256']==sha(master.data)
assert report['manifest']['MUSIC.ELF']['sha256']==sha(transport.data)
assert report['existing_media_unchanged'] and not report['guest_memory_edits'] and not report['state_loads']
start=master.symbols['___wf_music_hot_start'][0];end=master.symbols['___wf_music_hot_end'][0]
private=master.symbols['___wf_sdk_private_pool_start'][0]
bottom=master.symbols['___wf_slave_stack_bottom'][0];top=master.symbols['___slave_stack'][0]
assert start-private==14336 and end-start==18432 and bottom==end and top-bottom==8192 and top-private==40960
code_start=module.symbols['_wf_hotcode_start'][0];code_end=module.symbols['_wf_hotcode_end'][0]
assert code_start==start and code_end+3220<=end
segment=next(p for p in module.ph if p[0]==1 and p[2]<=start and code_end<=p[2]+p[4])
expected_code=module.data[segment[1]+start-segment[2]:segment[1]+code_end-segment[2]]
program=next(p for p in transport.ph if p[0]==1 and p[2]==0x05c5c000)
expected_program=transport.data[program[1]:program[1]+program[4]]
checks=[]
for path in sorted(run.glob('frame-*.state'),key=lambda p:int(p.stem.split('-')[1])):
    frame=int(path.stem.split('-')[1]);state=path.read_bytes()
    work,_=c.field(state,'WorkRAMH',1048576);vram,_=c.field(state,'VRAM',524288)
    def read(address,count):
        if 0x06000000<=address and address+count<=0x06100000:base=0x06000000;off=work
        elif 0x05c00000<=address and address+count<=0x05c80000:base=0x05c00000;off=vram
        else:raise ValueError(hex(address))
        return bytes(state[off+((address-base+i)^1)] for i in range(count))
    def u32(address):return int.from_bytes(read(address,4),'big')
    assert read(start,len(expected_code))==expected_code
    assert read(0x05c5c000,len(expected_program))==expected_program
    assert u32(0x05c7f000+172)==start
    hot=u32(module.symbols['_hot'][0]);assert hot==code_end and u32(hot+16)==hot
    for offset,name in ((0,'_read_byte'),(4,'_write_byte'),(8,'_port_in'),(12,'_port_out'),(2148,'_event')):
        assert u32(hot+offset)==module.symbols[name][0]
    synth=hot+2164;assert u32(synth+1036)==module.symbols['_synth_write'][0]
    assert not read(synth+1052,1)[0]
    stats=[u32(master.symbols['_wf_saturn_allocator_live_stats'][0]+i*4) for i in range(16)]
    partition=[u32(master.symbols['_wf_saturn_allocator_partition'][0]+i*4) for i in range(12)]
    assert partition[0]==14336 and stats[1]==stats[14]==stats[15]==0
    def walk(size_address,exclusive_end):
        blocks=[];previous_free=False;previous_header=None
        while True:
            raw=u32(size_address);size=raw&~3;free=bool(raw&1)
            assert bool(raw&2)==previous_free
            if raw&2:assert u32(size_address-4)==previous_header
            if size==0:assert size_address==exclusive_end-4;break
            assert size%4==0 and size>=4 and size_address+4+size<=exclusive_end-4
            blocks.append(dict(address=hex(size_address+4),bytes=size,free=free))
            previous_free=free;previous_header=size_address-4;size_address+=size+4
            assert len(blocks)<2048
        return dict(used=sum(b['bytes']for b in blocks if not b['free']),free=sum(b['bytes']for b in blocks if b['free']),
            largest=max((b['bytes']for b in blocks if b['free']),default=0),blocks=blocks)
    live_private=walk(private+partition[4],start);live_user=walk(partition[11],0x06100000)
    assert live_private['free']>=4096 and live_private['largest']>=4096 and live_user['free']>=16384
    if frame==1800:
        assert (live_private['used'],live_private['free'],live_private['largest'])==(stats[2],stats[3],stats[4])
        assert (live_user['used'],live_user['free'],live_user['largest'])==(stats[7],stats[8],stats[9])
    cpus={}
    for cpu in ('SH2-M','SH2-S'):
        off=state.index(cpu.encode().ljust(32,b'\0'));size=int.from_bytes(state[off+32:off+36],'little');block=state[off+36:off+36+size]
        p,_=c.field(block,'PC',4);r,_=c.field(block,'R',64);ccr,_=c.field(block,'CCR',1)
        pc=int.from_bytes(block[p:p+4],'little');sp=int.from_bytes(block[r+60:r+64],'little')
        cpus[cpu]=dict(pc=hex(pc),sp=hex(sp),ccr=block[ccr])
        if cpu=='SH2-S':assert bottom<=sp<=top and not(pc&0x20000000) and block[ccr]==1
    row=next(row for row in report['observations'] if row['frame']==frame)
    checks.append(dict(frame=frame,game_frame=row.get('game_frame'),hot_window_bytes=end-start,
        whole_window_sha256=sha(read(start,end-start)),resident_code_bytes=len(expected_code),resident_code_matches=True,
        vdp_transport_bytes=len(expected_program),vdp_transport_matches=True,userdata_and_callbacks_valid=True,
        unused_tail_zero=not any(read(code_end+3220,end-code_end-3220)),
        live_private=live_private,live_user=live_user,boot_stats_captures=stats[0],cpu=cpus,
        synth_writes=u32(synth+1044),synth_key=list(read(synth+256,8)),fm_active_mask=row.get('fm_active_mask'),audio_rms=row.get('audio_rms')))
combat=[row for row in report['observations'] if row.get('game_frame',0)>0]
active=[row for row in combat if row.get('fm_active_mask',0) and row.get('audio_rms',0)>0 and row.get('music_state')==2 and not row.get('music_error')]
summary=dict(label='INTEGRATION AUDIO DIAGNOSTIC - '+('OBSERVED GATE PASSED'if report['audio_gate']['software_audio_gate_passed']else'OBSERVED GATE FAILED'),delivery_authorized=False,
    master_sha256=sha(master.data),music_sha256=sha(transport.data),run=run.name,checks=checks,
    game_frame_max=max((row.get('game_frame',0)for row in report['observations']),default=0),combat_rows=len(combat),
    active_music_combat_rows=len(active),combat_active_examples=[{k:row.get(k) for k in ('frame','game_frame','fm_active_mask','audio_rms','music_events','dropped_fm')}for row in active[-5:]],
    audio_gate=report['audio_gate'],wav=report.get('wav_capture'),media_created=False,
    disposition='Full catalogue and performance remain required; see audio_gate. A failed cold4200 prohibits9600 continuation and any image. Await cycle profile if failed.')
(run/'MEMORY-AND-COMBAT-READBACK.json').write_text(json.dumps(summary,indent=2)+'\n')
print(json.dumps({k:summary[k]for k in ('run','game_frame_max','combat_rows','active_music_combat_rows','master_sha256','music_sha256')},indent=2))
