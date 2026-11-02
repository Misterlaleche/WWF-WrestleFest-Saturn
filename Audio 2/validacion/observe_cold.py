"""Read-only extensions of the existing cold VFS observer, kept in test scope.

No RAM/state writes; no muting; WAV is a copy of the libretro audio callback.
This compiles the existing observer in memory with small checked insertions so
its image-free VFS, input script, core options and lifecycle remain authoritative.
"""
import argparse
import hashlib
import json
import re
import struct
import sys
import wave
from pathlib import Path

HERE=Path(__file__).resolve().parent
REPAIR=HERE.parents[1]
ORIGINAL=REPAIR/'emulator/cold_overlay.py'
sys.path.insert(0,str(ORIGINAL.parent))
from observer_identity import direct_load_identity


def scsp_load_digest(elf):
    """Pin target addresses, zero-fill bounds, flags and bytes, not SYMTAB."""
    digest=hashlib.sha256();loads=0
    for p in elf.ph:
        if p[0]!=1:continue
        _,offset,address,physical,file_bytes,memory_bytes,flags,_=p
        if address!=physical or file_bytes>memory_bytes or address+memory_bytes>0x80000 or offset>len(elf.data) or file_bytes>len(elf.data)-offset:
            raise ValueError('Invalid SCSP load segment')
        digest.update(struct.pack('>5I',address,physical,file_bytes,memory_bytes,flags))
        digest.update(elf.data[offset:offset+file_bytes]);loads+=1
    if not loads:raise ValueError('SCSP load segments absent')
    return digest.hexdigest()


def bind_scsp_counter(debug,release,layout,layout_sha256):
    """Accept only a compiler-derived layout tied to this exact target ELF."""
    if layout.get('schema')!='wf_scsp_sfx_counter_v1':raise ValueError('SCSP counter layout schema missing or unsupported')
    for key in ('debug_elf_sha256','release_elf_sha256','load_segments_sha256','compiler_sha256','header_sha256','probe_source_sha256'):
        if not isinstance(layout.get(key),str) or not re.fullmatch('[0-9a-f]{64}',layout[key]):raise ValueError('SCSP layout hash missing: '+key)
    if not isinstance(layout_sha256,str) or not re.fullmatch('[0-9a-f]{64}',layout_sha256):raise ValueError('SCSP layout itself is not pinned')
    if not layout.get('compiler_id'):raise ValueError('SCSP layout compiler identity missing')
    if hashlib.sha256(debug.data).hexdigest()!=layout['debug_elf_sha256']:raise ValueError('SCSP debug ELF changed')
    if hashlib.sha256(release.data).hexdigest()!=layout['release_elf_sha256']:raise ValueError('SCSP release ELF changed')
    load=scsp_load_digest(debug)
    if load!=layout['load_segments_sha256'] or scsp_load_digest(release)!=load:raise ValueError('SCSP debug/release target loads differ')
    symbol=layout.get('engine_symbol')
    if symbol not in debug.symbols:raise ValueError('Exact SCSP engine symbol absent')
    address,size=debug.symbols[symbol];expected=layout.get('engine_bytes');offset=layout.get('underruns_offset');width=layout.get('underruns_bytes')
    if type(expected)is not int or size!=expected or size<=0:raise ValueError('SCSP engine sizeof differs from compiled layout')
    if type(offset)is not int or offset<0 or width!=4 or offset+4>size or (address+offset)&1:raise ValueError('Invalid32-bit SCSP underrun offset/width')
    if not 0<=address<address+size<=0x80000 or not any(p[0]==1 and p[6]&2 and p[2]<=address and address+size<=p[2]+p[5] for p in debug.ph):raise ValueError('SCSP engine lies outside writable target memory')
    return dict(validated=True,counter_bytes=4,engine_address=address,engine_bytes=size,counter_address=address+offset,
        engine_symbol=symbol,layout_sha256=layout_sha256,debug_elf_sha256=layout['debug_elf_sha256'],
        release_elf_sha256=layout['release_elf_sha256'],load_segments_sha256=load,compiler_id=layout['compiler_id'])


