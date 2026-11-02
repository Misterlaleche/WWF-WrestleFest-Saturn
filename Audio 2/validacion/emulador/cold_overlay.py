"""Cold boot Beetle with a read-only RAM sector overlay; never writes disc files.

All existing media are opened rb. Virtual cue text and changed sectors exist only
in Python memory, supplied through libretro VFS. No guest RAM/state writes.
"""
import argparse, array, bisect, ctypes as C, hashlib, importlib.util, io
import json, os, re, struct, time, zlib
from pathlib import Path

HERE = Path(__file__).resolve().parent
RUNTIME = HERE.parents[3]
ROOT = RUNTIME.parent
ENV = C.CFUNCTYPE(C.c_bool, C.c_uint, C.c_void_p)
VIDEO = C.CFUNCTYPE(None,C.c_void_p,C.c_uint,C.c_uint,C.c_size_t)
AUDIO = C.CFUNCTYPE(None,C.c_int16,C.c_int16)
BATCH = C.CFUNCTYPE(C.c_size_t,C.c_void_p,C.c_size_t)
POLL = C.CFUNCTYPE(None)
INPUT = C.CFUNCTYPE(C.c_int16,C.c_uint,C.c_uint,C.c_uint,C.c_uint)

class Variable(C.Structure): _fields_=[('key',C.c_char_p),('value',C.c_char_p)]
class Game(C.Structure): _fields_=[('path',C.c_char_p),('data',C.c_void_p),('size',C.c_size_t),('meta',C.c_char_p)]
class VFSInfo(C.Structure): _fields_=[('required_interface_version',C.c_uint32),('iface',C.c_void_p)]

def sha(data): return hashlib.sha256(data).hexdigest()
def file_sha(path):
    h=hashlib.sha256()
    with path.open('rb') as f:
        for b in iter(lambda:f.read(1024*1024),b''):h.update(b)
    return h.hexdigest()

class Elf:
    def __init__(self,path,data=None):
        self.path=path;self.data=path.read_bytes() if data is None else data;d=self.data
        assert d[:7]==b'\x7fELF\x01\x02\x01', 'ELF32 BE required'
        off=struct.unpack_from('>I',d,32)[0];ents,n=struct.unpack_from('>HH',d,46)
        self.headers=[struct.unpack_from('>10I',d,off+i*ents) for i in range(n)]
        self.symbols={}
        for h in self.headers:
            if h[1]!=2:continue
            st=self.headers[h[6]];strings=d[st[4]:st[4]+st[5]]
            for pos in range(h[4],h[4]+h[5],h[9]):
                no,addr,size=struct.unpack_from('>III',d,pos)
                if no:self.symbols[strings[no:strings.index(b'\0',no)].decode()]=(addr,size)
        po=struct.unpack_from('>I',d,28)[0];pe,pn=struct.unpack_from('>HH',d,42)
        self.ph=[struct.unpack_from('>8I',d,po+i*pe) for i in range(pn)]
    def binary(self):
        pieces=[]
        for h in self.headers:
            if h[1]==8 or not h[2]&2 or not h[5]:continue
            candidates=[p for p in self.ph if p[0]==1 and p[2]<=h[3] and h[3]+h[5]<=p[2]+p[4]]
            assert len(candidates)==1,(h,candidates)
            p=candidates[0];lma=p[3]+h[3]-p[2]
            pieces.append((lma,self.data[h[4]:h[4]+h[5]]))
        start=min(a for a,b in pieces);end=max(a+len(b) for a,b in pieces)
        assert start==0x06004000 and end-start<0x100000,(hex(start),hex(end))
        out=bytearray(end-start)
        for a,b in pieces:out[a-start:a-start+len(b)]=b
        return bytes(out)

