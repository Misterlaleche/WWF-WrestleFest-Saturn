"""Read-only cold harness extension: exact WAV and unique LTO symbol aliases."""
from pathlib import Path
import argparse,hashlib,json,sys,wave,ctypes
from observer_identity import direct_load_identity

here=Path(__file__).resolve().parent
parser=argparse.ArgumentParser(add_help=False)
parser.add_argument('--wav',type=Path,required=True)
options,rest=parser.parse_known_args()
target=options.wav.resolve();target.parent.mkdir(parents=True,exist_ok=True)
original=here/'cold_overlay.py';source=direct_load_identity(original.read_text())

anchor="        po=struct.unpack_from('>I',d,28)[0];pe,pn=struct.unpack_from('>HH',d,42)"
assert source.count(anchor)==1
replacement='''        # Host-only aliases for GCC LTO-local symbol names. Never guess when
        # two private objects have the same basename at different addresses.
        grouped={}
        for name,value in list(self.symbols.items()):
            base=re.sub(r'\\.lto_priv\\.\\d+$','',name)
            if base!=name:grouped.setdefault(base,[]).append((name,value))
        self.lto_aliases={}
        for base,values in grouped.items():
            if base not in self.symbols and len(values)==1:
                self.symbols[base]=values[0][1];self.lto_aliases[base]=values[0][0]
'''+anchor
source=source.replace(anchor,replacement)
anchor="    out['native_profile']=u16(ram+0xc05e)"
assert source.count(anchor)==1
source=source.replace(anchor,"    out.update(fm_active_mask=u16(ram+0xc054),pcm_underruns_mail=u16(ram+0xc05c),late_key_events=u16(ram+0xb038),dsp_jump_count=u16(ram+0xb03c))\n"+anchor)
anchor="    def audio(l,r):samples.extend(struct.pack('<hh',l,r))"
assert source.count(anchor)==1
source=source.replace(anchor,"    def audio(l,r):\n        raw=struct.pack('<hh',l,r);samples.extend(raw);_capture.writeframesraw(raw);_capture_count[0]+=1")
anchor="    def batch(data,n):samples.extend(C.string_at(data,n*4));return n"
assert source.count(anchor)==1
source=source.replace(anchor,"    def batch(data,n):\n        raw=C.string_at(data,n*4);samples.extend(raw);_capture.writeframesraw(raw);_capture_count[0]+=n;return n")
anchor="    start=time.monotonic()"
assert source.count(anchor)==1
source=source.replace(anchor,"""    class Geometry(C.Structure):
        _fields_=[('base_width',C.c_uint),('base_height',C.c_uint),('max_width',C.c_uint),('max_height',C.c_uint),('aspect',C.c_float)]
    class Timing(C.Structure):_fields_=[('fps',C.c_double),('sample_rate',C.c_double)]
    class AVInfo(C.Structure):_fields_=[('geometry',Geometry),('timing',Timing)]
    info=AVInfo();core.retro_get_system_av_info.argtypes=[C.POINTER(AVInfo)];core.retro_get_system_av_info.restype=None
    core.retro_get_system_av_info(C.byref(info));assert info.timing.sample_rate==44100
    report['wav_capture']=dict(sample_rate=44100,channels=2,bits=16,frames=0,fps=info.timing.fps,guest_audio_modified=False)
    report['runtime_lto_aliases']=getattr(elf,'lto_aliases',{}) if elf else {}
"""+anchor)
anchor="        report['counter_trace_samples']=len(counter_trace)"
assert source.count(anchor)==1
source=source.replace(anchor,anchor+"\n        report['wav_capture']['frames']=_capture_count[0]\n        report['wav_capture']['path']=str(_capture_path)")

count=[0]
with target.open('xb') as raw:
    capture=wave.open(raw,'wb');capture.setnchannels(2);capture.setsampwidth(2);capture.setframerate(44100)
    namespace=dict(__file__=str(original),__name__='__main__',_capture=capture,_capture_count=count,_capture_path=target)
    sys.path.insert(0,str(here));sys.argv=[str(original),*rest]
    try:exec(compile(source,str(original),'exec'),namespace)
    finally:capture.close()
with wave.open(str(target),'rb')as recording:
    assert recording.getnframes()==count[0]
metadata=dict(base_harness_sha256=hashlib.sha256(original.read_bytes()).hexdigest(),extension_sha256=hashlib.sha256(Path(__file__).read_bytes()).hexdigest(),identity_adapter_sha256=hashlib.sha256((here/'observer_identity.py').read_bytes()).hexdigest(),wav_sha256=hashlib.sha256(target.read_bytes()).hexdigest(),frames=count[0],sample_rate=44100,seconds=count[0]/44100,guest_audio_modified=False,state_loads=0,guest_memory_edits=False)
target.with_suffix('.json').write_text(json.dumps(metadata,indent=2)+'\n')
print(json.dumps(metadata),flush=True)