def sample_scsp_counter(state,sound,binding):
    """Serialized SCSP words are host-little-endian; target longs are BE words."""
    if sound<0 or sound+0x80000>len(state):raise ValueError('Serialized SCSP RAM missing or truncated')
    def word(address):return int.from_bytes(state[sound+address:sound+address+2],'little')
    address=binding.get('counter_address')
    if binding.get('validated')is not True or binding.get('counter_bytes')!=4 or type(address)is not int or not 0<=address<=0x80000-4 or address&1:raise ValueError('Unvalidated SCSP counter binding')
    return dict(pcm_engine_underruns=(word(address)<<16)|word(address+2),pcm_underruns_mail=word(0xc05c),pcm_engine_observation=dict(binding))


def inserted_source():
    source=ORIGINAL.read_text()
    edits=[
        ("        po=struct.unpack_from('>I',d,28)[0];pe,pn=struct.unpack_from('>HH',d,42)",
         '''        grouped={}
        for name,value in list(self.symbols.items()):
            base=re.sub(r'\\.lto_priv\\.\\d+$','',name)
            if base!=name:grouped.setdefault(base,[]).append((name,value))
        self.lto_aliases={}
        for base,values in grouped.items():
            if base not in self.symbols and len(values)==1:
                self.symbols[base]=values[0][1];self.lto_aliases[base]=values[0][0]
        po=struct.unpack_from('>I',d,28)[0];pe,pn=struct.unpack_from('>HH',d,42)'''),
        ('    @AUDIO\n    def audio(l,r):samples.extend(struct.pack(\'<hh\',l,r))',
         '''    wave_sink=None;wave_frames=0
    @AUDIO
    def audio(l,r):
        nonlocal wave_frames
        pcm=struct.pack('<hh',l,r);samples.extend(pcm)
        if wave_sink:wave_sink.writeframesraw(pcm);wave_frames+=1'''),
        ('    def batch(data,n):samples.extend(C.string_at(data,n*4));return n',
         '''    def batch(data,n):
        nonlocal wave_frames
        pcm=C.string_at(data,n*4);samples.extend(pcm)
        if wave_sink:wave_sink.writeframesraw(pcm);wave_frames+=n
        return n'''),
        ("    assert vfs.calls.get('read',0)>0,'Core did not read VFS'",
         '''    assert vfs.calls.get('read',0)>0,'Core did not read VFS'
    if VALIDATION_WAV:
        class Geometry(C.Structure):
            _fields_=[('base_width',C.c_uint),('base_height',C.c_uint),('max_width',C.c_uint),('max_height',C.c_uint),('aspect_ratio',C.c_float)]
        class Timing(C.Structure):_fields_=[('fps',C.c_double),('sample_rate',C.c_double)]
        class AVInfo(C.Structure):_fields_=[('geometry',Geometry),('timing',Timing)]
        av=AVInfo();core.retro_get_system_av_info.argtypes=[C.POINTER(AVInfo)];core.retro_get_system_av_info(C.byref(av))
        sample_rate=round(av.timing.sample_rate)
        assert sample_rate>0 and abs(sample_rate-av.timing.sample_rate)<0.01
        VALIDATION_WAV.parent.mkdir(parents=True,exist_ok=True)
        wave_sink=wave.open(str(VALIDATION_WAV),'wb');wave_sink.setnchannels(2);wave_sink.setsampwidth(2);wave_sink.setframerate(sample_rate)'''),
        ('        return guest_counters(elf.symbols,read32,read8)',
         '        return guest_counters(elf.symbols,read32,read8,lambda address,n:C.string_at(base+(address&0x1fffffff)-0x06000000,n))'),
        ('            if frame%300==0 or frame==a.frames:',
         '            if frame%VALIDATION_CHECKPOINT==0 or frame==a.frames:'),
        ("        report['existing_media_unchanged']=file_sha(overlay.path)==overlay.original_sha",
         '''        if wave_sink:
            wave_sink.close()
            report['wav_capture']=dict(path=str(VALIDATION_WAV),frames=wave_frames,sample_rate=sample_rate,channels=2,bits=16,guest_muted=False,callback_copy_only=True)
        report['existing_media_unchanged']=file_sha(overlay.path)==overlay.original_sha'''),
        ("        report['audio_trace_sampling']='HWRAM diagnostics every frame after signature; VDP1 ABI3 heartbeats at serialized checkpoints (300 frames and final).'",
         '''        report['audio_trace_sampling']='Read-only HWRAM counters each frame; full music/SCSP state each requested checkpoint.'
        report['validation_observer']=dict(original_path=str(VALIDATION_ORIGINAL),original_sha256=VALIDATION_ORIGINAL_SHA,identity_helper_sha256=VALIDATION_IDENTITY_HELPER_SHA,checkpoint_frames=VALIDATION_CHECKPOINT,case=VALIDATION_CASE,layout=VALIDATION_LAYOUT,scsp_counter=VALIDATION_SCSP_COUNTER,guest_memory_edits=False,guest_muted=False)''')]
    for old,new in edits:
        if source.count(old)!=1:raise ValueError('Upstream observer changed; review insertion: '+old[:80])
        source=source.replace(old,new)
    return direct_load_identity(source)


