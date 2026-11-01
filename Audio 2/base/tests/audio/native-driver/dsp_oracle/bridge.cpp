#include "emu.h"
#include "scspdsp.h"
static SCSPDSP dsp;static address_space ram;
#define API extern "C" __declspec(dllexport)
API void dsp_reset(){dsp.Init();dsp.space=&ram;std::memset(&ram,0,sizeof ram);}
API void dsp_program(unsigned i,unsigned v){dsp.MPRO[i]=u16(v);}
API void dsp_start(){dsp.Start();}
API void dsp_coef(unsigned i,unsigned v){dsp.COEF[i]=s16(v);}
API void dsp_madrs(unsigned i,unsigned v){dsp.MADRS[i]=u16(v);}
API void dsp_ram_write(unsigned a,unsigned v){ram.write_word(a,u16(v));}
API unsigned dsp_ram_read(unsigned a){return ram.read_word(a);}
API void dsp_step(){dsp.Step();}
API int dsp_output(unsigned i){return dsp.EFREG[i];}
API int dsp_mems(unsigned i){return dsp.MEMS[i];}
API int dsp_coefficient(unsigned i){return dsp.COEF[i];}
API int dsp_temp(unsigned i){return dsp.TEMP[i];}
API unsigned dsp_dec(){return dsp.DEC;}
API void dsp_input(unsigned channel,int value){dsp.SetSample(value,(int)channel,5);}

API void dsp_callbacks(u16 (*read)(u32),void (*write)(u32,u16)){ram.read=read;ram.write=write;}
