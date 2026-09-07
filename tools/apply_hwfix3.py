#!/usr/bin/env python3
from pathlib import Path
import re, struct, sys
root=Path(sys.argv[1] if len(sys.argv)>1 else '.')
p=root/'main.c'
s=p.read_text()
needle='static void selection_cache_free(void){for(unsigned i=0;i<2;i++){free(selection_rle[i]);selection_rle[i]=NULL;selection_rle_size[i]=0;}}\n'
if needle not in s: raise SystemExit('HWFIX3: selection cache anchor missing')
if 'static void match_frontend_cache_release(void)' not in s:
    s=s.replace(needle,needle+'''static void match_frontend_cache_release(void){\n selection_cache_free();\n free(mode_rle_cache);mode_rle_cache=NULL;mode_rle_size=0;\n free(entry_rle_cache);entry_rle_cache=NULL;entry_rle_size=0;\n free(title_delta);title_delta=NULL;\n}\n''',1)
obj_decl='static roster_pack_t object_pack;\nstatic uint8_t *object_pack_data;\n'
if obj_decl not in s: raise SystemExit('HWFIX3: object declaration anchor missing')
if 'match_pack_arena_raw' not in s:
    s=s.replace(obj_decl,obj_decl+'''static uint8_t *match_pack_arena_raw,*match_pack_arena;\n''',1)
old='''static bool ring_transition_pack_load(void){\n if(ring_transition_pack_data)return true;\n uint8_t *dst=malloc(RING_TRANSITION_PACK_ALIGNED_BYTES);if(!dst){check_error=342;return false;}\n unsigned bytes=cd_read("RING14.PAK",dst,RING_TRANSITION_PACK_ALIGNED_BYTES);\n if(bytes!=RING_TRANSITION_PACK_BYTES||hash_ram(dst,RING_TRANSITION_PACK_BYTES)!=RING_TRANSITION_PACK_HASH){free(dst);check_error=343;return false;}\n roster_pack_t pack;if(!roster_pack_open(&pack,dst,RING_TRANSITION_PACK_BYTES,RING_TRANSITION_PACK_FRAMES)){free(dst);check_error=344;return false;}\n ring_transition_pack=pack;ring_transition_pack_data=dst;return true;\n}\n'''
new='''static bool ring_transition_pack_load(void){\n if(ring_transition_pack_data)return true;\n if(!match_pack_arena){check_error=347;return false;}\n uint8_t *dst=match_pack_arena+OBJECT_PACK_ALIGNED_BYTES;\n unsigned bytes=cd_read("RING14.PAK",dst,RING_TRANSITION_PACK_ALIGNED_BYTES);\n if(bytes!=RING_TRANSITION_PACK_BYTES||hash_ram(dst,RING_TRANSITION_PACK_BYTES)!=RING_TRANSITION_PACK_HASH){check_error=343;return false;}\n roster_pack_t pack;if(!roster_pack_open(&pack,dst,RING_TRANSITION_PACK_BYTES,RING_TRANSITION_PACK_FRAMES)){check_error=344;return false;}\n ring_transition_pack=pack;ring_transition_pack_data=dst;return true;\n}\n'''
if old not in s: raise SystemExit('HWFIX3: ring loader anchor missing')
s=s.replace(old,new,1)
old='''static bool object_pack_load(void){\n if(object_pack_data)return true;\n uint8_t *dst=malloc(OBJECT_PACK_ALIGNED_BYTES);if(!dst){check_error=339;return false;}\n unsigned bytes=cd_read("OBJ15.PAK",dst,OBJECT_PACK_ALIGNED_BYTES);\n if(bytes!=OBJECT_PACK_BYTES||hash_ram(dst,OBJECT_PACK_BYTES)!=OBJECT_PACK_HASH){free(dst);check_error=340;return false;}\n roster_pack_t pack;if(!roster_pack_open(&pack,dst,OBJECT_PACK_BYTES,OBJECT_PACK_FRAMES)){free(dst);check_error=341;return false;}\n object_pack=pack;object_pack_data=dst;return true;\n}\n'''
new='''static bool object_pack_load(void){\n if(object_pack_data)return true;\n if(!match_pack_arena){check_error=347;return false;}\n uint8_t *dst=match_pack_arena;\n unsigned bytes=cd_read("OBJ15.PAK",dst,OBJECT_PACK_ALIGNED_BYTES);\n if(bytes!=OBJECT_PACK_BYTES||hash_ram(dst,OBJECT_PACK_BYTES)!=OBJECT_PACK_HASH){check_error=340;return false;}\n roster_pack_t pack;if(!roster_pack_open(&pack,dst,OBJECT_PACK_BYTES,OBJECT_PACK_FRAMES)){check_error=341;return false;}\n object_pack=pack;object_pack_data=dst;return true;\n}\n'''
if old not in s: raise SystemExit('HWFIX3: object loader anchor missing')
s=s.replace(old,new,1)
roster_text=(root/'roster_data.h').read_text();m=re.search(r'#define\s+ROSTER_COUNT\s+(\d+)',roster_text)
if not m: raise SystemExit('HWFIX3: ROSTER_COUNT not found')
roster_count=int(m.group(1));meta=[]
for ch in range(roster_count):
    path=root/f'cd/C{ch:02d}.PAK'
    if not path.exists(): raise SystemExit(f'HWFIX3: missing {path.name}')
    data=path.read_bytes(); magic,frames,records,tiles,texoff,_unused,block_tiles,blocks=struct.unpack('>8I',data[:32]);meta.append((magic,frames,records,tiles,texoff,blocks,block_tiles))