def configure(ns,debug,layout,case,scsp_debug,scsp_layout,scsp_release,scsp_layout_sha256):
    base_state=ns['state_values'];base_counters=ns['guest_counters'];field=ns['field']
    scsp_binding=bind_scsp_counter(scsp_debug,scsp_release,scsp_layout,scsp_layout_sha256)
    def state_values(state):
        row=base_state(state)
        mp,_=field(state,'DSP.MPROG',1024);sound,_=field(state,'RAM',524288,mp)
        vram,_=field(state,'VRAM',524288);work,_=field(state,'WorkRAMH',1048576)
        def u16(at):return int.from_bytes(state[at:at+2],'little')
        def u32(at):return (u16(at)<<16)|u16(at+2)
        def guest_offset(addr):
            addr &= 0x1fffffff
            if 0x06000000<=addr<0x06100000:return work+addr-0x06000000
            if 0x05c00000<=addr<0x05c80000:return vram+addr-0x05c00000
            raise ValueError(('Unsupported read-only target address',hex(addr)))
        def sym16(name):return u16(guest_offset(debug.symbols['_'+name][0]))
        def sym32(name):return u32(guest_offset(debug.symbols['_'+name][0]))
        shared=vram+0x7f000
        row.update(music_producer=u16(shared),music_consumer=u16(shared+2),
            music_generation=u16(shared+160),music_last_command=u16(shared+162),
            music_workspace=u32(shared+172),firmware_generation=u16(sound+0xb024),
            firmware_generation_ack=u16(sound+0xb02a),firmware_state=u16(sound+0xb026),
            ring_producer=u16(sound+0xb020),ring_consumer=u16(sound+0xb022),
            fm_active_mask=u16(sound+0xc054),late_key_events=u16(sound+0xb038),
            dsp_jump_count=u16(sound+0xb03c),mail_request=u16(sound+0xc048),
            mail_ack=u16(sound+0xc04a),mail_command=u16(sound+0xc04c),
            mail_argument=u16(sound+0xc04e),mail_result=u16(sound+0xc050))
        if row.get('native_profile') in (0x4d46,0x53f1) and row.get('mcu_state')==2:
            row.update(sample_scsp_counter(state,sound,scsp_binding))
        row['accepted_sfx_word']=(row['mail_argument']&255) if (row['mail_command']==7 and row['mail_request']==row['mail_ack'] and row['mail_result']==0 and 0x20<=(row['mail_argument']&255)<=0x81) else None
        row['ring_backlog']=(row['ring_producer']-row['ring_consumer'])&65535
        if row['native_profile']==0x4d46 and row['music_state']==2:
            row['test']={name:sym16('wf_test_'+name) for name in ('started','start_frame','applied_track')}
            row['test'].update({name:sym32('wf_test_'+name) for name in ('suppressed_music','suppressed_stop','suppressed_fade')})
            row['test']['elapsed_master_frames']=(row.get('master_frame',0)-row['test']['start_frame'])&65535
            hot_start=debug.symbols['_wf_hotcode_start'][0];hot_end=debug.symbols['_wf_hotcode_end'][0]
            if not 0x06004000<=hot_start<hot_end<=0x06100000:raise ValueError('Test ELF is not linked into HWRAM')
            segment=next(p for p in debug.ph if p[0]==1 and p[2]<=hot_start and hot_end<=p[2]+p[4])
            expected=debug.data[segment[1]+hot_start-segment[2]:segment[1]+hot_end-segment[2]]
            raw=state[guest_offset(hot_start):guest_offset(hot_end)];observed=bytearray(raw)
            observed[::2],observed[1::2]=raw[1::2],raw[::2]
            row['hot_code_identity']=dict(address=hot_start,bytes=len(expected),expected_sha256=hashlib.sha256(expected).hexdigest(),observed_sha256=hashlib.sha256(observed).hexdigest(),matches=observed==expected)
            hot=sym32('hot');row['hot_state_pointer']=hot
            if hot!=hot_end:raise ValueError(('Unexpected final hot-state layout',hex(hot),hex(hot_end)))
            ram=guest_offset(hot)+layout['sequence_ram']
            # Serialized SH2 bytes are stored word-swapped; all reads stay local.
            row['sequence_active']=state[ram+(6^1)]
            row['sequence_fade']=state[ram+(0x62^1)]
            cycles=guest_offset(hot)+layout['sequence_cycles']
            boot=guest_offset(debug.symbols['_boot_snapshot'][0])+layout['sequence_cycles']
            row['sequence_cycles']=(u32(cycles)<<32)|u32(cycles+4)
            origin=(u32(boot)<<32)|u32(boot+4)
            row['score_elapsed_seconds']=(row['sequence_cycles']-origin)/3579545
        return row

    def guest_counters(symbols,read32,read8,readraw):
        row=base_counters(symbols,read32,read8)
        if '_rt' in symbols:
            addr=symbols['_rt'][0]
            stage=read8(addr+layout['runtime_frame_stage']);ready=read8(addr+layout['runtime_frame_ready'])
            row['runtime_frame_stage']=stage;row['runtime_frame_ready']=ready
            # Stage1 is the scheduler's completed phase list while awaiting ACK.
            # Compare the LAST such sample per game frame, after world/HUD commit.
            if stage==1 and ready:
                raw=bytearray(readraw(addr,layout['scene_prefix_bytes']))
                for offset in (layout['scene_frame_handshake'],layout['scene_frame_handshake']+1,layout['scene_vblank_enable']):raw[offset^1]=0
                row['scene_sha256']=hashlib.sha256(raw).hexdigest()
        return row
    ns['state_values']=state_values;ns['guest_counters']=guest_counters


