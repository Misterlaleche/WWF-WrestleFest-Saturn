#include <string.h>
#include "wf_r17_dispatch_1c03e.h"
#include "wf_r17_raw23.h"
#include "wf_r18_runtime_handoff.h"
#include "wf_physics.h"
#include "wf_r17_resolver_280dc.h"

static wf_status_t raw23_audio(uint16_t command,void *user){
    (void)command;(void)user;return WF_OK;
}
static wf_status_t raw23_partner(wf_runtime_t *rt,wf_actor_raw_t *actor,void *user){
    uint32_t blocked=0u;(void)user;
    return wf_r17_action_partner_215b6(rt,actor,&blocked);
}

static wf_status_t video_batch(wf_runtime_t *rt,const wf_r17_fg0_write_t *writes,
                               size_t count,void *user){
    uint8_t *mem=user;(void)rt;
    for(size_t i=0;i<count;i++){
        if(!((writes[i].addr>=0xc0000u&&writes[i].addr+writes[i].width<=0xc2000u)||
             (writes[i].addr>=0x180000u&&writes[i].addr+writes[i].width<=0x184000u)))return WF_BLOCKED;
        for(unsigned j=0;j<writes[i].width;j++)
            mem[writes[i].addr+j]=(uint8_t)(writes[i].value>>(8u*(writes[i].width-1u-j)));
    }
    return WF_OK;
}
static wf_status_t palette_ready(wf_runtime_t *rt,uint16_t id,void *user){
    uint8_t *mem=user;(void)rt;
    if(id!=31u)return WF_BLOCKED;
    uint8_t *used=mem+0x1c1600u,*map=mem+0x1c1610u;
    if(map[id]&0x80u)return WF_OK;
    unsigned bank=0u;while(bank<16u&&used[bank])bank++;
    if(bank==16u)return WF_OK;
    used[bank]=0xffu;map[id]=(uint8_t)(0x80u|bank);
    uint8_t *dst=mem+0x182000u+bank*128u;
    dst[0]=dst[1]=0u;
    memcpy(dst+2,mem+0x2f22u+id*32u+2u,30u);
    return WF_OK;
}
static wf_status_t release_palette(wf_runtime_t *rt,uint16_t id,void *user){
    uint8_t *mem=user,*g=mem+0x1c0000u;
    (void)rt;
    if(id>=12u)return WF_BLOCKED;
    uint8_t allocation=g[0x1610u+id];
    if(allocation){g[0x1610u+id]=0u;g[0x1600u+(allocation&15u)]=0u;}
    return WF_OK;
}

int action_reversal_family_oracle(uint8_t *mem, uint32_t *d, unsigned slot,
                                  uint32_t *descriptor) {
    wf_runtime_t r = {0};
    wf_r17_actions_r11_bridge_t actions = {0};
    wf_r17_raw23_ctx_t raw23 = {0};
    wf_r17_dispatch_1c03e_ctx_t dispatch = {0};
    wf_r17_m68k_shadow_t regs = {0};
    memcpy(regs.d, d, 32);
    regs.valid = 1;
    if (wf_r18_runtime_import(&r, mem + 0x1c0000, 0x4000, &regs) != WF_OK) return -1;
    actions.fixed_object0 = &r.fixed_object0;
    actions.fixed_object1 = &r.fixed_object1;
    raw23.mode_0161 = r.g.mode_0161;
    raw23.global_15d2 = r.g.global_15d2;
    raw23.global_15d3 = r.g.global_15d3;
    raw23.shake_1800 = r.g.global_1800;
    raw23.referee_1150 = r.g.global_1150;
    raw23.referee_1180 = r.g.global_1180;
    raw23.referee_11b0 = r.g.global_11b0;
    raw23.referee_11e0 = r.g.global_11e0;
    raw23.referee_1151 = (uint8_t)r.g.global_1150;
    raw23.referee_11b1 = (uint8_t)r.g.global_11b0;
    raw23.audio_emit = raw23_audio;
    raw23.special_215b6 = raw23_partner;
    actions.raw23_ctx = &raw23;
    actions.video_batch = video_batch;
    actions.video_user = mem;
    actions.resource_2aea = palette_ready;
    actions.resource_2b58 = release_palette;
    dispatch.actions = &actions;
    int was_raw23 = (wf_be16(&r.actors[slot],WF_A_SELECTOR)&0xffu)==0x23u;
    wf_status_t st = wf_r17_dispatch_1c03e_bounded(&r, &r.actors[slot], &dispatch);
    if(was_raw23){
        r.g.global_15d2 = raw23.global_15d2;
        r.g.global_15d3 = raw23.global_15d3;
        r.g.global_1800 = raw23.shake_1800;
        r.g.global_1150 = raw23.referee_1150;
        r.g.global_1180 = raw23.referee_1180;
        r.g.global_11b0 = raw23.referee_11b0;
        r.g.global_11e0 = raw23.referee_11e0;
    }
    if (wf_r18_runtime_export(&r, mem + 0x1c0000, 0x4000) != WF_OK) return -2;
    memcpy(d, r.m68k_regs.d, 32);
    *descriptor = actions.trace[slot].tick_descriptor_rom;
    return st ? -(int)dispatch.blocked_dependency_rom : 0;
}