cases=[]
for ch,(magic,frames,records,tiles,texoff,blocks,block_tiles) in enumerate(meta):
    cases.append(f' case {ch}: *out=(roster_pack_t){{.data=data,.size=roster_bytes[{ch}],.magic={magic}U,.frames={frames}U,.records={records}U,.tiles={tiles}U,.texture_offset={texoff}U,.blocks={blocks}U,.block_tiles={block_tiles}U}}; return true;')
binder='''static bool roster_pack_bind_verified(roster_pack_t *out,const uint8_t *data,unsigned ch){\n if(!out||!data||ch>=ROSTER_COUNT)return false;\n switch(ch){\n'''+"\n".join(cases)+'''\n default:return false;\n }\n}\n'''
anchor='static void roster_load_ids(const unsigned *ids,unsigned count){\n'
if anchor not in s: raise SystemExit('HWFIX3: roster loader anchor missing')
if 'static bool roster_pack_bind_verified' not in s:s=s.replace(anchor,binder+anchor,1)
if anchor+' match_frontend_cache_release();\n' not in s:s=s.replace(anchor,anchor+' match_frontend_cache_release();\n',1)
oldcall='if(!roster_pack_open(&character_packs[ch],dst,roster_bytes[ch],ROSTER_FRAMES)){check_error=309;return;}'
if oldcall not in s: raise SystemExit('HWFIX3: roster open call missing')
s=s.replace(oldcall,'if(!roster_pack_bind_verified(&character_packs[ch],dst,ch)){check_error=346;return;}',1)
entry='  transfer((void *)(VDP2_CRAM_MODE_1_OFFSET(0,0,0)+1792*2),asset_entryuipal,64);\n'
if entry in s and entry+'  free(entry_rle_cache);entry_rle_cache=NULL;entry_rle_size=0;\n' not in s:s=s.replace(entry,entry+'  free(entry_rle_cache);entry_rle_cache=NULL;entry_rle_size=0;\n',1)
main_anchor=' cdfs_config_default_set();cdfs_filelist_init(&files,file_entries,CDFS_FILE_CAPACITY);cdfs_filelist_root_read(&files);\n'
if main_anchor not in s: raise SystemExit('HWFIX3: CDFS init anchor missing')
alloc=''' cdfs_config_default_set();cdfs_filelist_init(&files,file_entries,CDFS_FILE_CAPACITY);cdfs_filelist_root_read(&files);\n /* HWFIX3: Yaul/newlib owns its filesystem buffers first. Reserve the combat\n  * pack arena only after CDFS root init, before frontend allocations can\n  * fragment the heap. Keep 32-byte alignment for CD/DMA-friendly reads. */\n match_pack_arena_raw=malloc(OBJECT_PACK_ALIGNED_BYTES+RING_TRANSITION_PACK_ALIGNED_BYTES+31U);\n if(!match_pack_arena_raw)check_error=347;\n else match_pack_arena=(uint8_t *)(((uintptr_t)match_pack_arena_raw+31U)&~(uintptr_t)31U);\n'''
s=s.replace(main_anchor,alloc,1)
s=s.replace('WWF SATURN TEST254: DATA ERROR','WWF SATURN TEST255 HWFIX3: DATA ERROR')
s=s.replace('TEST254 V0.269 - START: RESUME','TEST255 V0.270 HWFIX3 - START: RESUME')
p.write_text(s)
print(f'HWFIX3 patch applied for {roster_count} roster packs')
