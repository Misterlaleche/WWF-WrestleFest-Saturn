#!/usr/bin/env python3
"""Drive the production Saturn pad adapter through native input and movement.

The host supplies Jo's sampled peripheral array and a synthetic resident ROM.
The actual port lookup,
pad mapping, cabinet read, $514E decode, $F2D6 direction lookup, physics and
world/raster coordinate conversion run unchanged. Expected movement signs are
screen directions, independently of the arcade's upward-positive world Y.
"""
import argparse
import os
from pathlib import Path
import re
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[1]


def function(source, name):
    pattern = (r"(?:static\s+)?(?:uint8_t|int|wf_status_t)\s+"
               + re.escape(name) + r"\([^;]+?\)\s*\{")
    match = re.search(pattern, source)
    if not match:
        raise RuntimeError("Production function not found: " + name)
    depth, end = 1, match.end()
    while depth:
        depth += (source[end] == "{") - (source[end] == "}")
        end += 1
    return source[match.start():end] + "\n"


PREFIX = r'''
#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "jo/types.h"
#include "jo/smpc.h"
#include "jo/input.h"
#include "wf_jo_assets.h"
#include "wf_gameplay_bindings.h"
#include "wf_input.h"
#include "wf_physics.h"
#include "wf_r17_contract.h"
#include "wf_renderer_contract.h"
#include "wp1979_input.h"
#include "wp1980_input.h"
#define JO_ZERO(value) ((value)=0)
jo_peripheral jo_inputs[JO_INPUT_MAX_DEVICE];
static wf_runtime_t rt;
static wf_gameplay_binding_context_t gameplay;
static uint8_t test_rom[WF_JO_ROM_BYTES];
static bool rom_loaded;
static unsigned rom_reads;
static uint32_t rom_read_offset;
static size_t rom_read_bytes;

/* The asset boundary keeps its production signature and resident-read error
 * contract. Only synthetic bytes are supplied; input_word itself is extracted
 * unchanged from port/main.c below. No asset worker exists in this host test. */
int wf_jo_assets_rom_read(uint32_t offset,void *destination,size_t bytes)
{
    ++rom_reads;rom_read_offset=offset;rom_read_bytes=bytes;
    if(!rom_loaded)return WF_JO_ASSET_UNLOADED;
    if((!destination&&bytes)||offset>sizeof test_rom||bytes>sizeof test_rom-offset)
        return WF_JO_ASSET_ARGUMENT;
    if(bytes)memcpy(destination,test_rom+offset,bytes);
    return WF_JO_ASSET_OK;
}
'''

