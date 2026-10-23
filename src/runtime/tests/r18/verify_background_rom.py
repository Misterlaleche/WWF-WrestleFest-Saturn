#!/usr/bin/env python3
"""Original $298EC writes must reach the live VDP2 BG map, including raw69."""
import argparse
import ctypes
import hashlib
import json
import random
import subprocess
import tempfile
from pathlib import Path

import machine68k as mus

parser=argparse.ArgumentParser()
parser.add_argument('--out',type=Path,help='optional JSON and software ring previews (no emulator screenshot)')
args=parser.parse_args()
root=Path(__file__).resolve().parents[2]
rom=(root/'audit/runtime18-cd/ROM68K.BIN').read_bytes()
pack=(root/'audit/runtime18-cd/R18RING.BIN').read_bytes()
assert hashlib.sha256(rom).hexdigest()=='265ac89deeb7e708ac6fda724356bfd6af1c9012928a4e551e6e2faa424c2306'
def be16(data,at):return int.from_bytes(data[at:at+2],'big')
tiles=int.from_bytes(pack[16:20],'big');count=be16(pack,10)
dense={be16(pack,tiles+count*128+i*2):i for i in range(count)}
core=next(line[5:].split() for line in (root/'Makefile').read_text().splitlines() if line.startswith('CORE='))
rng=random.Random(0x298ec)
def software_ring(names):
    from PIL import Image
    palettes=[]
    for table in (0x2a98,0x2ab4):
        address=int.from_bytes(rom[table:table+4],'big')
        words=[be16(rom,address+i*2) for i in range(256)]
        palettes.append([((w&15)*17,((w>>4)&15)*17,((w>>8)&15)*17) for w in words])
    pixels=[]
    def tile_pixel(tile,attr,wx,wy):
        x=wx&15;y=wy&15
        if attr&64:x=15-x
        if attr&128:y=15-y
        value=pack[tiles+tile*128+y*8+x//2]
        return ((attr&15)<<4)|((value&15) if x&1 else value>>4)
    for y in range(240):
        for x in range(320):
            wx=480+x;wy=1488+y;tx=wx//16;ty=wy//16
            page=int.from_bytes(pack[32+(ty//16*8+tx//20)*4:36+(ty//16*8+tx//20)*4],'big')
            offset=page+((ty%16)*20+tx%20)*4
            fg=tile_pixel(be16(pack,offset+2),be16(pack,offset),wx,wy)
            pn=names[(ty&31)*32+(tx&31)]
            bg=tile_pixel(pn&1023,(pn>>12)|((pn&0xc00)>>4),wx,wy)
            pixels.append(palettes[1][bg] if bg&15 else palettes[0][fg])
    image=Image.new('RGB',(320,240));image.putdata(pixels);return image

with tempfile.TemporaryDirectory() as tmp:
    library=Path(tmp)/'background.so'
    subprocess.run(['cc','-shared','-fPIC','-std=c11','-Wall','-Wextra','-Werror','-O2','-Iinclude',
                    *core,'src/wf_r18_video.c','src/wf_r18_ring.c',
                    'tests/r18/background_oracle_bridge.c','-o',str(library)],cwd=root,check=True)
    native=ctypes.CDLL(str(library))
    native.background_owner_case.argtypes=[ctypes.c_void_p,ctypes.c_size_t,ctypes.c_void_p,ctypes.c_size_t,
                                          ctypes.c_void_p,ctypes.c_void_p,ctypes.c_void_p]
    blob=ctypes.create_string_buffer(pack)
    previews={}
    if args.out:
        output=(ctypes.c_uint8*0x800)();names=(ctypes.c_uint16*1024)();registers=(ctypes.c_uint32*8)()
        assert native.background_owner_case(blob,len(pack),b'',0,output,names,registers)==0
        previews['before']=software_ring(names)
    sequences=[bytes([1]),bytes([2]),bytes([1,2]),bytes([2,1]),bytes([2,1,2])]
    sequences += [bytes(rng.choice([1,2]) for _ in range(rng.randrange(1,17))) for _ in range(91)]
    for case,sequence in enumerate(sequences):
        registers=[rng.getrandbits(32) for _ in range(8)]
        output=(ctypes.c_uint8*0x800)();names=(ctypes.c_uint16*1024)()
        actual=(ctypes.c_uint32*8)(*registers)
        assert native.background_owner_case(blob,len(pack),sequence,len(sequence),output,names,actual)==0
        machine=mus.Machine(mus.CPUType.M68000,2048)
        cpu=machine.cpu;memory=machine.mem
        memory.w_block(0,rom);memory.w_block(0x1c1808,sequence);memory.w16(0x1c1828,len(sequence))
        for i,value in enumerate(registers):cpu.w_reg(i,value)
        cpu.w_sp(0x1d0000);memory.w32(0x1d0000,0x70000);cpu.w_pc(0x298ec)
        for step in range(100000):
            if cpu.r_pc()==0x70000:break
            cpu.execute(1)
        else:raise AssertionError(('ROM runaway',case,hex(cpu.r_pc())))
        expected=bytes(memory.r_block(0x82000,0x800))
        assert bytes(output)==expected,('RAM',case,sequence)
        assert list(actual)==[cpu.r_reg(i) for i in range(8)],('registers',case)
        assert memory.r16(0x1c1828)==0
        for y in range(3):
            for x in range(6):
                cell=0x18a//2+y*32+x
                value=be16(expected,cell*2)
                want=((value>>12)<<12)|dense[value&4095]
                assert names[cell]==want,('VDP2 lost ROM background write',case,sequence,cell,hex(names[cell]),hex(want))
        if args.out and case==0:previews['after']=software_ring(names)
        machine.cleanup()
    print('PASS 96 untouched-ROM $298EC cases: all BG bytes, D0-D7 and live VDP2 cells; clear/restore ordering')
    if args.out:
        from PIL import Image,ImageChops,ImageDraw
        args.out.mkdir(parents=True,exist_ok=True)
        for name,preview in previews.items():preview.save(args.out/(name+'.png'))
        difference=ImageChops.difference(previews['before'],previews['after'])
        rgb=difference.tobytes()
        changed=sum(rgb[i:i+3]!=b'\0\0\0' for i in range(0,len(rgb),3))
        comparison=Image.new('RGB',(1920,752),(24,24,24));draw=ImageDraw.Draw(comparison)
        for i,name in enumerate(('before','after')):
            draw.text((i*960+12,10),('ANTES: cuerda no se abre' if i==0 else 'DESPUES: copia original $298EC abre la cuerda'),fill='white')
            comparison.paste(previews[name].resize((960,720),Image.Resampling.NEAREST),(i*960,32))
        comparison.save(args.out/'software-comparison.png')
        report={'rom_sha256':hashlib.sha256(rom).hexdigest(),'cases':len(sequences),
                'oracle':'Musashi executes untouched $298EC including $29954 with no hooks',
                'red':'Before bridge: VDP2 cell197 retained base tile despite correct background shadow',
                'green':'All 2048 BG bytes, D0-D7 and18 VDP2 cells equal ROM per case',
                'preview':'Software reconstruction of ring layers from ROM pixels, palettes and native VDP2 names. No sprites, HUD, Mednafen or hardware screenshot.',
                'camera':[480,1488],'rom_bg_rectangle':[592,1632,688,1680],
                'screen_rectangle':[112,144,208,192],'changed_rgb_pixels':changed,
                'difference_bounds':difference.getbbox(),
                'entry_bytes':rom[0x298ec:0x29902].hex(),
                'raw69_enqueue1_bytes_at_19a3c':rom[0x19a3c:0x19a56].hex()}
        (args.out/'background-oracle.json').write_text(json.dumps(report,indent=2)+'\n')
        print('Software ring evidence:',args.out,'changed RGB pixels:',changed)