int action_reversal_physics_oracle(uint8_t *mem, uint32_t *d, unsigned slot) {
    wf_runtime_t r = {0};
    wf_r17_m68k_shadow_t regs = {0};
    if (!mem || !d || slot >= WF_ACTOR_SLOTS) return -1;
    memcpy(regs.d, d, sizeof regs.d);
    regs.valid = 1;
    if (wf_r18_runtime_import(&r, mem + 0x1c0000, 0x4000, &regs) != WF_OK) return -2;
    if (wf_u8(&r.actors[slot], WF_A_MOTION_MODE) > 2u) return -3;
    wf_physics_integrate_common(&r.actors[slot]);
    if (wf_r18_runtime_export(&r, mem + 0x1c0000, 0x4000) != WF_OK) return -4;
    memcpy(d, r.m68k_regs.d, sizeof r.m68k_regs.d);
    return 0;
}

int action_reversal_resolver_oracle(uint8_t *mem, uint32_t *d, unsigned slot) {
    wf_runtime_t r = {0};
    wf_r17_m68k_shadow_t regs = {0};
    if (!mem || !d || slot >= WF_ACTOR_SLOTS) return -1;
    memcpy(regs.d, d, sizeof regs.d);
    regs.valid = 1;
    if (wf_r18_runtime_import(&r, mem + 0x1c0000, 0x4000, &regs) != WF_OK) return -2;
    wf_actor_raw_t *a = &r.actors[slot];
    uint16_t state = wf_be16(a, WF_A_STATE);
    if (!(state & 0x8000u)) {
        wf_set_be16(a, WF_A_PRIOR, wf_be16(a, WF_A_LATCHED));
        wf_set_be16(a, WF_A_LATCHED, state);
        wf_set_be16(a, WF_A_STATE, (uint16_t)(state|0x8000u));
    }
    wf_set_be16(a, WF_A_CONTACT, 0u);
    if (wf_u8(a, WF_A_MOTION_MODE)) {
        if (wf_r17_resolver_280dc(&r, a) != WF_OK) return -3;
        if (wf_u8(a, WF_A_CONTACT+1u)) {
            wf_set_be16(a, WF_A_X, (uint16_t)(wf_be16(a, WF_A_X)+wf_be16(a, WF_A_CORR_X)));
            wf_set_be16(a, WF_A_Y, (uint16_t)(wf_be16(a, WF_A_Y)+wf_be16(a, WF_A_CORR_Y)));
            wf_set_be16(a, WF_A_Z, (uint16_t)(wf_be16(a, WF_A_Z)+wf_be16(a, WF_A_CORR_Z)));
        }
    }
    if (wf_r18_runtime_export(&r, mem + 0x1c0000, 0x4000) != WF_OK) return -4;
    memcpy(d, r.m68k_regs.d, sizeof r.m68k_regs.d);
    return 0;
}