SUFFIX = r'''
static const unsigned ports[4]={0,1,6,7};
static unsigned failures,cases;
static const char *routes[3]={"live","TAG takeover","Royal takeover"};

static wf_actor_raw_t *sample(unsigned player,unsigned keys,unsigned pipeline,unsigned route)
{
    memset(&rt,0,sizeof rt);memset(&gameplay,0,sizeof gameplay);
    for(unsigned i=0;i<JO_INPUT_MAX_DEVICE;i++)jo_inputs[i].data=0xffffu;
    jo_inputs[ports[player]].data=(uint16_t)~keys;
    for(unsigned i=0;i<4;i++)gameplay.input_hw_low[i]=controls(i);
    uint8_t physical=gameplay.input_hw_low[player];
    if(route==1){
        wp1979_demo_active=1;
        gameplay.input_hw_low[player]=wp1979_input(10,physical);
        assert(!wp1979_demo_active&&wp1979_input(11,0xffu)==0xffu);
    }else if(route==2){
        wp1980_demo_active=1;
        gameplay.input_hw_low[player]=wp1980_input(&rt,physical);
        assert(!wp1980_demo_active&&wp1980_input(&rt,0xffu)==0xffu);
    }
    assert(gameplay.input_hw_low[player]==physical);
    gameplay.participant_input_word=input_word;
    rt.m68k_regs.valid=1;rt.g.pipeline_mode=(uint8_t)pipeline;
    wf_actor_raw_t *a=&rt.actors[0];
    wf_set_u8(a,WF_A_ACTIVE,0x80u);
    /* The actual +8A input source can move between TAG partners. */
    wf_set_be32(a,WF_A_INPUT_SOURCE,0x140020u+2u*player);
    wf_set_be16(a,WF_A_X,160);wf_set_be16(a,WF_A_Y,128);
    wf_set_u8(a,WF_A_MOTION_MODE,1);
    wf_set_be16(a,WF_A_MAGNITUDE,wf_physics_walk_speed(0,0));
    assert(wf_phase_input_real(&rt,0x514eu,&gameplay)==WF_OK);
    return a;
}

static int sign(int value){return (value>0)-(value<0);}
static void movement(void)
{
    static const struct {const char *name;unsigned keys;int x,y;} directions[]={
        {"UP",JO_KEY_UP,0,-1},{"DOWN",JO_KEY_DOWN,0,1},
        {"LEFT",JO_KEY_LEFT,-1,0},{"RIGHT",JO_KEY_RIGHT,1,0},
        {"UP LEFT",JO_KEY_UP|JO_KEY_LEFT,-1,-1},
        {"UP RIGHT",JO_KEY_UP|JO_KEY_RIGHT,1,-1},
        {"DOWN LEFT",JO_KEY_DOWN|JO_KEY_LEFT,-1,1},
        {"DOWN RIGHT",JO_KEY_DOWN|JO_KEY_RIGHT,1,1}
    };
    for(unsigned player=0;player<4;player++)
    for(unsigned pipeline=0;pipeline<2;pipeline++)
    for(unsigned route=0;route<(player==0?3u:1u);route++)
    for(unsigned i=0;i<sizeof directions/sizeof directions[0];i++){
        wf_actor_raw_t *a=sample(player,directions[i].keys,pipeline,route);
        assert(wf_r17_world_to_screen_247c(a,0,0)==WF_OK);
        int x=wf_sprite_x_decode(wf_be16(a,WF_A_SCREEN_X));
        int y=wf_sprite_y_decode(wf_be16(a,WF_A_SCREEN_Y));
        assert(direction_f2d6(&rt,a,&gameplay)==WF_OK);
        wf_physics_integrate_common(a);
        assert(wf_r17_world_to_screen_247c(a,0,0)==WF_OK);
        int dx=wf_sprite_x_decode(wf_be16(a,WF_A_SCREEN_X))-x;
        int dy=wf_sprite_y_decode(wf_be16(a,WF_A_SCREEN_Y))-y;
        ++cases;
        if(sign(dx)!=directions[i].x||sign(dy)!=directions[i].y){
            if(failures<8)fprintf(stderr,
                "FAIL %s P%u %s pipeline%u: raw=$%02x angle=$%04x screen delta=(%d,%d), wanted signs=(%d,%d)\n",
                directions[i].name,player+1,routes[route],pipeline,
                gameplay.input_hw_low[player],wf_be16(a,WF_A_ANGLE_WORD),
                dx,dy,directions[i].x,directions[i].y);
            ++failures;
        }
    }
}

static void buttons_and_connections(void)
{
    static const struct {unsigned keys;uint16_t held;} buttons[]={
        {JO_KEY_A,1},{JO_KEY_B,2},{JO_KEY_C,4},{JO_KEY_START,0x100},
        {JO_KEY_A|JO_KEY_B|JO_KEY_C|JO_KEY_START,0x107}
    };
    for(unsigned p=0;p<4;p++)
    for(unsigned i=0;i<sizeof buttons/sizeof buttons[0];i++){
        wf_actor_raw_t *a=sample(p,buttons[i].keys,1,0);
        assert(wf_be16(a,WF_A_INPUT_HELD)==buttons[i].held);
        assert(wf_be16(a,WF_A_INPUT_FRESH)==buttons[i].held);
        assert(wf_be16(a,WF_A_INPUT_DERIVED)==0);
    }
    for(unsigned i=0;i<JO_INPUT_MAX_DEVICE;i++)jo_inputs[i].data=0xffffu;
    for(unsigned p=0;p<4;p++)assert(controls(p)==0xffu);
    /* P2 without a multitap is physical array slot6, never slot1. */
    jo_inputs[1].id=jo_inputs[7].id=JO_PERIPHERAL_ID_NOT_CONNECTED;
    jo_inputs[6].data=(uint16_t)~JO_KEY_UP;
    assert(controls(0)==0xffu);
    if(controls(1)!=0xfbu){++failures;fprintf(stderr,"FAIL UP on physical port2\n");}
    assert(controls(2)==0xffu&&controls(3)==0xffu);
    jo_inputs[0].id=jo_inputs[6].id=JO_PERIPHERAL_ID_NOT_CONNECTED;
    for(unsigned p=0;p<4;p++)assert(controls(p)==0xffu);
}

static void rom_input_words(void)
{
    /* Cabinet reads must work without loading or consulting the ROM. */
    assert(!rom_loaded&&rom_reads==0);
    test_rom[0]=0x12;test_rom[1]=0x34;
    test_rom[2]=0x56;test_rom[3]=0x78;
    test_rom[WF_JO_ROM_BYTES-2u]=0x9a;test_rom[WF_JO_ROM_BYTES-1u]=0xbc;
    rom_loaded=true;
    static const struct {uint32_t address;uint16_t expected;} words[]={
        {0u,0x1234u},{2u,0x5678u},{WF_JO_ROM_BYTES-2u,0x9abcu},
        {0x01000000u,0x1234u}
    };
    for(unsigned i=0;i<sizeof words/sizeof words[0];i++){
        uint16_t value=0xa55au;unsigned reads=rom_reads;
        assert(input_word(&rt,words[i].address,&value,NULL)==WF_OK);
        assert(value==words[i].expected&&rom_reads==reads+1u);
        assert(rom_read_offset==(words[i].address&0xffffffu)&&rom_read_bytes==2u);
    }
    /* Invalid words must preserve the caller's output and never reach assets. */
    static const uint32_t invalid[]={
        1u,WF_JO_ROM_BYTES-1u,WF_JO_ROM_BYTES,0x14001eu,0x140028u,0xfffffffeu
    };
    unsigned reads=rom_reads;
    for(unsigned i=0;i<sizeof invalid/sizeof invalid[0];i++){
        uint16_t value=0xa55au;
        assert(input_word(&rt,invalid[i],&value,NULL)==WF_BAD_STATE);
        assert(value==0xa55au&&rom_reads==reads);
    }
    assert(input_word(&rt,0u,NULL,NULL)==WF_BAD_STATE&&rom_reads==reads);
    rom_loaded=false;
    uint16_t value=0xa55au;
    assert(input_word(&rt,0u,&value,NULL)==WF_BLOCKED);
    assert(value==0xa55au&&rom_reads==reads+1u);
    assert(rom_read_offset==0u&&rom_read_bytes==2u);
    rom_loaded=true;
    assert(input_word(&rt,0u,&value,NULL)==WF_OK&&value==0x1234u);
}

int main(void)
{
    for(unsigned i=0;i<JO_INPUT_MAX_DEVICE;i++){
        jo_inputs[i].id=JO_PERIPHERAL_ID_NOT_CONNECTED;
        jo_inputs[i].data=0xffffu;
    }
    for(unsigned p=0;p<4;p++)jo_inputs[ports[p]].id=JO_PERIPHERAL_ID_STANDARD_PAD;
    movement();buttons_and_connections();rom_input_words();
    if(failures){fprintf(stderr,"FAIL %u cases; %u movement cases exercised\n",failures,cases);return 1;}
    printf("PASS %u movement cases: eight visible directions, four players, both input pipelines; P1 TAG/Royal takeover\n",cases);
    puts("PASS A/B/C/START decoding, idle/disconnected pads and physical second-port routing");
    puts("PASS ROM words: big-endian reads, 24-bit aliases, bounds and failed-read output preservation");
    return 0;
}
'''


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--main-source", type=Path, default=ROOT / "port/main.c")
    parser.add_argument("--sanitize", action="store_true")
    args = parser.parse_args()
    main_source = args.main_source.read_text()
    prepass = (ROOT / "core/src/wf_actor_prepass.c").read_text()
    code = PREFIX + function((ROOT / "sdk/jo_engine/input.c").read_text(), "jo_get_player_input_port")
    code += function(main_source, "controls") + function(main_source, "input_word")
    code += function((ROOT / "core/src/wf_gameplay_bindings.c").read_text(), "wf_phase_input_real")
    code += function(prepass, "block") + function(prepass, "direction_f2d6") + SUFFIX
    sources = ["core/src/wf_input.c", "core/src/wf_physics.c",
               "core/src/wf_r17_render_contract.c", "core/src/wf_r17_m68k_shadow.c",
               "core/src/wp1979_input.c", "core/src/wp1980_input.c",
               "core/generated/wf_action_matrix.c", "core/generated/wf_physics_tables.c"]
    with tempfile.TemporaryDirectory(prefix="wf-jo-controls-") as directory:
        cfile, binary = Path(directory) / "test.c", Path(directory) / "test"
        cfile.write_text(code)
        flags = ["-std=c11", "-fms-extensions", "-O2", "-Wall", "-Wextra", "-Werror"]
        if args.sanitize:
            flags += ["-fsanitize=address,undefined", "-fno-omit-frame-pointer"]
        subprocess.run([os.environ.get("CC", "cc"), *flags,
                        "-I" + str(ROOT / "sdk/jo_engine"), "-I" + str(ROOT / "core/include"),
                        "-I" + str(ROOT / "port"),
                        str(cfile), *(str(ROOT / path) for path in sources), "-o", str(binary)], check=True)
        subprocess.run([str(binary)], check=True)


if __name__ == "__main__":
    main()