class Overlay:
    def __init__(self,cue,replacements):
        self.cue=cue.resolve();text=cue.read_text()
        assert 'MODE1/2352' in text
        name=re.search(r'FILE\s+"([^"]+)"\s+BINARY',text).group(1)
        self.path=cue.parent/name;self.file=self.path.open('rb')
        self.size=self.path.stat().st_size;self.original_size=self.size;assert self.size%2352==0
        self.original_sha=file_sha(self.path);self.sectors={};self.files={}
        self.walk(self.payload(16)[156:190])
        spec=importlib.util.spec_from_file_location('sector_only',RUNTIME/'validation/iso_to_bin_exact.py')
        mod=importlib.util.module_from_spec(spec);spec.loader.exec_module(mod);self.sector=mod.sector
        self.replacements=[]
        for name,data in replacements.items():self.replace(name,data)
        self.verify()
        self.cue_bytes=b'FILE "disc.bin" BINARY\n  TRACK 01 MODE1/2352\n    INDEX 01 00:00:00\n'
    def raw(self,lba):
        if lba in self.sectors:return self.sectors[lba]
        if lba*2352>=self.original_size:return self.sector(bytes(2048),lba)
        self.file.seek(lba*2352);return self.file.read(2352)
    def payload(self,lba):return self.raw(lba)[16:2064]
    def walk(self,r,prefix=''):
        lba=struct.unpack_from('<I',r,2)[0];size=struct.unpack_from('<I',r,10)[0]
        data=b''.join(self.payload(lba+i) for i in range((size+2047)//2048));pos=0
        while pos<size:
            n=data[pos]
            if not n:pos=(pos//2048+1)*2048;continue
            rec=data[pos:pos+n];record_lba=lba+pos//2048;record_off=pos%2048;pos+=n
            name=rec[33:33+rec[32]]
            if name in (b'\0',b'\1'):continue
            name=name.decode().split(';')[0]
            if rec[25]&2:self.walk(rec,prefix+name+'/');continue
            self.files[prefix+name]=dict(lba=struct.unpack_from('<I',rec,2)[0],size=struct.unpack_from('<I',rec,10)[0],record_lba=record_lba,record_off=record_off)
    def manifest(self):return {name:dict(bytes=e['size'],sha256=sha(self.contents(name))) for name,e in self.files.items()}
    def contents(self,name):
        e=self.files[name];return b''.join(self.payload(e['lba']+i) for i in range((e['size']+2047)//2048))[:e['size']]
    def put(self,lba,payload):self.sectors[lba]=self.sector(bytes(payload),lba)
    def replace(self,name,data):
        assert name in self.files,name
        e=self.files[name];start=e['lba'];oldsize=e['size'];oldhash=sha(self.contents(name))
        # Reuse extents when possible; larger files move to appended virtual
        # sectors, present only in RAM. Directory and PVD reflect new extent.
        next_lba=min([v['lba'] for v in self.files.values() if v['lba']>start]+[self.size//2352])
        relocated=len(data)>(next_lba-start)*2048
        if relocated:
            start=self.size//2352;self.size+=(len(data)+2047)//2048*2352
            pvd=bytearray(self.payload(16));sectors=self.size//2352
            struct.pack_into('<I',pvd,80,sectors);struct.pack_into('>I',pvd,84,sectors);self.put(16,pvd)
        for i in range(((len(data) if relocated else max(oldsize,len(data)))+2047)//2048):
            payload=data[i*2048:(i+1)*2048].ljust(2048,b'\0');self.put(start+i,payload)
        r=bytearray(self.payload(e['record_lba']));pos=e['record_off']
        struct.pack_into('<I',r,pos+2,start);struct.pack_into('>I',r,pos+6,start)
        struct.pack_into('<I',r,pos+10,len(data));struct.pack_into('>I',r,pos+14,len(data));self.put(e['record_lba'],r)
        e['size']=len(data);e['lba']=start
        self.replacements.append(dict(name=name,bytes=len(data),old_sha256=oldhash,new_sha256=sha(data),lba=start,relocated_in_ram=relocated))
    def verify(self):
        for e in self.replacements:assert sha(self.contents(e['name']))==e['new_sha256']
        for lba,r in self.sectors.items():assert r==self.sector(r[16:2064],lba)
    def patch_boot(self,elf,binary):
        p=bytearray(self.payload(0));changes=[]
        for off,value,name in [(0xe4,elf.symbols['___master_stack'][0],'master_stack'),(0xec,elf.symbols['___slave_stack'][0],'slave_stack'),(0xf0,0x06004000,'boot_address'),(0xf4,max(0x20000,len(binary)),'boot_size')]:
            before=struct.unpack_from('>I',p,off)[0];struct.pack_into('>I',p,off,value)
            changes.append(dict(field=name,offset=off,before=before,after=value))
        self.put(0,p);return changes
    def read(self,pos,n):
        n=min(n,max(0,self.size-pos));self.file.seek(pos);data=bytearray(self.file.read(n)).ljust(n,b'\0')
        if n:
            for lba in range(pos//2352,(pos+n-1)//2352+1):
                if lba in self.sectors:
                    a=max(pos,lba*2352);b=min(pos+n,(lba+1)*2352)
                    data[a-pos:b-pos]=self.sectors[lba][a-lba*2352:b-lba*2352]
        return bytes(data)

class VFS:
    def __init__(self,overlay):
        self.overlay=overlay;self.handles={};self.next_handle=100;self.calls={};self.errors=[];self.requests=[];self.opened=[];self.callbacks=[]
        def cb(name,rt,args,func):
            def call(*a):
                self.calls[name]=self.calls.get(name,0)+1
                try:return func(*a)
                except Exception as ex:self.errors.append((name,str(ex)));return -1 if rt not in (C.c_void_p,C.c_bool) else 0
            c=C.CFUNCTYPE(rt,*args)(call);self.callbacks.append(c);return c
        def open_(path,mode,hints):
            s=path.decode();self.opened.append(dict(path=s,mode=mode,hints=hints))
            if mode&2:return None
            normal=s.replace('\\','/')
            if normal.endswith('/disc.cue'):f=io.BytesIO(overlay.cue_bytes);size=len(overlay.cue_bytes);kind='cue'
            elif normal.endswith('/disc.bin'):f=None;size=overlay.size;kind='overlay'
            elif '://' not in normal:
                p=Path(s)
                if not p.is_file():return None
                f=p.open('rb');size=p.stat().st_size;kind='readonly'
            else:return None
            self.next_handle+=1;h=self.next_handle
            self.handles[h]=dict(path=C.create_string_buffer(path),f=f,size=size,pos=0,kind=kind);return h
        def close(h):
            s=self.handles.pop(h)
            if s['f']:s['f'].close()
            return 0
        def seek(h,off,origin):
            s=self.handles[h];p=off+(s['pos'] if origin==1 else s['size'] if origin==2 else 0)
            if p<0:return -1
            s['pos']=p;return p
        def read(h,dest,n):
            s=self.handles[h];n=min(n,max(0,s['size']-s['pos']))
            if s['kind']=='overlay':b=overlay.read(s['pos'],n)
            else:s['f'].seek(s['pos']);b=s['f'].read(n)
            C.memmove(dest,b,len(b));s['pos']+=len(b);return len(b)
        def stat(path,size):
            s=path.decode().replace('\\','/')
            if s.endswith('/disc.cue'):n=len(overlay.cue_bytes)
            elif s.endswith('/disc.bin'):n=overlay.size
            elif '://' not in s and Path(s).exists():
                if Path(s).is_dir():return 3
                n=Path(s).stat().st_size
            else:return 0
            if size:size[0]=n
            return 1
        v=C.c_void_p;i=C.c_int;u=C.c_uint;q=C.c_int64
        fields=[('get_path',v,[v],lambda h:C.addressof(self.handles[h]['path'])),('open',v,[C.c_char_p,u,u],open_),('close',i,[v],close),('size',q,[v],lambda h:self.handles[h]['size']),('tell',q,[v],lambda h:self.handles[h]['pos']),('seek',q,[v,q,i],seek),('read',q,[v,v,C.c_uint64],read),('write',q,[v,v,C.c_uint64],lambda *a:-1),('flush',i,[v],lambda *a:0),('remove',i,[C.c_char_p],lambda *a:-1),('rename',i,[C.c_char_p,C.c_char_p],lambda *a:-1),('truncate',q,[v,q],lambda *a:-1),('stat',i,[C.c_char_p,C.POINTER(C.c_int32)],stat),('mkdir',i,[C.c_char_p],lambda *a:-1),('opendir',v,[C.c_char_p,C.c_bool],lambda *a:None),('readdir',C.c_bool,[v],lambda *a:False),('dirent_get_name',v,[v],lambda *a:None),('dirent_is_dir',C.c_bool,[v],lambda *a:False),('closedir',i,[v],lambda *a:0)]
        class Interface(C.Structure):_fields_=[(name,C.c_void_p) for name,rt,args,f in fields]
        self.interface=Interface(*[C.cast(cb(name,rt,args,f),C.c_void_p) for name,rt,args,f in fields])
    def request(self,data):
        req=C.cast(data,C.POINTER(VFSInfo)).contents;self.requests.append(req.required_interface_version)
        if req.required_interface_version>3:return False
        req.required_interface_version=3;req.iface=C.addressof(self.interface);return True
    def report(self):return dict(request_versions=self.requests,calls=self.calls,opened=self.opened,errors=self.errors)

def field(data,name,size=None,start=0):
    key=bytes([len(name)])+name.encode();at=start
    while (at:=data.find(key,at))>=0:
        n=int.from_bytes(data[at+len(key):at+len(key)+4],'little')
        if size is None or n==size:return at+len(key)+4,n
        at+=1
    raise ValueError((name,size,start))

def state_values(state):
    mp,_=field(state,'DSP.MPROG',1024);ram,_=field(state,'RAM',524288,mp);vram,_=field(state,'VRAM',524288)
    def u16(p):return int.from_bytes(state[p:p+2],'little')
    def u32(p):return (u16(p)<<16)|u16(p+2)
    out=dict(dsp_clock=u16(ram+0xb010),music_state=u16(vram+0x7f000+132),music_error=u32(vram+0x7f000+136),music_events=u32(vram+0x7f000+140),late_count=u16(ram+0xb028),dropped_fm=u16(ram+0xb03a),mcu_state=u16(ram+0xc046))
    out['native_profile']=u16(ram+0xc05e)
    out['native_profile_name']={0x53f1:'SFX',0x4d46:'MUSIC'}.get(out['native_profile'],'unknown_or_booting')
    out['music_heartbeat_required']=out['native_profile']==0x4d46
    abi=u16(vram+0x5c000+4) if u32(vram+0x5c000)==0x57464d53 else None
    out['music_shared_abi']=abi
    if abi is not None and abi>=3:
        out.update(master_frame=u16(vram+0x7f000+164),worker_frame=u16(vram+0x7f000+166),dsp_progress_frame=u16(vram+0x7f000+168))
        out['worker_lag_frames']=(out['master_frame']-out['worker_frame'])&65535
        out['dsp_progress_lag_frames']=(out['master_frame']-out['dsp_progress_frame'])&65535
    return out

def guest_counters(symbols,read32,read8):
    """Observe target ABI fields through read-only callbacks; no state changes."""
    out={}
    for name in ('_video_clock','_fatal_owner','_fatal_dependency','_fatal_frame','_finished_frame','_rate_draws','_rate_start_clock','_frontend_serviced_clock','_read_clock','_boot_seed_observed','_attract_exit_frame'):
        if name in symbols:out[name[1:]]=read32(symbols[name][0])
    for name in ('_rate_value','_rate_ready'):
        if name in symbols:out[name[1:]]=read8(symbols[name][0])
    for name in ('_boot_timing','_scsp_timing','_scsp_io','_frontend_observed'):
        if name in symbols:
            addr,size=symbols[name];out[name[1:]]=[read32(addr+i) for i in range(0,size,4)]
    if '_rt' in symbols:out['game_frame']=read32(symbols['_rt'][0]+24)
    if '_backend' in symbols:
        addr,size=symbols['_backend'];assert size==1684,('backend ABI changed',size)
        for offset,name in [(1156,'texture_bytes'),(1160,'texture_transfers'),(1164,'texture_reused'),(28,'emitted_commands')]:out[name]=read32(addr+offset)
    if '_wf_r18_scsp_diagnostics' in symbols:
        addr,size=symbols['_wf_r18_scsp_diagnostics'];assert size==20,('audio diagnostic ABI changed',size)
        diag={name:read32(addr+offset) for offset,name in [(0,'reason'),(4,'dropped_sfx'),(8,'dropped_music'),(12,'ignored')]}
        diag['disabled']=bool(read8(addr+16));out['audio_diagnostics']=diag
    if '_wf_saturn_allocator_partition' in symbols:
        addr,size=symbols['_wf_saturn_allocator_partition'];assert size in (32,48),('allocator diagnostic ABI changed',size)
        raw=[read32(addr+i*4) for i in range(size//4)]
        out['allocator_partition_raw']=raw
        out['allocator_partition']=dict(zip(('raw_private_bytes','useful_private_bytes','raw_user_bytes','useful_user_bytes','tlsf_control_bytes','pool_overhead_bytes','private_handle','user_handle','user_control_address','user_control_block_bytes','allocation_overhead_bytes','user_pool_address'),raw))
    if '_wf_saturn_allocator_live_stats' in symbols:
        addr,size=symbols['_wf_saturn_allocator_live_stats'];assert size==64
        out['allocator_live_stats']=dict(zip(('captures','faults','private_used','private_free','private_largest_free','private_used_blocks','private_free_blocks','user_used','user_free','user_largest_free','user_used_blocks','user_free_blocks','private_reported_size','user_reported_size','private_consistency','user_consistency'),[read32(addr+i*4) for i in range(16)]))
    return out

def audio_gate(rows,trace):
    observed=[x['audio_diagnostics'] for x in trace if 'audio_diagnostics' in x and x.get('video_clock',0)>0]
    blockers=[]
    if not observed:blockers.append('runtime_audio_diagnostics_missing_or_not_initialized')
    else:
        if any(x['disabled'] for x in observed):blockers.append('audio_disabled')
        for key in ('reason','dropped_sfx','dropped_music','ignored'):
            if any(x[key] for x in observed):blockers.append('audio_diagnostics_'+key)
    if any(x.get('dropped_fm',0) for x in rows):blockers.append('firmware_dropped_fm')
    active=[x for x in rows if x.get('native_profile')==0x4d46]
    if any(x.get('music_error',0) for x in active if x.get('music_state') in (1,2,3)):blockers.append('music_error')
    running=[x for x in active if x.get('music_shared_abi',0) is not None and x.get('music_shared_abi',0)>=3 and x.get('music_state')==2]
    for key in ('worker_lag_frames','dsp_progress_lag_frames'):
        if any(30<x.get(key,0)<32768 for x in running):blockers.append(key)
    return dict(software_audio_gate_passed=not blockers,blockers=blockers,diagnostic_samples=len(observed),physical_audio_certified=False,scope='No stability claim is permitted when disabled, reason, drops, ignored or music errors are observed. A passing gate alone is not complete audio or graphics validation.')

def code_identity(state,audio_elfs):
    """Read-only evidence of code delivered by CDFS into both target memories."""
    mp,_=field(state,'DSP.MPROG',1024);ram,_=field(state,'RAM',524288,mp);vram,_=field(state,'VRAM',524288)
    out={}
    for name,elf in audio_elfs.items():
        samples=[]
        for segment,p in enumerate(elf.ph):
            if p[0]!=1 or not p[6]&1 or p[4]<32:continue
            skip=0x400 if name=='SCSP.ELF' and p[2]==0 and p[4]>0x500 else 0
            length=min(256,p[4]-skip);address=p[2]+skip
            offset=ram+address if name=='SCSP.ELF' else vram+address-0x05c00000
            raw=state[offset:offset+length];actual=bytearray(raw);actual[::2],actual[1::2]=raw[1::2],raw[::2]
            expected=elf.data[p[1]+skip:p[1]+skip+length]
            samples.append(dict(segment=segment,address=hex(address),bytes=length,expected_sha256=sha(expected),observed_sha256=sha(actual),matches=actual==expected))
        out[name]=dict(elf_sha256=sha(elf.data),executable_samples=samples)
    return out

def main():
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('--core',type=Path,default=ROOT/'toolchains/beetle-saturn/mednafen_saturn_libretro.dll')
    p.add_argument('--bios-dir',type=Path,default=ROOT/'local-bios');p.add_argument('--cue',type=Path,required=True)
    p.add_argument('--runtime',type=Path);p.add_argument('--symbols',type=Path);p.add_argument('--music',type=Path);p.add_argument('--scsp',type=Path)
    p.add_argument('--out',type=Path,required=True);p.add_argument('--frames',type=int,default=4200);p.add_argument('--input',choices=['none','tag','royal'],default='none')
    p.add_argument('--inspect-only',action='store_true');a=p.parse_args();a.out.mkdir(parents=True,exist_ok=True)
    replacements={};elf=None
    if a.runtime:elf=Elf(a.runtime);replacements['A.BIN']=elf.binary()
    elif a.symbols:elf=Elf(a.symbols)
    if a.music:replacements['MUSIC.ELF']=a.music.read_bytes()
    if a.scsp:replacements['SCSP.ELF']=a.scsp.read_bytes()
    original=Overlay(a.cue,{});original_manifest=original.manifest();original.file.close()
    overlay=Overlay(a.cue,replacements);boot_changes=overlay.patch_boot(elf,replacements['A.BIN']) if a.runtime else []
    overlay.verify();vfs=VFS(overlay)
    audio_elfs={name:Elf(Path(name),overlay.contents(name)) for name in ('SCSP.ELF','MUSIC.ELF')}
    manifest=overlay.manifest()
    report=dict(scope='Read-only libretro VFS RAM filesystem overlay; cold boot; no state loads or guest memory edits. Infrastructure run does not certify repairs.',physical_validation=False,disc_images_created=False,state_loads=0,guest_memory_edits=False,existing_media_sha256=overlay.original_sha,overlay_sectors=len(overlay.sectors),replacements=overlay.replacements,boot_header_changes=boot_changes,boot_area_sha256=sha(b''.join(overlay.payload(i) for i in range(16))),files=overlay.files,manifest=manifest,changed_files=[name for name in manifest if manifest[name]!=original_manifest[name]],core_sha256=file_sha(a.core),observations=[],full_cache=True,exact_interleave=True,jit_disabled=True,virtual_bytes=overlay.size,original_bytes=overlay.original_size)
    if elf:
        report['elf']=dict(path=str(elf.path.resolve()),sha256=sha(elf.data),flat_sha256=sha(elf.binary()),flat_bytes=len(elf.binary()),format='ELF32BE SHF_ALLOC sections with LMA derived from containing PT_LOAD; no ELF header is executed',symbols={name:dict(address=address,bytes=size) for name,(address,size) in elf.symbols.items() if name in ('_rt','_video_clock','_fatal_owner','_fatal_dependency','_fatal_frame','_backend','_rate_draws','_rate_start_clock','_rate_value','_wf_r18_scsp_diagnostics','_wf_saturn_allocator_partition')})
        canonical=elf.path.with_name('A.BIN')
        if canonical.is_file():report['elf']['existing_canonical_flat_binary_equal']=canonical.read_bytes()==elf.binary()
    if a.inspect_only:(a.out/'inspection.json').write_text(json.dumps(report,indent=2));print(json.dumps(report,indent=2));return
    system=str(a.bios_dir.resolve()).encode();save=str(a.out.resolve()).encode();last_video=None;last_crc=None;changed=0;pixel=1;samples=bytearray();frame=0
    opts={b'beetle_saturn_region':b'North America',b'beetle_saturn_cart':b'None',b'beetle_saturn_cpucache_emumode':b'full',b'beetle_saturn_sh2_interleave':b'exact',b'beetle_saturn_sh2_jit':b'disabled',b'beetle_saturn_jit_scu':b'disabled',b'beetle_saturn_jit_scsp':b'disabled'}
    option_requests=[];accepted_options={};env_commands=[]
    @ENV
    def environment(cmd,data):
        nonlocal pixel
        env_commands.append(cmd)
        if cmd in (9,31):C.cast(data,C.POINTER(C.c_char_p))[0]=system if cmd==9 else save;return True
        if cmd==3:C.cast(data,C.POINTER(C.c_bool))[0]=True;return True
        if cmd==10:pixel=C.cast(data,C.POINTER(C.c_uint))[0];return pixel in (1,2)
        if cmd==15:
            var=C.cast(data,C.POINTER(Variable)).contents;option_requests.append(var.key.decode())
            if var.key in opts:var.value=opts[var.key];accepted_options[var.key.decode()]=opts[var.key].decode();return True
            return False
        if cmd==17:C.cast(data,C.POINTER(C.c_bool))[0]=False;return True
        if cmd==52:C.cast(data,C.POINTER(C.c_uint))[0]=0;return True
        if cmd in (45,45|0x10000):return vfs.request(data)
        return False
    @VIDEO
    def video(data,w,h,pitch):
        nonlocal last_video,last_crc,changed
        if not data or data==C.c_void_p(-1).value:return
        raw=C.string_at(data,h*pitch);crc=zlib.crc32(raw)
        if last_crc is not None and crc!=last_crc:changed+=1
        last_crc=crc;last_video=(raw,w,h,pitch,pixel)
    @AUDIO
    def audio(l,r):samples.extend(struct.pack('<hh',l,r))
    @BATCH
    def batch(data,n):samples.extend(C.string_at(data,n*4));return n
    @POLL
    def poll():pass
    events=[(4200,4230,11),(4350,4380,3)]+[(n,n+30,0) for n in (5000,5500,6000,6500,7000,7500,8000,8500)]
    if a.input=='royal':events.append((4650,4680,6))
    @INPUT
    def input_state(port,device,index,button):return int(a.input!='none' and port==0 and device==1 and any(x<=frame<y and button==b for x,y,b in events))
    core=C.CDLL(str(a.core.resolve()))
    for name,typ,callback in [('environment',ENV,environment),('video_refresh',VIDEO,video),('audio_sample',AUDIO,audio),('audio_sample_batch',BATCH,batch),('input_poll',POLL,poll),('input_state',INPUT,input_state)]:
        f=getattr(core,'retro_set_'+name);f.argtypes=[typ];f(callback)
    core.retro_init();core.retro_load_game.argtypes=[C.POINTER(Game)];core.retro_load_game.restype=C.c_bool
    loaded=core.retro_load_game(C.byref(Game(b'overlay://session/disc.cue',None,0,None)))
    report.update(load_success=bool(loaded),vfs=vfs.report(),environment_commands=sorted(set(env_commands)),option_requests=option_requests)
    (a.out/'report.json').write_text(json.dumps(report,indent=2)+'\n')
    assert loaded,'VFS virtual cold boot rejected; inspect report.json'
    assert vfs.calls.get('read',0)>0,'Core did not read VFS'
    core.retro_serialize_size.restype=C.c_size_t;core.retro_serialize.argtypes=[C.c_void_p,C.c_size_t];core.retro_serialize.restype=C.c_bool
    core.retro_get_memory_data.argtypes=[C.c_uint];core.retro_get_memory_data.restype=C.c_void_p
    core.retro_get_memory_size.argtypes=[C.c_uint];core.retro_get_memory_size.restype=C.c_size_t
    mem=core.retro_get_memory_data(2);memsize=core.retro_get_memory_size(2);base=None
    if elf:
        binary=elf.binary();signature=bytearray(binary[:4096]);signature[::2],signature[1::2]=binary[1:4096:2],binary[:4096:2]
    counter_trace=[]
    def read32(addr):
        b=C.string_at(base+(addr&0x1fffffff)-0x06000000,4);return int.from_bytes(b[1::-1]+b[3:1:-1],'big')
    def read8(addr):
        addr&=0x1fffffff;return C.string_at(base+((addr-0x06000000)^1),1)[0]
    def counters():
        return guest_counters(elf.symbols,read32,read8)
    start=time.monotonic()
    try:
        for frame in range(1,a.frames+1):
            core.retro_run()
            if elf and base:counter_trace.append(dict(frame=frame,**counters()))
            if frame%300==0 or frame==a.frames:
                n=core.retro_serialize_size();buf=C.create_string_buffer(n);assert core.retro_serialize(buf,n);state=buf.raw
                row=state_values(state);row.update(frame=frame,video_changes=changed,video_crc=last_crc,elapsed_seconds=round(time.monotonic()-start,2))
                if elf and mem:
                    if base is None:
                        off=C.string_at(mem,memsize).find(signature)
                        if off>=0:base=mem+off-0x4000
                    row['runtime_signature_found']=base is not None
                    if base:row.update(counters())
                row['loaded_audio_code']=code_identity(state,audio_elfs)
                if counter_trace and counter_trace[-1]['frame']==frame:
                    counter_trace[-1]['audio_checkpoint']={key:row[key] for key in ('music_shared_abi','master_frame','worker_frame','dsp_progress_frame','worker_lag_frames','dsp_progress_lag_frames','dsp_clock','music_state','music_error','music_events','late_count','dropped_fm','mcu_state','native_profile','music_heartbeat_required') if key in row}
                vals=array.array('h',samples);row['audio_peak']=max(map(abs,vals),default=0);row['audio_rms']=(sum(v*v for v in vals)/max(1,len(vals)))**.5;samples.clear()
                report['observations'].append(row);report['vfs']=vfs.report();report['option_requests']=option_requests
                (a.out/'report.json').write_text(json.dumps(report,indent=2)+'\n');print(json.dumps(row),flush=True)
                if frame in (1800,3000,4200,a.frames):(a.out/f'frame-{frame}.state').write_bytes(state)
                if last_video and frame in (1800,3000,4200,a.frames):
                    from PIL import Image
                    raw,w,h,pitch,fmt=last_video;Image.frombytes('RGB',(w,h),raw,'raw','BGRX' if fmt==1 else 'BGR;16',pitch).save(a.out/f'frame-{frame}.png')
    finally:
        core.retro_unload_game();core.retro_deinit()
        report['existing_media_unchanged']=file_sha(overlay.path)==overlay.original_sha
        report['vfs']=vfs.report();report['input_events']=events if a.input!='none' else [];report['accepted_options']=accepted_options
        report['counter_trace_samples']=len(counter_trace)
        report['audio_gate']=audio_gate(report['observations'],counter_trace)
        report['audio_trace_sampling']='HWRAM diagnostics every frame after signature; VDP1 ABI3 heartbeats at serialized checkpoints (300 frames and final).'
        (a.out/'counter-trace.json').write_text(json.dumps(counter_trace,separators=(',',':'))+'\n')
        (a.out/'report.json').write_text(json.dumps(report,indent=2)+'\n')

if __name__=='__main__':main()