def main():
    p=argparse.ArgumentParser(add_help=False)
    p.add_argument('--music-debug',type=Path,required=True);p.add_argument('--layout',type=Path,required=True)
    p.add_argument('--scsp-debug',type=Path,required=True);p.add_argument('--scsp-layout',type=Path,required=True)
    p.add_argument('--case',type=Path,required=True);p.add_argument('--checkpoint',type=int,default=60);p.add_argument('--wav',type=Path)
    extra,remaining=p.parse_known_args()
    if extra.checkpoint<1:raise ValueError('Checkpoint interval must be positive')
    ns=dict(__file__=str(ORIGINAL),__name__='cold_overlay_validation',wave=wave,
            VALIDATION_WAV=extra.wav,VALIDATION_CHECKPOINT=extra.checkpoint,
            VALIDATION_ORIGINAL=ORIGINAL,VALIDATION_ORIGINAL_SHA=hashlib.sha256(ORIGINAL.read_bytes()).hexdigest(),
            VALIDATION_IDENTITY_HELPER_SHA=hashlib.sha256((ORIGINAL.parent/'observer_identity.py').read_bytes()).hexdigest(),
            VALIDATION_CASE=json.loads(extra.case.read_text()),VALIDATION_LAYOUT=json.loads(extra.layout.read_text()))
    exec(compile(inserted_source(),str(ORIGINAL),'exec'),ns)
    debug=ns['Elf'](extra.music_debug)
    target=argparse.ArgumentParser(add_help=False);target.add_argument('--scsp',type=Path,required=True)
    release,_=target.parse_known_args(remaining)
    scsp_debug=ns['Elf'](extra.scsp_debug);scsp_release=ns['Elf'](release.scsp)
    scsp_layout=json.loads(extra.scsp_layout.read_text());scsp_layout_sha=hashlib.sha256(extra.scsp_layout.read_bytes()).hexdigest()
    configure(ns,debug,ns['VALIDATION_LAYOUT'],ns['VALIDATION_CASE'],scsp_debug,scsp_layout,scsp_release,scsp_layout_sha)
    ns['VALIDATION_SCSP_COUNTER']=bind_scsp_counter(scsp_debug,scsp_release,scsp_layout,scsp_layout_sha)
    sys.argv=[sys.argv[0],*remaining]
    ns['main']()


if __name__=='__main__':main()
