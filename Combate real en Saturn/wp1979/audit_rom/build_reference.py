#!/usr/bin/env python3
"""Extract the wp1979 four-wrestler movement reference from the supplied ROM.

No velocity is selected by visual inference. Coefficients and magnitudes are
read from the canonical program and all arithmetic retains the fixed-point units.
"""
from pathlib import Path
from fractions import Fraction
import argparse, hashlib, json, zipfile

SHA1 = "a726c46f350723ea73158bc78055d17d418b14e2"
CHARACTERS = [(0,"Hulk Hogan"),(1,"Ultimate Warrior"),(9,"Ted DiBiase"),(7,"Mr. Perfect")]
def reconstruct(path):
    with zipfile.ZipFile(path) as z:
        b=bytearray(0x80000)
        b[0::2]=z.read("31e14-0.ic18")
        b[1::2]=z.read("31e13-0.ic19")
    assert hashlib.sha1(b).hexdigest()==SHA1, "Unexpected original program"
    return b
def build(b):
    u16=lambda a:int.from_bytes(b[a:a+2],"big")
    fps=Fraction(7000000,448*272)
    chars=[]
    for ident,name in CHARACTERS:
        c={"id":ident,"name":name,"walking":{},"running":{}}
        for key,base in [("walking",0x116ae),("running",0x11d4a)]:
            mag=b[base+ident]
            c[key]={"magnitude":mag,"magnitude_rom_address":hex(base+ident)}
            for axis,coef in [("x",u16(0x2378+64*2)),("y",u16(0x23fa))]:
                delta=mag*coef
                c[key][axis]={"delta_16_16_per_tick":delta,"pixels_per_tick":float(Fraction(delta,65536)),"pixels_per_second_at_mame_reference":float(Fraction(delta,65536)*fps)}
        chars.append(c)
    return {"program_sha1":SHA1,"refresh":{"reference_hz":float(fps),"numerator":fps.numerator,"denominator":fps.denominator,"provenance":"MAME machine configuration: pixel clock 28MHz/4, HTOTAL448, VTOTAL272. Totals explicitly guessed; not a measured original-PCB rate."},"cadence_contract":"One gameplay iteration per original VBlank frame-ready/ack handshake. Never alter movement or animation constants to compensate for renderer cost.","characters":chars,"animation":{"interpreter":"0x1C0C8","finite_duration":"stored N means N+1 interpreter ticks","hold":65280,"hold_semantics":"0xFF00: external/callback hold; no automatic expiry","loop_kind":2,"walking":{"descriptor":"0x115F2","callback":"0x11652","poses":[1,2,3,4],"stored_duration_each":10,"ticks_each":11,"cycle_ticks":44},"running":{"descriptor":"0x11C62","callback":"0x11C82","poses":[9,10,11,12,13,14],"stored_duration_each":6,"ticks_each":7,"cycle_ticks":42}},"actors":{"wrestler_base":"0x1C05B0","stride":268,"count":9,"referee":"0x1C11F4","tag_partner_pairs":[[0,1],[2,3]],"fields":{"identity_word":2,"visual_word":4,"x_16_16":6,"y_16_16":10,"z_16_16":14,"display_x":20,"display_y":22,"latched_state":28,"previous_state":30,"main_state":32,"animation_countdown":34,"animation_index":36,"linked_actor":38,"magnitude_word":42,"angle_word":44,"facing_word":46,"contact_word":54,"control_flags":86,"identity_byte":87,"vx8_8":88,"vy8_8":90,"vz8_8":92,"vertical_decrement":94,"raw_action":96,"state4_subtype_word":100,"stamina":102,"pending_damage":104,"stamina_tier":112,"target_actor":122,"partner_actor":134,"input_source_pointer":138}},"frame":{"gameplay_entry":"0xF8C","iteration_start":"0xF9A","loop_counter":"0x1C0080","handshake_byte":"0x1C006F","ready_bit":0,"ack_bit":1,"actor_order":"ascending, live active set","actor_turn":["clear contact +36/+37","resolver280DC","apply high-WORD corrections","state callback1C03E","animation1C0C8","projection247C","queue27B8","motion2208"],"after_actor_turns":["24090 collision profile invalidation","branchA:24062 collision","branchA:24E58 damage"]}}
def main():
    ap=argparse.ArgumentParser();ap.add_argument("rom",type=Path);ap.add_argument("output",type=Path);a=ap.parse_args()
    a.output.write_text(json.dumps(build(reconstruct(a.rom)),indent=2)+"\n")
    print("PASS canonical ROM hash; wrote",a.output)
if __name__=="__main__":main()
