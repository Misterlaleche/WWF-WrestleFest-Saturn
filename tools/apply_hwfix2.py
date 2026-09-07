#!/usr/bin/env python3
from pathlib import Path
import struct, sys

root=Path(sys.argv[1] if len(sys.argv)>1 else '.')
p=root/'main.c'
s=p.read_text()

needle='static void selection_cache_free(void){for(unsigned i=0;i<2;i++){free(selection_rle[i]);selection_rle[i]=NULL;selection_rle_size[i]=0;}}\n'
if needle not in s: raise SystemExit('HWFIX2: selection cache anchor missing')
if 'static void match_frontend_cache_release(void)' not in s:
    s=s.replace(needle,needle+'''static void match_frontend_cache_release(void){
 selection_cache_free();
 free(mode_rle_cache);mode_rle_cache=NULL;mode_rle_size=0;
 free(entry_rle_cache);entry_rle_cache=NULL;entry_rle_size=0;
 free(title_delta);title_delta=NULL;
}
''',1)

obj_decl='static roster_pack_t object_pack;\nstatic uint8_t *object_pack_data;\n'
if obj_decl not in s: raise SystemExit('HWFIX2: object declaration anchor missing')
if 'object_pack_storage[' not in s:
    s=s.replace(obj_decl,obj_decl+'''static uint8_t object_pack_storage[OBJECT_PACK_ALIGNED_BYTES] __attribute__((aligned(32)));
static uint8_t ring_transition_pack_storage[RING_TRANSITION_PACK_ALIGNED_BYTES] __attribute__((aligned(32)));
''',1)

old='''static bool ring_transition_pack_load(void){
 if(ring_transition_pack_data)return true;
 uint8_t *dst=malloc(RING_TRANSITION_PACK_ALIGNED_BYTES);if(!dst){check_error=342;return false;}
 unsigned bytes=cd_read("RING14.PAK",dst,RING_TRANSITION_PACK_ALIGNED_BYTES);
 if(bytes!=RING_TRANSITION_PACK_BYTES||hash_ram(dst,RING_TRANSITION_PACK_BYTES)!=RING_TRANSITION_PACK_HASH){free(dst);check_error=343;return false;}
 roster_pack_t pack;if(!roster_pack_open(&pack,dst,RING_TRANSITION_PACK_BYTES,RING_TRANSITION_PACK_FRAMES)){free(dst);check_error=344;return false;}
 ring_transition_pack=pack;ring_transition_pack_data=dst;return true;
}
'''
new='''static bool ring_transition_pack_load(void){
 if(ring_transition_pack_data)return true;
 uint8_t *dst=ring_transition_pack_storage;
 unsigned bytes=cd_read("RING14.PAK",dst,RING_TRANSITION_PACK_ALIGNED_BYTES);
 if(bytes!=RING_TRANSITION_PACK_BYTES||hash_ram(dst,RING_TRANSITION_PACK_BYTES)!=RING_TRANSITION_PACK_HASH){check_error=343;return false;}
 roster_pack_t pack;if(!roster_pack_open(&pack,dst,RING_TRANSITION_PACK_BYTES,RING_TRANSITION_PACK_FRAMES)){check_error=344;return false;}
 ring_transition_pack=pack;ring_transition_pack_data=dst;return true;
}
'''
if old not in s: raise SystemExit('HWFIX2: ring loader anchor missing')
s=s.replace(old,new,1)

old='''static bool object_pack_load(void){
 if(object_pack_data)return true;
 uint8_t *dst=malloc(OBJECT_PACK_ALIGNED_BYTES);if(!dst){check_error=339;return false;}
 unsigned bytes=cd_read("OBJ15.PAK",dst,OBJECT_PACK_ALIGNED_BYTES);
 if(bytes!=OBJECT_PACK_BYTES||hash_ram(dst,OBJECT_PACK_BYTES)!=OBJECT_PACK_HASH){free(dst);check_error=340;return false;}
 roster_pack_t pack;if(!roster_pack_open(&pack,dst,OBJECT_PACK_BYTES,OBJECT_PACK_FRAMES)){free(dst);check_error=341;return false;}
 object_pack=pack;object_pack_data=dst;return true;
}
'''
new='''static bool object_pack_load(void){
 if(object_pack_data)return true;
 uint8_t *dst=object_pack_storage;
 unsigned bytes=cd_read("OBJ15.PAK",dst,OBJECT_PACK_ALIGNED_BYTES);
 if(bytes!=OBJECT_PACK_BYTES||hash_ram(dst,OBJECT_PACK_BYTES)!=OBJECT_PACK_HASH){check_error=340;return false;}
 roster_pack_t pack;if(!roster_pack_open(&pack,dst,OBJECT_PACK_BYTES,OBJECT_PACK_FRAMES)){check_error=341;return false;}
 object_pack=pack;object_pack_data=dst;return true;
}
'''
if old not in s: raise SystemExit('HWFIX2: object loader anchor missing')
s=s.replace(old,new,1)

meta=[]
for ch in range(12):
    data=(root/f'cd/C{ch:02d}.PAK').read_bytes()
    if len(data)<32: raise SystemExit(f'C{ch:02d}.PAK too small')
    magic,frames,records,tiles,texoff,_unused,block_tiles,blocks=struct.unpack('>8I',data[:32])
    meta.append((magic,frames,records,tiles,texoff,blocks,block_tiles))
cases=[]
for ch,(magic,frames,records,tiles,texoff,blocks,block_tiles) in enumerate(meta):
    cases.append(f' case {ch}: *out=(roster_pack_t){{.data=data,.size=roster_bytes[{ch}],.magic={magic}U,.frames={frames}U,.records={records}U,.tiles={tiles}U,.texture_offset={texoff}U,.blocks={blocks}U,.block_tiles={block_tiles}U}}; return true;')
binder='''static bool roster_pack_bind_verified(roster_pack_t *out,const uint8_t *data,unsigned ch){
 if(!out||!data||ch>=ROSTER_COUNT)return false;
 switch(ch){
'''+"\n".join(cases)+'''\n default:return false;\n }\n}\n'''
anchor='static void roster_load_ids(const unsigned *ids,unsigned count){\n'
if anchor not in s: raise SystemExit('HWFIX2: roster loader anchor missing')
if 'static bool roster_pack_bind_verified' not in s: s=s.replace(anchor,binder+anchor,1)
if anchor+' match_frontend_cache_release();\n' not in s: s=s.replace(anchor,anchor+' match_frontend_cache_release();\n',1)
oldcall='if(!roster_pack_open(&character_packs[ch],dst,roster_bytes[ch],ROSTER_FRAMES)){check_error=309;return;}'
if oldcall not in s: raise SystemExit('HWFIX2: roster open call missing')
s=s.replace(oldcall,'if(!roster_pack_bind_verified(&character_packs[ch],dst,ch)){check_error=346;return;}',1)

entry='  transfer((void *)(VDP2_CRAM_MODE_1_OFFSET(0,0,0)+1792*2),asset_entryuipal,64);\n'
if entry in s and entry+'  free(entry_rle_cache);entry_rle_cache=NULL;entry_rle_size=0;\n' not in s:
    s=s.replace(entry,entry+'  free(entry_rle_cache);entry_rle_cache=NULL;entry_rle_size=0;\n',1)

s=s.replace('WWF SATURN TEST254: DATA ERROR','WWF SATURN TEST255 HWFIX2: DATA ERROR')
s=s.replace('TEST254 V0.269 - START: RESUME','TEST255 V0.270 HWFIX2 - START: RESUME')
p.write_text(s)
print('HWFIX2 patch applied')
