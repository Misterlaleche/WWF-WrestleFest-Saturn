"""Actual 68000 + independent MAME DSP. FM slot synthesis is not emulated."""
import argparse,csv,ctypes as C,json,struct,unittest
from pathlib import Path
from test_firmware import segments
BUILD=PACK=LIB=YM=TRACES=None
METRICS={}
HZ=11289600
class MusicFirmware(unittest.TestCase):
 def setUp(self):
  from machine68k import Machine,CPUType
  self.m=Machine(CPUType.M68000,1088);self.mem=self.m.mem;self.cpu=self.m.cpu
  self.symbols=json.loads((BUILD/'symbols.json').read_text())
  self.d=C.CDLL(str(LIB));self.d.dsp_reset();self.d.dsp_output.restype=C.c_int
  self.rb=C.CFUNCTYPE(C.c_uint16,C.c_uint32)(lambda a:self.mem.r16(a&0x7ffff))
  self.wb=C.CFUNCTYPE(None,C.c_uint32,C.c_uint16)(lambda a,v:self.mem.w16(a&0x7ffff,v))
  self.d.dsp_callbacks(self.rb,self.wb);self.cycles=0;self.frames=0;self.regs=bytearray(4096)
  self.keyed=[False]*32;self.ever=0;self.max_late=0;self.pcm_nonzero=0
  def read(a):return int.from_bytes(self.regs[a-0x100000:a-0x100000+2],'big')
  def write(a,v):
   if 0x100700<=a<0x100780:self.d.dsp_coef((a-0x100700)//2,v)
   elif 0x100780<=a<0x1007c0:self.d.dsp_madrs((a-0x100780)//2,v)
   elif 0x100800<=a<0x100c00:
    self.d.dsp_program((a-0x100800)//2,v)
    if a==0x100bf0:self.d.dsp_start()
   if a<0x100400 and a%32==0 and v&0x1000:
    v&=~0x1000
    self.regs[a-0x100000:a-0x100000+2]=v.to_bytes(2,'big')
    for slot in range(32):
     self.keyed[slot]=bool(read(0x100000+slot*32)&0x800)
     if self.keyed[slot]:self.ever|=1<<slot
   self.regs[a-0x100000:a-0x100000+2]=v.to_bytes(2,'big')
  self.mmio_write=write
  self.mem.set_special_range_read_funcs(0x100000,1,None,read,None)
  self.mem.set_special_range_write_funcs(0x100000,1,None,write,None)
  self.mem.w_block(0,b'\xa5'*0x80000)
  for a,b,n in segments(BUILD/'SCSP.ELF'):self.mem.w_block(a,b)
  self.mem.w_block(0xc040,b'\0'*32);self.cpu.pulse_reset()
  for _ in range(40000):
   self.step()
   if self.mem.r16(0xc046)!=0 and self.mem.r16(0xc046)!=1:break
  self.assertEqual(self.mem.r16(0xc046),2)
  self.assertEqual(self.mem.r16(0xc05e),0x4d46)
  self.mem.w_block(0x14000,PACK.read_bytes());self.assertEqual(self.command(8),0)
 def tearDown(self):
  self.d.dsp_callbacks(None,None);self.m.cleanup()
 def step(self):
  self.cycles+=self.cpu.execute(256).cycles
  while self.frames*256<self.cycles:
   self.d.dsp_step();self.frames+=1
   if any(self.d.dsp_output(v) for v in range(4)):self.pcm_nonzero+=1
  if self.mem.r32(self.symbols['wf_sfx_engine']+32)==1 and not getattr(self,'dumped',False):
   self.dumped=True;at=self.symbols['wf_sfx_engine']
   print('underrun debug',self.frames,'frames,lastpoll,clock=',self.mem.r32(at+36),self.mem.r32(self.symbols['last_poll']),self.mem.r32(self.symbols['clock_frames']),[(struct.unpack('>6I',self.mem.r_block(at+40+v*44,24))) for v in range(4)],flush=True)
 def command(self,c,a=0):
  n=(self.mem.r16(0xc048)+1)&65535
  self.mem.w16(0xc04c,c);self.mem.w16(0xc04e,a);self.mem.w16(0xc048,n)
  for _ in range(20000):
   self.step()
   if self.mem.r16(0xc04a)==n:return self.mem.r16(0xc050)
  self.fail('mailbox timeout')
 def feed(self,events,origin,generation=1):
  index=0;end=self.frames+44100*3;last=end
  frame_base=self.frames;clock_base=self.mem.r16(0xb010);origin=frame_base+512
  direct=[];direct_index=0
  self.mem.w16(0xb024,generation)
  while self.frames<end:
   while index<len(events):
    time,kind,reg,value=events[index];when=origin+time
    if when>self.frames+512:break
    if kind==0x80:
     direct.append((when,reg,value));index+=1;continue
    producer=self.mem.r16(0xb020);consumer=self.mem.r16(0xb022)
    if (producer-consumer)&65535>=256:break
    self.mem.w_block(0xb100+(producer&255)*8,struct.pack('>4H',(clock_base+when-frame_base)&65535,kind*256+reg,value,generation))
    self.mem.w16(0xb020,(producer+1)&65535);index+=1;last=max(last,when)
   while direct_index<len(direct) and direct[direct_index][0]<=self.frames:
    when,reg,value=direct[direct_index];self.mmio_write(0x100000+reg*2,value);direct_index+=1
   consumer=self.mem.r16(0xb022)
   if consumer!=self.mem.r16(0xb020):
    t=self.mem.r16(0xb100+(consumer&255)*8);late=(self.mem.r16(0xb010)-t)&65535
    if late<32768:self.max_late=max(self.max_late,late)
   self.step()
   self.assertEqual(self.mem.r16(0xc046),2,f'fault generation={generation} pc={self.cpu.r_pc():x} frame={self.frames} origin={origin} late={self.max_late} producer={self.mem.r16(0xb020)} consumer={self.mem.r16(0xb022)} ymfault={self.mem.r8(self.symbols["wf_ym_engine"]+1050)} dsp={self.mem.r16(0xb010)} underruns={self.mem.r32(self.symbols["wf_sfx_engine"]+32)} sfx_tail={self.mem.r_block(self.symbols["wf_sfx_engine"]+248,16).hex()}')
  self.assertLess(self.max_late,2205)
  return index
 def test_full_music_drums_and_three_effects(self):
  from machine68k import Register
  summary=json.loads((TRACES/'capture-summary.json').read_text())
  for cmd in range(1,19):
   events=[];expected=0;ym=C.CDLL(str(YM));time=0
   def converted(address,value,user):
    events.append((time,0x80,(address-0x100000)//2,value));return True
   output=C.CFUNCTYPE(C.c_bool,C.c_uint32,C.c_uint16,C.c_void_p)(converted)
   ym.ym_reset(output);events.clear();name=f'cmd{cmd:02x}';base=summary['captures'][name]['origin_cycle']
   with (TRACES/(name+'.csv')).open() as f:
    for e in csv.DictReader(f):
     if e['kind'] not in ('ym_write','oki_write','bank_write'):continue
     if e['phase']=='boot':continue
     else:time=(int(e['cycle'])-base)*44100//3579545
     if time>44100*3:continue
     kind={'ym_write':0,'oki_write':1,'bank_write':2}[e['kind']]
     if kind==0:
      self.assertTrue(ym.ym_write(int(e['reg']),int(e['data'])))
      if int(e['reg'])==8 and time<44100*3-512:expected|=((int(e['data'])>>3)&15)<<(4*(int(e['data'])&7))
     else:events.append((time,kind,int(e['reg']),int(e['data'])))
   for v in range(3):self.assertEqual(self.command(11,(v<<12)|(1<<11)|(20<<4)|3),0)
   self.ever=0;count=self.feed(events,self.mem.r16(0xb010)+512,cmd)
   self.assertEqual(self.ever,expected,name)
   self.assertGreater(self.pcm_nonzero,100)
   self.assertEqual(self.mem.r16(0xc05c),0)
   self.assertTrue(0x12800<=self.cpu.r_reg(Register.A7)<=0x14000)
   METRICS[name]=dict(events=count,max_lateness_fs=self.max_late,operator_mask=f"{self.ever:08x}",expected_operator_mask=f"{expected:08x}",pcm_nonzero_frames=self.pcm_nonzero)
  self.assertEqual(self.command(7,0),0)
  self.assertTrue(any(self.keyed),'effect stop must not kill newer FM')
  self.assertEqual(self.command(10)&7,0)
  self.mem.w16(0xb024,100);self.feed([],self.frames,100)
  self.assertFalse(any(self.keyed))
if __name__=='__main__':
 p=argparse.ArgumentParser()
 for n in ['build','pack','oracle','ym','traces']:p.add_argument('--'+n,type=Path,required=True)
 p.add_argument('--report',type=Path);a,left=p.parse_known_args();BUILD=a.build;PACK=a.pack;LIB=a.oracle.resolve();YM=a.ym.resolve();TRACES=a.traces
 result=unittest.main(argv=['test_music_firmware',*left],exit=False).result
 if a.report:a.report.write_text(json.dumps(dict(passed=result.wasSuccessful(),fm_waveform_emulated=False,**METRICS),indent=2)+'\n')
 raise SystemExit(0 if result.wasSuccessful() else 1)
