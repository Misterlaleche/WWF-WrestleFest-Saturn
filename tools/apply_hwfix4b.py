#!/usr/bin/env python3
from pathlib import Path
import re, struct, sys

root=Path(sys.argv[1] if len(sys.argv)>1 else '.')
p=root/'main.c'
s=p.read_text()

needle='static void selection_cache_free(void){for(unsigned i=0;i<2;i++){free(selection_rle[i]);selection_rle[i]=NULL;selection_rle_size[i]=0;}}\n'
if needle not in s: raise SystemExit('HWFIX4: selection cache anchor missing')
if 'static void match_frontend_cache_release(void)' not in s:
    s=s.replace(needle,needle+'''static void match_frontend_cache_release(void){
 selection_cache_free();
 free(mode_rle_cache);mode_rle_cache=NULL;mode_rle_size=0;
 free(entry_rle_cache);entry_rle_cache=NULL;entry_rle_size=0;
 free(title_delta);title_delta=NULL;
}
''',1)

obj_decl='static roster_pack_t object_pack;\nstatic uint8_t *object_pack_data;\n'
if obj_decl not in s: raise SystemExit('HWFIX4: object declaration anchor missing')
if 'match_pack_arena_raw' not in s:
    s=s.replace(obj_decl,obj_decl+'''static uint8_t *match_pack_arena_raw,*match_pack_arena;
static unsigned match_pack_kind; /* 0 none, 1 object, 2 ring */
static bool match_pack_arena_acquire(void){
 if(match_pack_arena)return true;
 match_pack_arena_raw=malloc(RING_TRANSITION_PACK_ALIGNED_BYTES+31U);
 if(!match_pack_arena_raw){check_error=348;return false;}
 match_pack_arena=(uint8_t *)(((uintptr_t)match_pack_arena_raw+31U)&~(uintptr_t)31U);
 match_pack_kind=0;return true;
}
''',1)

