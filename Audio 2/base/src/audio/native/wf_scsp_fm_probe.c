#include "wf_scsp_fm_probe.h"

enum { KEY_EXECUTE=0x1000u, KEY_BIT=0x0800u, REG_BASE=0x100000u };

bool wf_scsp_fm_probe_patch_valid(const wf_scsp_fm_probe_patch_t *patch)
{
    unsigned op;
    uint32_t center;
    if(!patch || (patch->wave_start&1u) ||
       !(patch->wave_start==0x10000u ||
         (patch->wave_start>=WF_FM_WAVE_MIN &&
          patch->wave_start<=WF_FM_WAVE_LIMIT-WF_FM_WAVE_BYTES)))
        return false;
    center=patch->wave_start+2048u;
    for(op=0;op<WF_FM_OPERATORS;++op) {
        const uint16_t *w=patch->words[op];
        if(w[0]!=(uint16_t)(0x20u|(center>>16)) ||
           w[1]!=(uint16_t)center || w[2]!=0u || w[3]!=1024u ||
           (w[5]&0x8000u) || (w[6]&0xff00u) ||
           (w[8]&0x8400u) || w[9]!=0u || w[10]!=0u || (w[11]&0x00ffu))
            return false;
    }
    return true;
}

bool wf_scsp_fm_probe_upload(const wf_scsp_fm_probe_patch_t *patch,
                            const uint8_t *wave_be,size_t bytes,
                            wf_scsp_probe_write16_fn write16,void *user)
{
    size_t i;
    if(!wf_scsp_fm_probe_patch_valid(patch) || !wave_be ||
       bytes!=WF_FM_WAVE_BYTES || !write16)
        return false;
    for(i=0;i<bytes;i+=2u) {
        uint16_t value=(uint16_t)(((uint16_t)wave_be[i]<<8)|wave_be[i+1u]);
        if(!write16(patch->wave_start+(uint32_t)i,value,user)) return false;
    }
    return true;
}

bool wf_scsp_fm_probe_stop(const wf_scsp_fm_probe_patch_t *patch,
                          wf_scsp_probe_write16_fn write16,void *user)
{
    unsigned op;
    if(!wf_scsp_fm_probe_patch_valid(patch) || !write16) return false;
    for(op=0;op<WF_FM_OPERATORS;++op)
        if(!write16(REG_BASE+op*32u,patch->words[op][0],user)) return false;
    return write16(REG_BASE+3u*32u,
                   (uint16_t)(patch->words[3][0]|KEY_EXECUTE),user);
}

bool wf_scsp_fm_probe_start(const wf_scsp_fm_probe_patch_t *patch,
                           wf_scsp_probe_write16_fn write16,void *user)
{
    unsigned op,word;
    if(!wf_scsp_fm_probe_patch_valid(patch) || !write16) return false;
    if(!wf_scsp_fm_probe_stop(patch,write16,user)) return false;
    for(op=0;op<WF_FM_OPERATORS;++op)
        for(word=0;word<WF_FM_WORDS;++word)
            if(!write16(REG_BASE+op*32u+word*2u,patch->words[op][word],user))
                return false;
    for(op=0;op<WF_FM_OPERATORS;++op)
        if(!write16(REG_BASE+op*32u,(uint16_t)(patch->words[op][0]|KEY_BIT),user))
            return false;
    return write16(REG_BASE+3u*32u,
                   (uint16_t)(patch->words[3][0]|KEY_BIT|KEY_EXECUTE),user);
}

bool wf_scsp_fm_probe_mmio_write16(uint32_t address,uint16_t value,void *user)
{
    uintptr_t base;
    if(!user || (address&1u) ||
       !((address>=0x10000u && address<0x11800u) ||
         (address>=WF_FM_WAVE_MIN && address<WF_FM_WAVE_LIMIT) ||
         (address>=REG_BASE && address<REG_BASE+128u)))
        return false;
    base=*(const uintptr_t *)user;
    *(volatile uint16_t *)(base+(uintptr_t)address)=value;
    return true;
}
