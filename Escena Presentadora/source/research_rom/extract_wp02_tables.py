#!/usr/bin/env python3
"""Extract numeric native prematch tables from the user-supplied verified ROM."""
from pathlib import Path
import argparse,hashlib,zipfile
SHA1='a726c46f350723ea73158bc78055d17d418b14e2'
def main():
 ap=argparse.ArgumentParser();ap.add_argument('romzip',type=Path);ap.add_argument('--out',type=Path,default=Path(__file__).resolve().parent/'wp02_rom_tables.h');args=ap.parse_args()
 with zipfile.ZipFile(args.romzip) as z:e=z.read('31e14-0.ic18');o=z.read('31e13-0.ic19')
 b=bytearray(len(e)+len(o));b[::2]=e;b[1::2]=o;assert hashlib.sha1(b).hexdigest()==SHA1
 w=lambda a:int.from_bytes(b[a:a+2],'big');l=lambda a:int.from_bytes(b[a:a+4],'big')
 rows=[]
 for i in range(12):
  n=b[0xad9c+i];a=l(0xad6c+4*i);rows.append((n,[w(a+2*j) for j in range(n)],[w(a+2*n+2*j) for j in range(n)]))
 text='/* Numeric prematch selector/threshold tables extracted from supplied ROM.\n   Main SHA1 '+SHA1+'. */\n'
 text+='static const uint16_t actor_windows[12] = {'+','.join(str(w(0xac9e+2*i)) for i in range(12))+'};\n'
 text+='static const uint8_t identity_voices[12] = {'+','.join('0x%02x'%b[0xac4a+i] for i in range(12))+'};\n'
 text+='static const uint8_t program_counts[12] = {'+','.join(str(r[0]) for r in rows)+'};\n'
 for label,index in [('program_selectors',1),('program_thresholds',2)]:
  text+='static const uint16_t '+label+'[12][11] = {\n'
  for r in rows:text+='    {'+','.join(str(v) for v in r[index])+'},\n'
  text+='};\n'
 args.out.write_text(text);print('Wrote',args.out,'from verified ROM',SHA1)
if __name__=='__main__':main()
