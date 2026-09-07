#!/usr/bin/env python3
from pathlib import Path
import sys
root=Path(sys.argv[1] if len(sys.argv)>1 else '.')
mp=root/'main.c'; rp=root/'render_pose.h'
s=mp.read_text(); r=rp.read_text()
old='''static inline bool render_pose_registered_fallback(const fighter_t *f,unsigned raw){
    (void)f;(void)raw;
    /* Test252 audit: the complete former Test212 registry is now covered by
     * ROM-exact resident art, Gxy partner overlays, or OBJ15.PAK.  Keep this
     * hook fail-closed so a newly discovered unmapped raw pose trips CHECK324
     * instead of silently substituting legacy native art. */
    return false;
}
'''
new='''static inline bool render_pose_registered_fallback(const fighter_t *f,unsigned raw){
    (void)raw;
    /* HWFIX6 DIAGNOSTIC / PROVISIONAL: HWFIX5 proved that resident RING14.PAK
     * and OBJ15.PAK are the memory-pressure trigger. Permit fallback ONLY for
     * the state families owned by those two deliberately disabled overlays.
     * Every other unmapped arcade pose still fails closed with CHECK324. */
    if(f->state==ST_RING_THROW||f->state==ST_RING_EXIT||f->state==ST_RING_ENTER||f->state==ST_TOSSED)
        return true;
    if(f->state==ST_RECOVER&&f->throw_recovery)return true;
    if(f->state==ST_OBJECT_PICKUP||f->state==ST_OBJECT_USE)return true;
    return false;
}
'''
if old not in r: raise SystemExit('HWFIX6 render fallback anchor missing')
r=r.replace(old,new,1);rp.write_text(r)
old='static uint32_t check_error;static unsigned phase,pending;\n'
new='static uint32_t check_error;static unsigned phase,pending;\nstatic unsigned check324_character=0xffffU,check324_state=0xffffU,check324_raw=0xffffU;\n'
if old not in s: raise SystemExit('HWFIX6 check_error anchor missing')
s=s.replace(old,new,1)
old='render_pose_t p=render_fighter_pose(f);if(p.frame>=ROSTER_FRAMES){check_error=324;return;}'
new='render_pose_t p=render_fighter_pose(f);if(p.frame>=ROSTER_FRAMES){int raw324=game_arcade_pose(f);check324_character=f->character;check324_state=f->state;check324_raw=raw324<0?0xffffU:((unsigned)raw324&0x7fffU);check_error=324;return;}'
if old not in s: raise SystemExit('HWFIX6 CHECK324 draw anchor missing')
s=s.replace(old,new,1)
old='''  if(check_error==330||check_error==331)
   text_printf("\\x1b[6;3HEXPECT %08lX\\x1b[7;3HACTUAL %08lX",(unsigned long)check_expected,(unsigned long)check_actual);
'''
new='''  if(check_error==324)
   text_printf("\\x1b[6;3HCHAR %lu  STATE %lu  RAW %lu",(unsigned long)check324_character,(unsigned long)check324_state,(unsigned long)check324_raw);
  if(check_error==330||check_error==331)
   text_printf("\\x1b[6;3HEXPECT %08lX\\x1b[7;3HACTUAL %08lX",(unsigned long)check_expected,(unsigned long)check_actual);
'''
if old not in s: raise SystemExit('HWFIX6 text diagnostic anchor missing')
s=s.replace(old,new,1)
s=s.replace('WWF SATURN TEST255 HWFIX5: DATA ERROR','WWF SATURN TEST255 HWFIX6: DATA ERROR')
s=s.replace('TEST255 V0.270 HWFIX5 - START: RESUME','TEST255 V0.270 HWFIX6 - START: RESUME')
mp.write_text(s)
print('HWFIX6 delta applied')