ring_decl='static roster_pack_t ring_transition_pack;\nstatic uint8_t *ring_transition_pack_data;\n'
if ring_decl not in s: raise SystemExit('HWFIX4: ring declaration anchor missing')
if 'static void match_pack_arena_release(void)' not in s:
    s=s.replace(ring_decl,ring_decl+'''static void match_pack_arena_release(void){
 free(match_pack_arena_raw);match_pack_arena_raw=NULL;match_pack_arena=NULL;match_pack_kind=0;
 object_pack_data=NULL;ring_transition_pack_data=NULL;
 memset(&object_pack,0,sizeof(object_pack));memset(&ring_transition_pack,0,sizeof(ring_transition_pack));
}
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
 if(match_pack_kind==2&&ring_transition_pack_data)return true;
 if(!match_pack_arena_acquire())return false;
 uint8_t *dst=match_pack_arena;
 unsigned bytes=cd_read("RING14.PAK",dst,RING_TRANSITION_PACK_ALIGNED_BYTES);
 if(bytes!=RING_TRANSITION_PACK_BYTES||hash_ram(dst,RING_TRANSITION_PACK_BYTES)!=RING_TRANSITION_PACK_HASH){check_error=343;return false;}
 roster_pack_t pack;if(!roster_pack_open(&pack,dst,RING_TRANSITION_PACK_BYTES,RING_TRANSITION_PACK_FRAMES)){check_error=344;return false;}
 object_pack_data=NULL;memset(&object_pack,0,sizeof(object_pack));
 ring_transition_pack=pack;ring_transition_pack_data=dst;match_pack_kind=2;return true;
}
'''
if old not in s: raise SystemExit('HWFIX4: ring loader anchor missing')
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
 if(match_pack_kind==1&&object_pack_data)return true;
 if(!match_pack_arena_acquire())return false;
 uint8_t *dst=match_pack_arena;
 unsigned bytes=cd_read("OBJ15.PAK",dst,RING_TRANSITION_PACK_ALIGNED_BYTES);
 if(bytes!=OBJECT_PACK_BYTES||hash_ram(dst,OBJECT_PACK_BYTES)!=OBJECT_PACK_HASH){check_error=340;return false;}
 roster_pack_t pack;if(!roster_pack_open(&pack,dst,OBJECT_PACK_BYTES,OBJECT_PACK_FRAMES)){check_error=341;return false;}
 ring_transition_pack_data=NULL;memset(&ring_transition_pack,0,sizeof(ring_transition_pack));
 object_pack=pack;object_pack_data=dst;match_pack_kind=1;return true;
}
'''
if old not in s: raise SystemExit('HWFIX4: object loader anchor missing')
s=s.replace(old,new,1)

roster_text=(root/'roster_data.h').read_text();m=re.search(r'#define\s+ROSTER_COUNT\s+(\d+)',roster_text)
if not m: raise SystemExit('HWFIX4: ROSTER_COUNT not found')
roster_count=int(m.group(1));meta=[]
for ch in range(roster_count):
    path=root/f'cd/C{ch:02d}.PAK'
    if not path.exists(): raise SystemExit(f'HWFIX4: missing {path.name}')
    data=path.read_bytes()
    if len(data)<32: raise SystemExit(f'HWFIX4: {path.name} too small')
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
if anchor not in s: raise SystemExit('HWFIX4: roster loader anchor missing')
if 'static bool roster_pack_bind_verified' not in s:s=s.replace(anchor,binder+anchor,1)
s=s.replace(anchor,anchor+' match_pack_arena_release();\n match_frontend_cache_release();\n if(!match_pack_arena_acquire())return;\n',1)
oldcall='if(!roster_pack_open(&character_packs[ch],dst,roster_bytes[ch],ROSTER_FRAMES)){check_error=309;return;}'
if oldcall not in s: raise SystemExit('HWFIX4: roster open call missing')
s=s.replace(oldcall,'if(!roster_pack_bind_verified(&character_packs[ch],dst,ch)){check_error=346;return;}',1)
oldtail='roster_low_used=used;if(!object_pack_load()||!ring_transition_pack_load())return;loaded_roster=true;'
if oldtail not in s: raise SystemExit('HWFIX4: roster tail anchor missing')
s=s.replace(oldtail,'roster_low_used=used;if(!ring_transition_pack_load())return;loaded_roster=true;',1)

s=s.replace('if(!object_pack_data||!object_pack.data){check_error=339;return true;}','if((!object_pack_data||!object_pack.data)&&!object_pack_load())return true;',1)
s=s.replace('if(!ring_transition_pack_data||!ring_transition_pack.data){check_error=342;return true;}','if((!ring_transition_pack_data||!ring_transition_pack.data)&&!ring_transition_pack_load())return true;',1)

entry='  transfer((void *)(VDP2_CRAM_MODE_1_OFFSET(0,0,0)+1792*2),asset_entryuipal,64);\n'
if entry in s and entry+'  free(entry_rle_cache);entry_rle_cache=NULL;entry_rle_size=0;\n' not in s:
    s=s.replace(entry,entry+'  free(entry_rle_cache);entry_rle_cache=NULL;entry_rle_size=0;\n',1)

s=s.replace('free(overflow_character);overflow_character=NULL;loaded_roster=false;',
            'free(overflow_character);overflow_character=NULL;match_pack_arena_release();loaded_roster=false;')
s=s.replace('campaign_ready=false;loaded_roster=false;presentation_load();return;',
            'campaign_ready=false;match_pack_arena_release();loaded_roster=false;presentation_load();return;')
s=s.replace('intro.demo=false;loaded_roster=false;',
            'intro.demo=false;match_pack_arena_release();loaded_roster=false;')
s=s.replace('loaded_roster=false;demo_ticks=0;',
            'match_pack_arena_release();loaded_roster=false;demo_ticks=0;')

s=s.replace('WWF SATURN TEST254: DATA ERROR','WWF SATURN TEST255 HWFIX4: DATA ERROR')
s=s.replace('TEST254 V0.269 - START: RESUME','TEST255 V0.270 HWFIX4 - START: RESUME')
p.write_text(s)
print(f'HWFIX4 patch applied for {roster_count} roster packs')
