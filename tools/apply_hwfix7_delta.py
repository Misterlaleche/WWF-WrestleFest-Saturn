#!/usr/bin/env python3
from pathlib import Path
import sys
root=Path(sys.argv[1] if len(sys.argv)>1 else '.')
rp=root/'render_pose.h'; mp=root/'main.c'
r=rp.read_text(); s=mp.read_text()
old='''    if(f->state==ST_OBJECT_PICKUP||f->state==ST_OBJECT_USE)return true;\n    return false;\n}\n'''
new='''    if(f->state==ST_OBJECT_PICKUP||f->state==ST_OBJECT_USE)return true;\n    /* HWFIX7 PROVISIONAL: raw ground actions are ROM-EXACT, but some partner\n     * compositions are not available for every active slot/pair yet.  In\n     * particular ST_GROUND_PICKUP uses ROM descriptor $15B6C and can emit raw\n     * $CE (206).  Allow fallback only when the Gxy overlay path did not already\n     * consume the state.  This keeps all unrelated unmapped poses fail-closed. */\n    if(f->state==ST_GROUND_ATTACK||f->state==ST_GROUND_PICKUP)return true;\n    return false;\n}\n'''
if old not in r: raise SystemExit('HWFIX7 render fallback anchor missing')
r=r.replace(old,new,1)
r=r.replace('HWFIX6 DIAGNOSTIC / PROVISIONAL','HWFIX7 DIAGNOSTIC / PROVISIONAL',1)
rp.write_text(r)
s=s.replace('WWF SATURN TEST255 HWFIX6: DATA ERROR','WWF SATURN TEST255 HWFIX7: DATA ERROR')
s=s.replace('TEST255 V0.270 HWFIX6 - START: RESUME','TEST255 V0.270 HWFIX7 - START: RESUME')
mp.write_text(s)
print('HWFIX7 delta applied: bounded ground-action fallback')
