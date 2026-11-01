"""Execute the actual 68000 ELF with a synthetic SCSP cursor, not an audio emulator."""
import argparse,json,struct,unittest
from pathlib import Path
from test_firmware import segments
BUILD=PACK=AUDIT=PCM=None
METRICS={}
HZ=11289600
class NativeSFX(unittest.TestCase):
    def setUp(self):
        from machine68k import Machine,CPUType
        self.m=Machine(CPUType.M68000,1088);self.mem=self.m.mem;self.cpu=self.m.cpu
        self.cycles=0;self.last_read=0;self.max_gap=0;self.gaps=[];self.track=False
        self.regs=bytearray(4096)
        self.keyed=[False]*32;self.started={};self.stopped={}
        def read16(a):
            if a==0x100408:
                if self.track:self.max_gap=max(self.max_gap,self.cycles-self.last_read)
                self.last_read=self.cycles
                pos=(self.cycles*44100*2047//(HZ*16))%61440
                return (30<<11)|((pos//4096)<<7)
            return int.from_bytes(self.regs[a-0x100000:a-0x100000+2],'big')
        def write16(a,v):
            if a==0x100408:self.assertEqual(v,30<<11)
            execute=a<0x100400 and a%32==0 and v&0x1000
            if a<0x100400 and a%32==0 and v&0x1000:v&=~0x1000
            self.regs[a-0x100000:a-0x100000+2]=v.to_bytes(2,'big')
            if execute:
                for slot in range(32):
                    on=bool(int.from_bytes(self.regs[slot*32:slot*32+2],'big')&0x800)
                    if on and not self.keyed[slot]:self.started[slot]=self.cycles
                    if not on and self.keyed[slot]:self.stopped[slot]=self.cycles
                    self.keyed[slot]=on
        self.read16=read16
        self.mem.set_special_range_read_funcs(0x100000,1,None,read16,None)
        self.mem.set_special_range_write_funcs(0x100000,1,None,write16,None)
        self.mem.w_block(0,b'\xa5'*0x80000)
        for a,b,n in segments(BUILD/'SCSP.ELF'):self.mem.w_block(a,b)
        self.mem.w_block(0xc040,b'\0'*32)
        self.cpu.pulse_reset()
        for _ in range(2000):
            self.step()
            if self.mem.r16(0xc046)==2:break
        self.assertEqual(self.mem.r16(0xc046),2)
        self.mem.w_block(0x14000,PACK.read_bytes())
        self.assertEqual(self.command(8)[0],0)
        self.last_read=self.cycles;self.track=True;self.max_gap=0
    def tearDown(self):self.m.cleanup()
    def step(self):self.cycles+=self.cpu.execute(1000).cycles
    def run_for(self,n):
        end=self.cycles+n
        while self.cycles<end:
            self.step()
            self.assertEqual(self.mem.r16(0xc046),2,'firmware fault/underrun')
    def command(self,cmd,arg=0):
        seq=(self.mem.r16(0xc048)+1)&65535
        self.mem.w16(0xc04c,cmd);self.mem.w16(0xc04e,arg);self.mem.w16(0xc048,seq)
        for _ in range(1200):
            self.step()
            if self.mem.r16(0xc04a)==seq:return self.mem.r16(0xc050),self.mem.r16(0xc052)
        self.fail('mailbox timeout')
    def test_profile_ram_fm_and_all_98_commands(self):
        from machine68k import Register
        self.assertEqual(self.mem.r16(0xc044),3)
        self.assertEqual(self.mem.r16(0xc05e),0x53f1)
        self.assertTrue(0x12000<=self.cpu.r_reg(Register.A7)<=0x14000)
        self.assertEqual(self.mem.r_block(0x10000,6144),(BUILD/'sine-three-cycles.pcm').read_bytes())
        self.assertEqual(self.mem.r_block(0x14000,PACK.stat().st_size),PACK.read_bytes())
        self.assertEqual(self.command(1)[0],0)
        for c in range(0x20,0x82):
            self.assertIn(self.command(7,0x3100|c)[0],(0,4));self.run_for(HZ//80)
        self.assertEqual(self.command(7,0x3101)[0],3) # music remains explicit
        self.assertEqual(self.command(3)[0],3) # incompatible R04 ring disabled
        self.assertEqual(self.command(7,0x3100)[0],0)
        self.assertEqual(self.command(10),(0,0x100))
        self.assertEqual(self.mem.r16(0xc05c),0)
        self.assertLess(self.max_gap,HZ*10//1000)
        self.gaps.append(self.max_gap)
        METRICS['all_commands_max_poll_cycles']=self.max_gap
    def test_four_long_voices_refill_under_68000_cycle_budget(self):
        for v in range(4):self.assertEqual(self.command(11,(v<<12)|(1<<11)|(20<<4)|3)[0],0)
        self.run_for(HZ*2)
        self.assertEqual(self.command(10),(0,0x10f))
        self.assertEqual(self.mem.r16(0xc05c),0)
        self.assertLess(self.max_gap,HZ*10//1000)
        print('Four voices: max poll gap cycles=',self.max_gap,'us=',round(self.max_gap*1e6/HZ))
        METRICS['four_voices_max_poll_cycles']=self.max_gap
    def test_physical_end_is_not_early_at_arbitrary_global_phase(self):
        self.run_for(33123)
        self.assertEqual(self.command(11,1<<4)[0],0)
        start=self.started[4]
        for _ in range(5000):
            self.step()
            self.assertEqual(self.mem.r16(0xc046),2)
            if 4 in self.stopped:break
        self.assertIn(4,self.stopped)
        minimum=1802*8192*HZ//(1486*44100)
        self.assertGreaterEqual(self.stopped[4]-start,minimum-1000)
        self.assertEqual(self.read16(0x10041e),0)
        self.assertEqual(self.read16(0x10042a),0)
    def test_actual_assembly_all_114_entries_exact_pcm(self):
        from machine68k import Register
        syms=json.loads((BUILD/'symbols.json').read_text());gold=PCM.read_bytes();pack=PACK.read_bytes()
        count=0
        for f in json.loads(AUDIT.read_text())['samples']:
            entry=32+(f['bank']*74+f['sample_id'])*8
            offset,n,ticks=struct.unpack_from('>IHH',pack,entry)
            self.mem.w_block(0xc300,struct.pack('>IIhhBB',offset,n*2,0,0,0,0))
            at=0
            while at<f['frames']:
                frames=min((1,31,32,65,256)[count%5],f['frames']-at);count+=1
                self.mem.w16(0xb000,0x60fe)
                self.mem.w_block(0x13f00,struct.pack('>IIIII',0xb000,0xc300,0x14000,0x3000,frames))
                self.cpu.w_reg(Register.A7,0x13f00);self.cpu.w_pc(syms['wf_sfx_decode_block'])
                for _ in range(100):
                    self.cpu.execute(1000)
                    if self.cpu.r_pc()==0xb000:break
                self.assertEqual(self.cpu.r_pc(),0xb000)
                self.assertEqual(self.mem.r_block(0x3000,frames*2),gold[f['offset']+at*2:f['offset']+(at+frames)*2],f"bank {f['bank']} sample {f['sample_id']} frame {at}")
                at+=frames
            self.assertEqual(self.mem.r32(0xc304),0)
        METRICS['assembly_pcm_entries_exact']=114
if __name__=='__main__':
    p=argparse.ArgumentParser()
    for name in ['build','pack','audit','pcm']:p.add_argument('--'+name,type=Path,required=True)
    p.add_argument('--report',type=Path)
    a,left=p.parse_known_args();BUILD=a.build;PACK=a.pack;AUDIT=a.audit;PCM=a.pcm
    result=unittest.main(argv=['test_sfx_firmware',*left],exit=False).result
    if a.report:a.report.write_text(json.dumps(dict(tests=result.testsRun,passed=result.wasSuccessful(),clock_hz=HZ,scsp_audio_emulated=False,**METRICS),indent=2)+'\n')
    raise SystemExit(0 if result.wasSuccessful() else 1)
