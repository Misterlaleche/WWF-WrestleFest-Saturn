#!/usr/bin/env python3
"""Compare native credit/START owners against original M68000 RAM and registers."""
import argparse,ctypes,hashlib,json,subprocess,zipfile
from pathlib import Path
from unicorn import Uc,UC_ARCH_M68K,UC_MODE_BIG_ENDIAN
from unicorn.m68k_const import *

class Registers(ctypes.Structure):
    _fields_=[('d',ctypes.c_uint32*8),('valid',ctypes.c_uint8)]
class Context(ctypes.Structure):
    _fields_=[('ram',ctypes.POINTER(ctypes.c_uint8)),('bytes',ctypes.c_size_t),
              ('regs',ctypes.POINTER(Registers)),('a',ctypes.POINTER(ctypes.c_uint32)),
              ('effect',ctypes.c_void_p),('user',ctypes.c_void_p)]
D=[UC_M68K_REG_D0,UC_M68K_REG_D1,UC_M68K_REG_D2,UC_M68K_REG_D3,UC_M68K_REG_D4,UC_M68K_REG_D5,UC_M68K_REG_D6,UC_M68K_REG_D7]
A=[UC_M68K_REG_A0,UC_M68K_REG_A1,UC_M68K_REG_A2,UC_M68K_REG_A3,UC_M68K_REG_A4,UC_M68K_REG_A5,UC_M68K_REG_A6]

def main():
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--rom',type=Path,required=True);p.add_argument('--out',type=Path,required=True);a=p.parse_args()
    a.out.mkdir(parents=True,exist_ok=False)
    root=Path(__file__).resolve().parents[1];native=root.parents[1]/'modo Royal/wp1980/work/hwfix01/native'
    lib=a.out/'start.so'
    subprocess.run(['cc','-std=c11','-O2','-Wall','-Wextra','-Werror','-shared','-fPIC','-I'+str(native/'include'),str(native/'src/wf_r18_frontend_boot.c'),str(native/'src/wf_r17_m68k_shadow.c'),'-o',str(lib)],check=True)
    n=ctypes.CDLL(str(lib.resolve()));n.wf_r18_frontend_start_0978.argtypes=[ctypes.POINTER(Context),ctypes.POINTER(ctypes.c_uint16),ctypes.POINTER(ctypes.c_uint)];n.wf_r18_frontend_start_0978.restype=ctypes.c_int
    n.wf_r18_frontend_credit_055a.argtypes=[ctypes.POINTER(Context)];n.wf_r18_frontend_credit_055a.restype=ctypes.c_int
    with zipfile.ZipFile(a.rom) as z:
        rom=bytearray(0x80000);rom[::2]=z.read('31e14-0.ic18');rom[1::2]=z.read('31e13-0.ic19')
    assert hashlib.sha1(rom).hexdigest()=='a726c46f350723ea73158bc78055d17d418b14e2'
    results=[]
    for owner in (0x978,0x55a):
      for config in ((0,0x800) if owner==0x978 else (0,)):
       for credits in (0,1,2,98,99,255):
        for buttons in (range(16) if owner==0x978 else (0,4)):
         for pattern in (0,1):
            raw=bytearray(((i*37+pattern*97)&255) for i in range(0x4000));raw[0x4f]=credits;raw[0x66:0x68]=config.to_bytes(2,'big')
            ds=[((0x12340000+i*0x10101)^pattern*0xfedcba98)&0xffffffff for i in range(8)]
            av=[0x1c0200+i*64 for i in range(7)]
            if owner==0x55a:ds[0]=(ds[0]&0xffff0000)|buttons
            ports=[0xffff^(0x80 if buttons&(1<<i) else 0) for i in range(4)]
            ram=(ctypes.c_uint8*0x4000).from_buffer_copy(raw);reg=Registers((ctypes.c_uint32*8)(*ds),1);ar=(ctypes.c_uint32*7)(*av);c=Context(ram,0x4000,ctypes.pointer(reg),ar,None,None);mask=ctypes.c_uint()
            carry=n.wf_r18_frontend_start_0978(ctypes.byref(c),(ctypes.c_uint16*4)(*ports),ctypes.byref(mask)) if owner==0x978 else n.wf_r18_frontend_credit_055a(ctypes.byref(c))
            u=Uc(UC_ARCH_M68K,UC_MODE_BIG_ENDIAN);u.ctl_set_cpu_model(UC_CPU_M68K_M68000);u.mem_map(0,0x200000);u.mem_write(0,bytes(rom));u.mem_write(0x1c0000,bytes(raw));u.reg_write(UC_M68K_REG_SR,0x2700);u.reg_write(UC_M68K_REG_A7,0x1c3bd0);u.mem_write(0x1c3bd0,(0x1ff000).to_bytes(4,'big'))
            for r,v in zip(D,ds):u.reg_write(r,v)
            for r,v in zip(A,av):u.reg_write(r,v)
            for i,v in enumerate(ports):u.mem_write(0x140020+i*2,v.to_bytes(2,'big'))
            u.emu_start(owner,0x1ff000,count=100000)
            assert u.reg_read(UC_M68K_REG_PC)==0x1ff000
            expected=bytes(u.mem_read(0x1c0000,0x3ad4));actual=bytes(ram)[:0x3ad4]
            delta=[i for i,(x,y) in enumerate(zip(actual,expected)) if x!=y]
            dref=[u.reg_read(r) for r in D];aref=[u.reg_read(r) for r in A]
            case=dict(owner=hex(owner),config=config,credits=credits,buttons=buttons,pattern=pattern)
            assert not delta and list(reg.d)==dref and list(ar)==aref,dict(case,ram_diff=delta[:20],native_d=list(reg.d),rom_d=dref,native_a=list(ar),rom_a=aref)
            # Unicorn's SR accessor does not expose lazy CCR reliably; capture
            # carry by executing MOVE SR,D7 after the original RTS boundary.
            u.mem_write(0x1ff000,bytes.fromhex('40c74e71'));u.emu_start(0x1ff000,0x1ff004,count=2)
            assert carry==(u.reg_read(UC_M68K_REG_D7)&1),dict(case,native_carry=carry,rom_sr=u.reg_read(UC_M68K_REG_D7))
            results.append(dict(case,carry=carry,accepted_mask=mask.value,ram_sha256=hashlib.sha256(actual).hexdigest()))
    report=dict(status='PASS',rom_sha1=hashlib.sha1(rom).hexdigest(),cases=len(results),scope='Persistent RAM below3AD4, D0-D7, A0-A6, carry; explicit fixtures, not cold boot',results=results)
    (a.out/'report.json').write_text(json.dumps(report,indent=2)+'\n');print('PASS',len(results),'original START/credit cases')
if __name__=='__main__':main()
