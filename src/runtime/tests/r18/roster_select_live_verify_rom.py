#!/usr/bin/env python3
"""Resumable original live $5990..$5EA8 against intact ROM, including joins."""
import argparse,ctypes,hashlib,itertools,json,os,random,subprocess,tempfile,zipfile
from pathlib import Path
import machine68k as mus
from roster_select_verify_rom import ROOT,RAM,BASE,ACTOR,SHA1,put,get

CALLS={0x2aea,0x26a42,0x247c,0x27b8,0x2836,0x51d2,0x527e,0x21e6}
STORES={0x5eea,0x5ef2}
def main():
    ap=argparse.ArgumentParser(description=__doc__);ap.add_argument('--smoke',action='store_true');ap.add_argument('--kind');ap.add_argument('--report',type=Path);v=ap.parse_args()
    p=ROOT/'audit/runtime18-cd/ROM68K.BIN'
    if p.exists():rom=p.read_bytes()
    else:
        with zipfile.ZipFile(ROOT.parents[2]/'local-rom/wwfwfest.zip') as z:e=z.read('31e14-0.ic18');o=z.read('31e13-0.ic19')
        r=bytearray(len(e)*2);r[0::2]=e;r[1::2]=o;rom=bytes(r)
    assert hashlib.sha1(rom).hexdigest()==SHA1
    rng=random.Random(0x599060a2);coverage=set();case_count=step_count=effects=waits=0;by_kind={}
    def seed(mode=1,page=0):
        m=bytearray(0x200000);m[:len(rom)]=rom;m[RAM:RAM+0x2000]=rng.randbytes(0x2000)
        m[RAM+0x15e]=page*2;m[RAM+0x160]=0;m[RAM+0x161]=mode
        put(m,RAM+0x66,0);put(m,RAM+0x15c,0);put(m,RAM+0x1698,0)
        put(m,RAM+0x17e6,0x210 if page else 0x140)
        for j in range(0x50):m[RAM+0x11f4+j]=0
        for i in range(4):
            a=RAM+BASE+i*ACTOR;put(m,a,0x8000);put(m,a+0x22,i);m[a+0x25]=i;m[a+0x33]=0;m[a+0xa1]=1
            for off in (0x56,0x58,0x5c,0x5e,0x90,0xa2,0xa6,0xa8):put(m,a+off,0)
            put(m,a+0x86,RAM+BASE+(i^1)*ACTOR,4);put(m,a+0x8a,0x140020+i*2,4)
            put(m,RAM+0x11f4+i*2,1)
        return m,[rng.getrandbits(32) for _ in range(8)],[0,*[rng.randrange(0x200000) for _ in range(7)]]
    def fixtures():
        # Navigation, all human action/held combinations, and TAG partner paths.
        for mode,page,slot,actor,action,partner in itertools.product((0,1),(0,1),range(10),range(4),(0,1,2,3),range(3)):
            if v.smoke and (mode,page,slot,actor,action,partner)!=(0,0,0,0,0,0):continue
            m,d,c=seed(mode,page);a=RAM+BASE+actor*ACTOR;b=RAM+BASE+(actor^1)*ACTOR
            put(m,a+0x22,slot);m[a+0x25]=slot;put(m,b,0x8000 if partner==0 else 0);m[b+0xa1]=partner==2
            if slot==8:m[b+0x33]=partner==2
            ports=[0xffff]*4;ports[actor]&=~(action<<4);ports[actor]&=~(1<<(actor%4))
            c[0]=0x59d2;c[1]=a;d[6]=3-actor
            yield 'actor',m,d,c,ports,1
        for flag,timer,start in itertools.product((0,8,10),(0,1022,1023,1024),(0x140,0x148,0x208,0x210)):
            m,d,c=seed();m[RAM+0x15e]=flag;put(m,RAM+0x15c,timer);put(m,RAM+0x17e6,start);c[0]=0x5996
            yield 'frame',m,d,c,[0xffff]*4,1
        for config,mode,flags,credit,slot,assignment,timer,inp in itertools.product((0,0x800),(0,1),(0,0x10,0x20,0x60),(0,1),range(4),(0,0x40,0x80),(0,511),(0xffff,0xff7f,0xffef,0xfffe,0xfffd,0xfffb,0xfff7)):
            # Only $0800 has assignment branches; keep generic sweep bounded.
            if not config and (flags or assignment or timer or inp not in (0xffff,0xff7f)):continue
            m,d,c=seed(mode);put(m,RAM+0x66,config);m[RAM+0x15e]=flags;m[RAM+0x160]=assignment
            m[RAM+0x4f]=credit;put(m,RAM+0x1698,timer);put(m,RAM+0x156,0x140022,4)
            for j in range(4):m[RAM+BASE+j*ACTOR+0xa1]=0
            ports=[0xffff]*4;ports[slot]=inp;c[0]=0x5d28
            yield 'join',m,d,c,ports,1
        for mode,page,slot,partner_slot,previous,full in itertools.product((0,1),(0,1),range(4),(7,8),(7,8),(False,True)):
            m,d,c=seed(mode,page);a=RAM+BASE+slot*ACTOR;p=RAM+BASE+(slot^1)*ACTOR
            put(m,a,0);m[a+0xa1]=0;m[p+0x33]=1;put(m,p+0x22,partner_slot);m[p+0x25]=previous;m[RAM+0x4f]=1
            if full:
                for j in range(10):put(m,RAM+0x11f4+j*2,1)
            ports=[0xffff]*4;ports[slot]=0xff7f;c[0]=0x5d28
            yield 'late-join',m,d,c,ports,1
        for active,confirmed,actor in itertools.product((False,True),(False,True),range(4)):
            m,d,c=seed();a=RAM+BASE+actor*ACTOR;m[a]=128 if active else 0;m[a+0x33]=int(confirmed)
            c[0]=0x59d2;c[1]=a;d[6]=3-actor
            yield 'actor-skip',m,d,c,[0xffff]*4,1
        for mode,actor,confirmed,timer,phase in itertools.product((0,1),range(4),(0,1),(0,73,74,75,0xffff),(0,1,2,3,4)):
            m,d,c=seed(mode);a=RAM+BASE+actor*ACTOR;m[a+0x33]=confirmed;put(m,a+0x58,timer);put(m,a+0x5c,phase)
            c[0]=0x5d52;c[1]=a;d[4]=3-actor;d[5]=0
            yield 'post',m,d,c,[0xffff]*4,1
        for mode,active,humans in itertools.product((0,1),range(1,16),range(16)):
            m,d,c=seed(mode)
            for j in range(4):m[RAM+BASE+j*ACTOR]=128 if active&(1<<j) else 0;m[RAM+BASE+j*ACTOR+0xa1]=humans>>j&1
            c[0]=0x5da0
            yield 'exit',m,d,c,[0xffff]*4,1
        for ack in (0,2):
            m,d,c=seed();m[RAM+0x6f]=ack;c[0]=0x5d1e
            yield 'handshake',m,d,c,[0xffff]*4,1
        # Whole-frame trajectories prove timeout 1024, automatic64, post76.
        for mode,automatic in itertools.product((0,1),(False,True)):
            m,d,c=seed(mode);c[0]=0x5990
            if automatic:put(m,RAM+0x15c,1023);c[0]=0x5996
            yield 'trajectory',m,d,c,[0xffef]*4,20000
    with tempfile.TemporaryDirectory(prefix='wf-roster-live-',ignore_cleanup_errors=os.name=='nt') as tmp:
        lib=Path(tmp)/'roster.so';sources=['src/wf_r18_roster_select.c','src/wf_r18_frontend_input.c','src/wf_r17_m68k_shadow.c','tests/r18/roster_select_oracle_bridge.c']
        subprocess.run(['cc','-shared','-fPIC','-std=c11','-O2','-Wall','-Wextra','-Werror','-Iinclude','-DWF_R18_ROSTER_IMPLEMENTED','-DWF_R18_ROSTER_INIT_IMPLEMENTED',*sources,'-o',str(lib)],cwd=ROOT,check=True)
        native=ctypes.CDLL(str(lib));native.roster_select_init_oracle.argtypes=[ctypes.c_void_p,ctypes.POINTER(ctypes.c_uint32),ctypes.POINTER(ctypes.c_uint32),ctypes.POINTER(ctypes.c_uint16),ctypes.POINTER(ctypes.c_uint32)]
        machine=mus.Machine(mus.CPUType.M68000,2048);cpu,mem=machine.cpu,machine.mem
        mem.w_block(0,rom)
        for kind,m,ds,cursor,inputs,limit in fixtures():
            if v.kind and kind!=v.kind:continue
            case_count+=1;by_kind[kind]=by_kind.get(kind,0)+1
            b=ctypes.create_string_buffer(bytes(m),len(m));address=ctypes.addressof(b)
            automatic_calls=[0]*4;confirm_times={};seen_auto=False
            for tick in range(limit):
                # VBlank acknowledges frame handshake. All physical input is held.
                if cursor[0]==0x5d1e and kind=='trajectory':m[RAM+0x6f]|=2
                if cursor[0]==0x5d1e and kind=='trajectory':b[RAM+0x6f]=bytes([m[RAM+0x6f]])
                step_count+=1;old_cursor=list(cursor)
                old_flags=m[RAM+0x15e];old_timer=get(m,RAM+0x15c)
                old_actor=cursor[1]-RAM if cursor[0] in (0x59d2,0x5d52) else None
                old_post=get(m,RAM+old_actor+0x58) if old_actor is not None else None
                old_slot=m[RAM+old_actor+0x23] if old_actor is not None else None
                was_confirmed=bool(m[RAM+old_actor+0x33]&1) if old_actor is not None else False
                if kind=='trajectory' and cursor[0]==0x59d2 and old_flags&16 and not old_flags&8 and m[RAM+old_actor]&128 and not was_confirmed:
                    automatic_calls[(old_actor-BASE)//ACTOR]+=1
                d=(ctypes.c_uint32*8)(*ds);cur=(ctypes.c_uint32*8)(*cursor);ports=(ctypes.c_uint16*4)(*inputs);events=(ctypes.c_uint32*321)()
                st=native.roster_select_init_oracle(b,d,cur,ports,events)
                assert st>=0,('live missing/blocked',kind,old_cursor,st,hex(events[320]))
                mem.w_block(RAM,bytes(m[RAM:RAM+0x2000]));mem.w_block(0x188000,bytes(m[0x188000:0x18a000]))
                for i,x in enumerate(ds):cpu.w_reg(i,x)
                for i,x in enumerate(old_cursor[1:]):cpu.w_reg(i+8,x)
                cpu.w_sp(0x1d0000);mem.w32(0x1d0000,0x70000);cpu.w_pc(old_cursor[0])
                for i,x in enumerate(inputs):mem.w16(0x140020+i*2,x)
                calls=[];target=(cur[0],cur[1])
                for instruction in range(10000):
                    pc=cpu.r_pc()
                    if (pc,cpu.r_reg(8))==target and (instruction or st==1):break
                    coverage.add(pc)
                    if pc in CALLS:
                        calls.extend([pc,cpu.r_reg(8),*[cpu.r_reg(i) for i in range(8)]]);cpu.w_pc(mem.r32(cpu.r_sp()));cpu.w_sp(cpu.r_sp()+4);continue
                    if pc in STORES:
                        at=cpu.r_reg(9)+(cpu.r_reg(3)&0xffff)+0x1e
                        value=mem.r16(cpu.r_reg(8)+0xae) if pc==0x5eea else mem.r16(cpu.r_reg(10)+(cpu.r_reg(2)&0xffff))
                        calls.extend([pc,at,value,*([0]*7)])
                    cpu.execute(1)
                assert (cpu.r_pc(),cpu.r_reg(8))==target,('runaway',kind,old_cursor,list(cur),hex(cpu.r_pc()))
                assert list(events)[1:1+events[0]*10]==calls,('effects',kind,old_cursor,list(events)[:1+events[0]*10],calls)
                for at,size in ((RAM,0x2000),(0x188000,0x2000)):
                    got=ctypes.string_at(address+at,size);want=bytes(mem.r_block(at,size))
                    assert got==want,('RAM',kind,old_cursor,[(hex(at+i),x,y) for i,(x,y) in enumerate(zip(got,want)) if x!=y][:12])
                    m[at:at+size]=got
                assert list(d)==[cpu.r_reg(i) for i in range(8)],('D',kind,old_cursor,[hex(x) for x in d],[hex(cpu.r_reg(i)) for i in range(8)])
                assert list(cur)[1:]==[cpu.r_reg(i) for i in range(8,15)],('A',kind,old_cursor,[hex(x) for x in list(cur)[1:]],[hex(cpu.r_reg(i)) for i in range(8,15)])
                if kind=='frame':
                    assert get(m,RAM+0x15c)==(old_timer if old_flags&8 else (old_timer+1)&0xffff)
                    assert bool(m[RAM+0x15e]&16)==bool(not old_flags&8 and ((old_timer+1)&0xffff)>=1024)
                if kind=='post' and was_confirmed:
                    assert get(m,RAM+old_actor+0x58)==(old_post+1)&0xffff
                    assert bool(events[0])==(((old_post+1)&0xffff)<76)
                    if old_post==74:assert calls[0]==0x5eea,'75th post tick restores original actor palette'
                if kind=='trajectory':
                    seen_auto|=bool(m[RAM+0x15e]&16)
                    for j in range(4):
                        a=RAM+BASE+j*ACTOR
                        if m[a+0x33]&1 and j not in confirm_times:
                            confirm_times[j]=tick
                            if seen_auto and automatic_calls[j]<64:
                                assert not m[RAM+0x161]&1 and old_cursor[0]==0x59d2 and old_slot==8 and j==(((old_actor-BASE)//ACTOR)^1),('early automatic confirmation outside original TAG pair8',j,automatic_calls)
                    if cur[0]==0x5e82:
                        assert all(get(m,RAM+BASE+j*ACTOR+0x58)>=76 for j in range(4))
                effects+=events[0];waits+=st==1;ds=list(d);cursor=list(cur)
                if cursor[0]==0xac0:break
            if limit>1:assert cursor[0]==0xac0,('not complete',kind,cursor,hex(get(m,RAM+0x15c)),[(get(m,RAM+BASE+j*ACTOR+0x22),get(m,RAM+BASE+j*ACTOR+0x32),get(m,RAM+BASE+j*ACTOR+0x5e),get(m,RAM+BASE+j*ACTOR+0x58)) for j in range(4)])
            if v.smoke:break
        machine.cleanup()
    report={'rom_sha1':SHA1,'cases':case_count,'steps':step_count,'effects':effects,'waits':waits,'cases_by_kind':by_kind,'source_sha256':hashlib.sha256((ROOT/'src/wf_r18_roster_select.c').read_bytes()).hexdigest(),'compared':['RAM8KB','D0-D7','A0-A6','paletteRAM8KB','ordered external calls and palette writes'],'covered_rom_pcs':[hex(x) for x in sorted(coverage)],'boundaries':['original renderer/panel/palette setup and waits intercepted symmetrically','payment055A mode0, input64E0/7304, joins60A2 and activation5F28 run natively versus intact ROM','Stops BEFORE AC0; CPU roster generation belongs to subsequent combat owner']}
    if v.report:v.report.write_text(json.dumps(report,indent=2)+'\n')
    print('PASS live',case_count,'cases',step_count,'steps',effects,'effects',waits,'waits',by_kind)
if __name__=='__main__':main()
