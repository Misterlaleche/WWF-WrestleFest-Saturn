# Read-only exact-build VDP1/VDP2 payload verification. Requires --graphics-layout.
import io,json,re,sys,contextlib,struct,hashlib
from pathlib import Path
# Reuse the existing read-only exact-executable locator; add only host address.
observer=Path(__file__).with_name('observe-mednafen-runtime18.py')
source=observer.read_text().replace('candidates.append(values)',"values['_host_ram_base']=base; candidates.append(values)")
ns={'__name__':'__main__'};stdout=io.StringIO()
with contextlib.redirect_stdout(stdout):exec(compile(source,str(observer),'exec'),ns)
state=json.loads(stdout.getvalue());host=state.pop('_host_ram_base');program=ns['program'];mapping=ns['mapping'];pid=int(sys.argv[1])
def symbol(n):
 m=re.search(r'\.bss\.'+n+r'(?:\.lto_priv\.\d+)?\s+0x([0-9a-f]+)\s',mapping);assert m,n;return int(m[1],16)
def hostaddr(a):return host+a-0x06000000
def u32(b):return int.from_bytes(b[1::-1]+b[3:1:-1],'big')
with open(f'/proc/{pid}/mem','rb',buffering=0) as mem:
 def get(a,n):mem.seek(a);return mem.read(n)
 count=u32(get(hostaddr(symbol('g_cmd_index')),4));assert 3<=count<=516,count
 payload=get(hostaddr(symbol('g_cmdts')),count*32)
 commands=[]
 for i in range(count):
  b=payload[i*32:i*32+32];w=[int.from_bytes(b[j:j+2],'little') for j in range(0,32,2)]
  commands.append({'index':i,'control':w[0],'draw_mode':w[2],'color':w[3],'texture_byte_offset':w[4]*8,'size':w[5],'x':w[6]-65536 if w[6]&32768 else w[6],'y':w[7]-65536 if w[7]&32768 else w[7]})
 assert commands[0]['control']&15==9 and commands[1]['control']&15==10
 assert commands[-1]['control']&0x8000
 sprites=commands[2:-1]
 assert all((c['control']&15)==0 and c['size']>>8==2 and 1<=c['size']&255<=16 and c['draw_mode']&0x80 for c in sprites)
 assert all(0x4b580<=c['texture_byte_offset']<0x5b580 for c in sprites)
 tile_count=(max(c['texture_byte_offset'] for c in sprites)-0x4b580)//128+1
 stage=get(hostaddr(symbol('dma_stage')),tile_count*128)
 matches=[]
 for line in Path(f'/proc/{pid}/maps').read_text().splitlines():
  parts=line.split()
  if not parts[1].startswith('rw') or len(parts)>5 and parts[5] not in ('[heap]',):continue
  lo,hi=(int(x,16) for x in parts[0].split('-'))
  if hi-lo>128*1024*1024:continue
  try:data=get(lo,hi-lo)
  except OSError:continue
  at=data.find(payload)
  while at>=0:
   for skip in (0,32):
    candidate=lo+at-skip
    try:pix=get(candidate+0x4b580,len(stage))
    except OSError:continue
    if pix==stage:matches.append({'command_offset':skip,'VDP1_VRAM_host_base':hex(candidate),'command_bytes_equal':len(payload),'texture_bytes_equal':len(stage)})
   at=data.find(payload,at+2)
 assert matches,'No command/texture-layout match for actual VDP1 VRAM'
 # Independently reconstruct the HUD names from the live original FG0 RAM.
 offsets=struct.unpack('>12I',ns['a'].graphics_layout.read_bytes())
 def unswap(b):
  out=bytearray(b);out[0::2],out[1::2]=b[1::2],b[0::2];return bytes(out)
 fg=unswap(get(hostaddr(symbol('video')+offsets[10]),8192))
 names=[(fg[i+3]<<8)|fg[i+1] for i in range(0,8192,4)]
 wanted_names=struct.pack('<4096H',*(names+names))
 font=Path('/repo/src/runtime/audit/runtime18-cd/FONT8.BIN').read_bytes()
 wanted_font=unswap(font);vdp2_matches=[]
 # The complete font at+60000 and complete live map at+24000 must be in
 # the same512KB VRAM image. A CD buffer/font signature alone cannot pass.
 for line in Path(f'/proc/{pid}/maps').read_text().splitlines():
  parts=line.split()
  if not parts[1].startswith('rw') or len(parts)>5 and parts[5] not in ('[heap]',):continue
  lo,hi=(int(x,16) for x in parts[0].split('-'))
  if hi-lo>128*1024*1024:continue
  try:data=get(lo,hi-lo)
  except OSError:continue
  at=data.find(wanted_font)
  while at>=0:
   candidate=lo+at-0x60000
   try:actual_names=get(candidate+0x24000,len(wanted_names))
   except OSError:actual_names=b''
   if actual_names==wanted_names:vdp2_matches.append(hex(candidate))
   at=data.find(wanted_font,at+2)
 assert vdp2_matches,'No complete native font+FG0 map match in actual VDP2 VRAM'
 # Verify both actual character banks after a scene reload, not only the
 # borrowed CD buffer. Saturn 2x2 characters store four 8x8 quadrants.
 ring_matches=[]
 for name in ('R18RING.BIN','R18MAP2.BIN'):
  asset=(Path('/repo/src/runtime/audit/runtime18-cd')/name).read_bytes()
  tiles=int.from_bytes(asset[10:12],'big');offset=int.from_bytes(asset[16:20],'big')
  characters=bytearray(tiles*128)
  for t in range(tiles):
   for q in range(4):
    for y in range(8):
     at=offset+t*128+((q//2)*8+y)*8+(q%2)*4
     characters[t*128+q*32+y*4:t*128+q*32+y*4+4]=asset[at:at+4]
  wanted=unswap(characters)
  for candidate in vdp2_matches:
   base=int(candidate,16)
   if get(base,len(wanted))==wanted and get(base+0x40000,len(wanted))==wanted:
    ring_matches.append({'scene_asset':name,'VDP2_VRAM_host_base':candidate,'bytes_equal_per_character_bank':len(wanted),'character_tiles':tiles,'characters_sha256':hashlib.sha256(characters).hexdigest()})
 assert ring_matches,'No scene character payload in both real VDP2 banks0/2'
 state['ring_character_banks_matches']=ring_matches
 state.update({'VDP2_VRAM_layout_matches':vdp2_matches,'HUD_font_bytes_equal':len(font),'HUD_map_bytes_equal':len(wanted_names),'HUD_font_sha256':hashlib.sha256(font).hexdigest(),'HUD_nonzero_cells':sum(v!=0 for v in names),'HUD_map_sha256':hashlib.sha256(wanted_names).hexdigest()})
 state.update({'VDP1_commands_sha256':hashlib.sha256(payload).hexdigest(),'VDP1_textures_sha256':hashlib.sha256(stage).hexdigest(),'VDP1_command_count':count,'normal_sprite_commands':len(sprites),'unique_texture_tiles':len({(c['texture_byte_offset']-0x4b580)//128 for c in sprites}),'texture_slot_span':tile_count,'submitted_sprite_rows':sum(c['size']&255 for c in sprites),'uncropped_rows_for_submitted_sprites':16*len(sprites),'VDP1_VRAM_layout_matches':matches,'command_examples':commands[:5]+commands[-1:],'scope':'Read-only exact SH-2 build in Mednafen; command bytes and corresponding DMA texture window match a live VRAM image. No physical Saturn or arcade-framebuffer claim.'})
 print(json.dumps(state,indent=2))
