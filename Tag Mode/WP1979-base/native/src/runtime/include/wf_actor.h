#ifndef WF_ACTOR_H
#define WF_ACTOR_H
#include <stdint.h>
#include <stddef.h>

#define WF_ACTOR_BYTES 0x10Cu
#define WF_ACTOR_SLOTS 9u

typedef struct wf_actor_raw { uint8_t b[WF_ACTOR_BYTES]; } wf_actor_raw_t;

static inline uint8_t wf_u8(const wf_actor_raw_t *a, size_t o){ return a->b[o]; }
static inline void wf_set_u8(wf_actor_raw_t *a,size_t o,uint8_t v){ a->b[o]=v; }
static inline uint16_t wf_be16(const wf_actor_raw_t *a,size_t o){ return (uint16_t)(((uint16_t)a->b[o]<<8)|a->b[o+1]); }
static inline void wf_set_be16(wf_actor_raw_t *a,size_t o,uint16_t v){ a->b[o]=(uint8_t)(v>>8); a->b[o+1]=(uint8_t)v; }
static inline uint32_t wf_be32(const wf_actor_raw_t *a,size_t o){ return ((uint32_t)a->b[o]<<24)|((uint32_t)a->b[o+1]<<16)|((uint32_t)a->b[o+2]<<8)|a->b[o+3]; }
static inline void wf_set_be32(wf_actor_raw_t *a,size_t o,uint32_t v){ a->b[o]=(uint8_t)(v>>24);a->b[o+1]=(uint8_t)(v>>16);a->b[o+2]=(uint8_t)(v>>8);a->b[o+3]=(uint8_t)v; }

/* ROM field offsets retained verbatim. */
enum {
 WF_A_ACTIVE=0x00, WF_A_MOTION_MODE=0x01, WF_A_CHARACTER=0x02, WF_A_MIRROR=0x04, WF_A_RENDER_LIST=0x12, WF_A_SCREEN_X=0x14, WF_A_SCREEN_Y=0x16, WF_A_X_CORR8=0x19, WF_A_Y_CORR8=0x1B, WF_A_X=0x06, WF_A_Y=0x0A, WF_A_Z=0x0E,
 WF_A_LATCHED=0x1C, WF_A_PRIOR=0x1E, WF_A_STATE=0x20, WF_A_ANIM_COUNTER=0x22, WF_A_ANIM_INDEX=0x24, WF_A_ACTION_PHASE=0x25,
 WF_A_LINK=0x26, WF_A_MAGNITUDE=0x2A, WF_A_ANGLE_WORD=0x2C, WF_A_ANGLE=0x2D, WF_A_FACING=0x2E,
 WF_A_FIELD18=0x18, WF_A_FIELD1A=0x1A, WF_A_SPATIAL_CLASS=0x30, WF_A_FLAGS32=0x32, WF_A_FLAGS33=0x33, WF_A_FLAGS34=0x34, WF_A_PINFLAGS=0x35, WF_A_CONTACT=0x36, WF_A_CORR_X=0x38,
 WF_A_CORR_Y=0x3A, WF_A_CORR_Z=0x3C, WF_A_FIELD3E=0x3E, WF_A_FIELD40=0x40, WF_A_FIELD42=0x42, WF_A_TIMER44=0x44, WF_A_TIMER46=0x46,
 WF_A_PAIR_CYCLE=0x48, WF_A_CAMERA_CLASS=0x4B, WF_A_PROFILE=0x4C, WF_A_TIMER52=0x52, WF_A_ROLE=0x56, WF_A_VX=0x58, WF_A_VY=0x5A, WF_A_VZ=0x5C, WF_A_VZ_DEC=0x5E,
 WF_A_SELECTOR=0x60, WF_A_SELECTOR_LO=0x61, WF_A_FIELD62=0x62, WF_A_STATE4=0x64, WF_A_STATE4_LO=0x65,
 WF_A_STAMINA=0x66, WF_A_PENDING_DAMAGE=0x68, WF_A_VISIBLE_DELTA=0x6A,
 WF_A_TIER=0x70, WF_A_TIER_LO=0x71, WF_A_STAMINA_MAX=0x72, WF_A_REL_FLAGS=0x74, WF_A_REL_LINK=0x76,
 WF_A_TARGET=0x7A, WF_A_RESERVE=0x7E, WF_A_TARGET2=0x82, WF_A_TEAM_LINK=0x86, WF_A_INPUT_SOURCE=0x8A, WF_A_COLL_CACHE=0x8E,
 WF_A_VISUAL_ACTION=0x90, WF_A_RECIP_LINK=0x92, WF_A_INPUT_FRESH=0xA2, WF_A_INPUT_ACC=0xA4,
 WF_A_INPUT_HELD=0xA6, WF_A_INPUT_DERIVED=0xA8, WF_A_MASH=0xAA,
 WF_A_DIR_STREAK=0xAC, WF_A_CPU_SUBSTATE=0xAF, WF_A_CPU_DISTANCE=0xB0, WF_A_CPU_DISTANCE_Y=0xB2,
 WF_A_CPU_B4=0xB4, WF_A_CPU_B5=0xB5, WF_A_CPU_B6=0xB6, WF_A_CPU_B7=0xB7, WF_A_CPU_B9=0xB9,
 WF_A_CPU_BA=0xBA, WF_A_CPU_BC=0xBC, WF_A_CPU_BD=0xBD, WF_A_CPU_BE=0xBE, WF_A_CPU_C0=0xC0, WF_A_C6=0xC6, WF_A_C7=0xC7, WF_A_C8=0xC8,
 WF_A_SECONDARY_LINK=0x9C, WF_A_AE=0xAE, WF_A_PROGRESS109=0x109, WF_A_INTERNAL_ID=0x57,
 WF_A_RESULT=0xA0, WF_A_RESULT_LO=0xA1, WF_A_REFILL_COUNTERS=0xE8, WF_A_CREDIT=0xC4, WF_A_COLL_COUNT=0xC6, WF_A_DOOM_COUNTER=0xBD, WF_A_TIMER_D0=0xD0, WF_A_LOCKOUT=0xFE, WF_A_RESULT_ROLE=0xFE, WF_A_RESULT_ROLE_LO=0xFF, WF_A_RECOVERY_COUNTER=0x106
};

static inline uint8_t wf_main_state(const wf_actor_raw_t *a){ return (uint8_t)(wf_be16(a,WF_A_STATE)&0x7Fu); }
static inline uint8_t wf_state4_subtype(const wf_actor_raw_t *a){ return wf_u8(a,WF_A_STATE4_LO); }
static inline uint8_t wf_state4_flags(const wf_actor_raw_t *a){ return wf_u8(a,WF_A_STATE4); }
/* Exact write-width helpers. */
static inline void wf_state4_write_low(wf_actor_raw_t *a,uint8_t low){ wf_set_u8(a,WF_A_STATE4_LO,low); }
static inline void wf_state4_write_word(wf_actor_raw_t *a,uint16_t word){ wf_set_be16(a,WF_A_STATE4,word); }

#define WF_ARCADE_ACTOR_BASE0 0x001C05B0u
#define WF_ARCADE_ACTOR_STRIDE 0x10Cu
static inline uint32_t wf_actor_rom_base(unsigned slot){ return slot < WF_ACTOR_SLOTS ? (WF_ARCADE_ACTOR_BASE0 + (uint32_t)slot * WF_ARCADE_ACTOR_STRIDE) : 0u; }

#endif
