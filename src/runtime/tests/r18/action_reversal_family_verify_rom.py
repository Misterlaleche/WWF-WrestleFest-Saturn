"""Compare bounded linked actions with the unmodified arcade ROM."""
import ctypes
import argparse
import hashlib
import itertools
import json
import random
import re
import shutil
import struct
import subprocess
import tempfile
from collections import deque
from pathlib import Path

import machine68k as mus

ROOT = Path(__file__).resolve().parents[2]
HERE = Path(__file__).resolve().parent
RAM = 0x1c0000
ACTORS = [RAM + 0x5b0 + i * 0x10c for i in range(9)]
FAMILY = {0x11: (0x14658, 0x14670), 0x12: (0x147e6, 0x14826),
          0x13: (0x149e2, 0x149f2), 0x14: (0x14b48, 0x14b64),
          0x35: (0x175a8, 0x175c4),
          0x44: (0x1863c, 0x18668),
          0x8e: (0x1ae28, 0x1ae2e),
          0x55: (0x18fee, 0x19002), 0x56: (0x19076, 0x19086),
          0x5a: (0x192f2, 0x19302),
          0x5b: (0x1939a, 0x193aa), 0x78: (0x1a186, 0x1a192),
          0x7d: (0x1a5f4, 0x1a604), 0x7e: (0x1a674, 0x1a688),
          0x7f: (0x1a704, 0x1a718),
          0x80: (0x1a794, 0x1a7b4), 0x82: (0x1a822, 0x1a832),
          0x86: (0x1aa76, 0x1aa9a), 0x88: (0x1ab32, 0x1ab38),
          0x89: (0x1aba4, 0x1abc0), 0x72: (0x19fc6, 0x19fda),
          0x06: (0x13020, 0x13034), 0x4e: (0x18a90, 0x18aa0),
          0x6a: (0x19b3c, 0x19b50), 0x00: (0x12850, 0x12888),
          0x4f: (0x18afe, 0x18b0e), 0x70: (0x19e80, 0x19ea0),
          0x7b: (0x1a532, 0x1a538), 0x7c: (0x1a594, 0x1a5a0),
          0x6b: (0x19bba, 0x19bc0), 0x69: (0x199fc, 0x19a20),
          0x68: (0x1981e, 0x19832), 0x08: (0x1316c, 0x13180),
          0x24: (0x1603a, 0x16056), 0x15: (0x14ce0, 0x14cf4),
          0x16: (0x14d8a, 0x14da2), 0x17: (0x14e2c, 0x14e40),
          0x26: (0x1623c, 0x16258), 0x29: (0x1656a, 0x16582),
          0x4c: (0x18772, 0x1878a), 0x0a: (0x13734, 0x13744),
          0x0c: (0x13b98, 0x13bc8), 0x0d: (0x153a0, 0x153bc),
          0x0e: (0x13d26, 0x13d46), 0x10: (0x141d8, 0x141ec),
          0x1e: (0x159f8, 0x15a20), 0x33: (0x17266, 0x17282),
          0x37: (0x17a40, 0x17a58),
          0x61: (0x194ca, 0x194d0),
          0x59: (0x19264, 0x19274),
          0x77: (0x1a148, 0x1a154),
          0x3d: (0x114ac, 0x114b2),
          0x4d: (0x1892c, 0x18948),
          0x4a: (0x180ac, 0x180c4),
          0x4b: (0x181ec, 0x1820c),
          0x48: (0x132d8, 0x132f4),
          0x51: (0x18bf4, 0x18c24),
          0x57: (0x1913e, 0x1914e),
          0x76: (0x1a0fe, 0x1a10e),
          0x65: (0x19664, 0x19670),
          0x73: (0x1a014, 0x1a020),
          0x74: (0x1a050, 0x1a05c),
          0x5e: (0x19530, 0x19536),
          0x81: (0x1a7a4, 0x1a7b4),
          0x84: (0x1a940, 0x1a950),
          0x85: (0x1a9d4, 0x1a9e4),
          0x8b: (0x1ad10, 0x1ad24),
          0x8c: (0x1ad76, 0x1ad7c),
          0x71: (0x19f0e, 0x19f1e),
          0x66: (0x1969e, 0x196ae), 0x87: (0x1aa8a, 0x1aa9a),
          0x23: (0x15ce0, 0x15d08)}
parser = argparse.ArgumentParser()
parser.add_argument('--selector', type=lambda value: int(value, 0))
parser.add_argument('--trace-first', action='store_true')
parser.add_argument('--fast', action='store_true', help='compile oracle at O0 for local iteration')
parser.add_argument('--all-characters', action='store_true')
parser.add_argument('--limit', type=int, help='stop after this many fixture cases')
parser.add_argument('--report', type=Path, help='write differential evidence as JSON')
parser.add_argument('--steps', type=int, default=1,
                    help='execute this many consecutive action ticks per fixture')
parser.add_argument('--sequence-only', action='store_true',
                    help='use one coherent initial state for each selected character')
parser.add_argument('--idle-descriptor', choices=('base', 'alternate'),
                    help='complete selector 0x00 using the ordinary or role/flag alternate timing')
IDLE_EDGES = ('center', 'left', 'right', 'left-high', 'right-high',
              'apron-left', 'apron-right', 'outer-left', 'outer-right')
parser.add_argument('--idle-edge', choices=IDLE_EDGES,
                    help='start selector 0x00 at a specific ring boundary and check its entry response')
parser.add_argument('--interleave-physics', action='store_true',
                    help='run arcade $2208 and the port physics owner between action ticks')
parser.add_argument('--interleave-resolver', action='store_true',
                    help='run state-latch handoff and arcade/port arena resolver before each action')
parser.add_argument('--royal', action='store_true',
                    help='legacy variant: selects mode byte 0x01 for action 0x68, 0x02 for 0x69/0x23')
parser.add_argument('--raw35-mode', type=int, choices=range(4), default=0,
                    help='exact arcade mode byte for action 0x35: game bit 0x01 and scene bit 0x02')
parser.add_argument('--raw35-target-x', type=lambda value: int(value, 0), default=0x268,
                    help='opponent X for interleaved selector 0x35 sequence')
parser.add_argument('--raw35-active', action='store_true',
                    help='use an active selector-0x61 opponent for an interleaved selector 0x35 sequence')
parser.add_argument('--raw8e-phase', type=int, choices=range(4), default=0,
                    help='Royal interlude phase for an isolated multiframe sequence')
parser.add_argument('--raw44-index', type=int, choices=range(5), default=0,
                    help='probability-table index for the linked throw (selector 0x44)')
parser.add_argument('--raw44-force-fail', action='store_true',
                    help='choose a nonzero RNG seed that sends each linked throw to action 0x82')
parser.add_argument('--raw44-force-success', action='store_true',
                    help='set the arcade wrestler success bit for a deterministic linked throw')
parser.add_argument('--raw13-downed', action='store_true',
                    help='place a downed linked wrestler under the aerial attack')
parser.add_argument('--raw12-downed', action='store_true',
                    help='place a downed wrestler at the landing region of the multi-phase leap')
parser.add_argument('--raw12-active', action='store_true',
                    help='place an active selector-0x61 opponent at the landing region')
parser.add_argument('--follow-impact', action='store_true',
                    help='continue action 0x12/0x13 after the first successful contact')
parser.add_argument('--linked-downed', action='store_true',
                    help='place a downed linked opponent under action 0x11 or 0x14')
parser.add_argument('--linked-active', action='store_true',
                    help='place an active selector-0x61 opponent under action 0x11 or 0x14')
parser.add_argument('--victim-follow', type=int, default=0,
                    help='compare victim dispatches through the complete knockdown-to-idle handoff')
parser.add_argument('--arrived', action='store_true',
                    help='place the ring-exit destination at the current actor position')
parser.add_argument('--credited', action='store_true',
                    help='exercise credited TAG exchange and compare palette RAM')
parser.add_argument('--tag-special', action='store_true',
                    help='place the linked opponent in the credited TAG rescue window')
INTERRUPT_VARIANTS = ('ground8', 'ground9', 'hold48', 'hold0e',
    'hold1a', 'hold09', 'hold61', 'hold49', 'hold48-q49')
parser.add_argument('--interrupt-kind', choices=('none', 'all', *INTERRUPT_VARIANTS),
    default='none', help='select the linked action interrupted by selector 0x0a')
parser.add_argument('--interrupt-royal-credit', action='store_true',
                    help='exercise credited Royal cleanup at ROM 0x21358 after interrupting hold09')
JUMP_CONTACT_VARIANTS = ('ground8', 'ground9', 'counter61', 'miss')
parser.add_argument('--jump-contact', choices=('none', 'all', *JUMP_CONTACT_VARIANTS),
    default='none', help='select a contact outcome for selector 0x0c')
JUMP_ATTACK_VARIANTS = ('route0', 'route1', 'route2', 'miss')
parser.add_argument('--jump-attack', choices=('none', 'all', *JUMP_ATTACK_VARIANTS),
    default='none', help='select a contact outcome for selector 0x0d')
RUNNING_CONTACT_VARIANTS = ('hit', 'miss', 'special54')
parser.add_argument('--running-contact', choices=('none', 'all', *RUNNING_CONTACT_VARIANTS),
    default='none', help='select a contact outcome for selector 0x0e phase three')
JUMP10_VARIANTS = ('ground8', 'ground9', 'counter61', 'miss')
parser.add_argument('--jump10-contact', choices=('none', 'all', *JUMP10_VARIANTS),
    default='none', help='select a contact outcome for selector 0x10')
parser.add_argument('--object-release-only', action='store_true',
                    help='exercise the two object release branches of selector 0x1e')
SLAM_VARIANTS = ('wall-left', 'wall-right', 'landing')
parser.add_argument('--slam-kind', choices=('none', 'all', *SLAM_VARIANTS),
    default='none', help='select a wall or landing branch of selector 0x33')
parser.add_argument('--null-link', choices=('none', 'zero', 'tagged'), default='none',
                    help='exercise a literal or tagged null link in selector 0x37')
parser.add_argument('--tag-recovery-kind', choices=('none', 'credited', 'exit'), default='none',
                    help='exercise credited setup or the terminal recovery branch in selector 0x61')
parser.add_argument('--pin-credited', action='store_true',
                    help='exercise the credited TAG partner helpers in selector 0x48')
parser.add_argument('--splash-credited', action='store_true',
                    help='exercise the credited TAG handoff in selector 0x51')
parser.add_argument('--throw-credited', action='store_true',
                    help='exercise the credited TAG cleanup in selector 0x57')
parser.add_argument('--hold-zero-health', action='store_true',
                    help='exercise the zero-health hold timer in selector 0x5e')
parser.add_argument('--tag66-credited', action='store_true',
                    help='exercise the credited TAG exchange and palettes in selector 0x66')
parser.add_argument('--raw23-phase', choices=('all', *(str(i) for i in range(9))),
                    help='select the special action 0x23 phase (default: 6)')
parser.add_argument('--raw23-mode', type=int, choices=range(4),
                    help='exact arcade mode byte for action 0x23, including Royal bit 0x01')
parser.add_argument('--raw23-impact', action='store_true',
                    help='exercise the connected aerial impact in action 0x23 phase 8')
parser.add_argument('--raw23-pin', action='store_true',
                    help='exercise the credited TAG pin after action 0x23 impact')
parser.add_argument('--raw23-referee', action='store_true',
                    help='exercise the phase 5 contact handoff to the referee')
parser.add_argument('--raw23-contact3', action='store_true',
                    help='exercise the phase 5 contact-three fall and sounds')
parser.add_argument('--raw23-release', choices=('none', 'target', 'signal', 'wait'),
                    default='none', help='exercise the phase 8 credited pin release')
args = parser.parse_args()
if args.steps < 1:
    parser.error('--steps must be positive')
if args.idle_descriptor is not None and (args.selector != 0x00 or not args.sequence_only):
    parser.error('--idle-descriptor requires --selector 0x00 --sequence-only')
if args.idle_edge is not None and (args.selector != 0x00 or not args.sequence_only):
    parser.error('--idle-edge requires --selector 0x00 --sequence-only')
if args.interleave_physics and (not args.sequence_only or args.selector not in (0x0c, 0x0d, 0x0e, 0x10, 0x11, 0x12, 0x13, 0x14, 0x35)):
    parser.error('--interleave-physics requires --sequence-only and selector 0x0c/0x0d/0x0e/0x10/0x11/0x12/0x13/0x14/0x35')
if args.interleave_resolver and not (args.interleave_physics or
                                     (args.selector in (0x44, 0x8e) and args.sequence_only)):
    parser.error('--interleave-resolver requires --interleave-physics, or sequence-only selector 0x44/0x8e')
if args.credited and args.selector != 0x4c:
    parser.error('--credited is supported only for selector 0x4c')
if args.tag_special and (args.selector != 0x4c or not args.credited):
    parser.error('--tag-special requires --selector 0x4c --credited')
if args.interrupt_kind != 'none' and args.selector != 0x0a:
    parser.error('--interrupt-kind requires --selector 0x0a')
if args.interrupt_royal_credit and (args.selector != 0x0a or args.interrupt_kind != 'hold09'):
    parser.error('--interrupt-royal-credit requires --selector 0x0a --interrupt-kind hold09')
if args.jump_contact != 'none' and args.selector != 0x0c:
    parser.error('--jump-contact requires --selector 0x0c')
if args.jump_attack != 'none' and args.selector != 0x0d:
    parser.error('--jump-attack requires --selector 0x0d')
if args.running_contact != 'none' and args.selector != 0x0e:
    parser.error('--running-contact requires --selector 0x0e')
if args.jump10_contact != 'none' and args.selector != 0x10:
    parser.error('--jump10-contact requires --selector 0x10')
if args.object_release_only and args.selector != 0x1e:
    parser.error('--object-release-only requires --selector 0x1e')
if args.slam_kind != 'none' and args.selector != 0x33:
    parser.error('--slam-kind requires --selector 0x33')
if args.null_link != 'none' and args.selector != 0x37:
    parser.error('--null-link requires --selector 0x37')
if args.tag_recovery_kind != 'none' and args.selector != 0x61:
    parser.error('--tag-recovery-kind requires --selector 0x61')
if args.pin_credited and args.selector != 0x48:
    parser.error('--pin-credited requires --selector 0x48')
if args.splash_credited and args.selector != 0x51:
    parser.error('--splash-credited requires --selector 0x51')
if args.throw_credited and args.selector != 0x57:
    parser.error('--throw-credited requires --selector 0x57')
if args.hold_zero_health and args.selector != 0x5e:
    parser.error('--hold-zero-health requires --selector 0x5e')
if args.tag66_credited and args.selector != 0x66:
    parser.error('--tag66-credited requires --selector 0x66')
if args.raw35_mode and args.selector != 0x35:
    parser.error('--raw35-mode requires --selector 0x35')
if args.raw35_active and (args.selector != 0x35 or not args.sequence_only or not args.interleave_resolver):
    parser.error('--raw35-active requires --selector 0x35 --sequence-only --interleave-resolver')
if args.raw8e_phase and (args.selector != 0x8e or not args.sequence_only):
    parser.error('--raw8e-phase requires --selector 0x8e --sequence-only')
if args.raw44_index and args.selector != 0x44:
    parser.error('--raw44-index requires --selector 0x44')
if args.raw44_force_fail and (args.selector != 0x44 or not args.sequence_only or
                              args.raw44_force_success or args.raw44_index == 0):
    parser.error('--raw44-force-fail requires selector 0x44, index 1..4, sequence-only, and no forced success')
if args.raw44_force_success and (args.selector != 0x44 or not args.sequence_only):
    parser.error('--raw44-force-success requires --selector 0x44 --sequence-only')
if args.raw13_downed and (args.selector != 0x13 or not args.sequence_only):
    parser.error('--raw13-downed requires --selector 0x13 --sequence-only')
if args.raw12_downed and (args.selector != 0x12 or not args.sequence_only):
    parser.error('--raw12-downed requires --selector 0x12 --sequence-only')
if args.raw12_active and (args.selector != 0x12 or not args.sequence_only or args.raw12_downed):
    parser.error('--raw12-active requires --selector 0x12 --sequence-only without --raw12-downed')
if args.follow_impact and not (args.sequence_only and
                               (args.selector == 0x13 and args.raw13_downed or
                                args.selector == 0x12 and (args.raw12_downed or args.raw12_active))):
    parser.error('--follow-impact requires an action 0x12/0x13 contact fixture')
if args.linked_downed and (args.selector not in (0x11, 0x14) or not args.sequence_only):
    parser.error('--linked-downed requires selector 0x11/0x14 with --sequence-only')
if args.linked_active and (args.selector not in (0x11, 0x14) or not args.sequence_only or args.linked_downed):
    parser.error('--linked-active requires selector 0x11/0x14 with --sequence-only, without --linked-downed')
if args.victim_follow and (args.victim_follow < 0 or
                           not (args.selector in (0x11, 0x14) and args.linked_downed or
                                args.selector == 0x12 and args.raw12_downed and args.follow_impact or
                                args.selector == 0x13 and args.raw13_downed and args.follow_impact or
                                args.selector == 0x0c and args.jump_contact in ('ground8', 'ground9', 'counter61') or
                                args.selector == 0x10 and args.jump10_contact in ('ground8', 'ground9', 'counter61') or
                                args.selector == 0x0e and args.running_contact == 'hit' or
                                args.selector == 0x0d and args.jump_attack in ('route0', 'route1', 'route2') or
                                args.selector == 0x35 and not args.raw35_active and args.raw35_target_x == 0x2c0) or
                           not args.sequence_only or not args.interleave_resolver or
                           not args.interleave_physics):
    parser.error('--victim-follow requires a supported 0x0c/0x0d/0x0e/0x10/0x11/0x12/0x13/0x14/0x35 contact sequence with resolver and physics')
if args.royal and args.selector == 0x35:
    parser.error('use --raw35-mode for action 0x35; --royal sets a different byte value')
if args.raw23_phase is not None and args.selector != 0x23:
    parser.error('--raw23-phase requires --selector 0x23')
if args.raw23_mode is not None and args.selector != 0x23:
    parser.error('--raw23-mode requires --selector 0x23')
if args.raw23_mode is not None and args.royal:
    parser.error('--raw23-mode and legacy --royal cannot be combined')
if args.raw23_impact and (args.selector != 0x23 or args.raw23_phase != '8'):
    parser.error('--raw23-impact requires --selector 0x23 --raw23-phase 8')
if args.raw23_pin and not args.raw23_impact:
    parser.error('--raw23-pin requires --raw23-impact')
if args.raw23_referee and (args.selector != 0x23 or args.raw23_phase != '5'):
    parser.error('--raw23-referee requires --selector 0x23 --raw23-phase 5')
if args.raw23_contact3 and (args.selector != 0x23 or args.raw23_phase != '5'):
    parser.error('--raw23-contact3 requires --selector 0x23 --raw23-phase 5')
if args.raw23_release != 'none' and (args.selector != 0x23 or args.raw23_phase != '8'):
    parser.error('--raw23-release requires --selector 0x23 --raw23-phase 8')
if args.steps > 1 and (args.selector not in (0x00, 0x08, 0x0c, 0x0d, 0x0e, 0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x23, 0x24, 0x26, 0x29, 0x33, 0x35, 0x37, 0x44, 0x48, 0x4c, 0x4e, 0x55, 0x56, 0x57, 0x59, 0x5a, 0x5b, 0x66, 0x76, 0x77, 0x78, 0x7d, 0x7e, 0x7f, 0x80, 0x81, 0x82, 0x84, 0x85, 0x86, 0x87, 0x89, 0x8e, 0x7b, 0x7c) or not args.sequence_only):
    parser.error('multiframe comparison requires a supported selector with --sequence-only')
raw23_mode = args.raw23_mode if args.raw23_mode is not None else (2 if args.royal else 0)
rom = (ROOT / 'audit/runtime18-cd/ROM68K.BIN').read_bytes()
assert hashlib.sha1(rom).hexdigest() == 'a726c46f350723ea73158bc78055d17d418b14e2'
for selector, (descriptor, _) in FAMILY.items():
    assert int.from_bytes(rom[0x12614 + selector * 4:0x12618 + selector * 4], 'big') == descriptor

rng = random.Random(0x1a60480)
def put(mem, address, value, n=2):
    mem[address:address+n] = (value & ((1 << (8*n))-1)).to_bytes(n, 'big')

def force_raw44_miss(mem, regs, cpu, memory, slot):
    # Fixture selection consults the untouched arcade ROM. The chosen image
    # is reloaded before the actual differential trajectory starts.
    probe = random.Random(0x44a82 + slot)
    observed = []
    for _ in range(512):
        seed = probe.getrandbits(32) or 1
        put(mem, RAM+0x5c, seed, 4)
        memory.w_block(0, bytes(mem))
        cpu.w_sp(0x1d0000)
        memory.w32(0x1d0000, 0x70000)
        cpu.w_pc(0x1c03e)
        cpu.w_reg(8, ACTORS[slot])
        for index, value in enumerate(regs):
            cpu.w_reg(index, value)
        for _ in range(20000):
            if cpu.r_pc() == 0x70000:
                break
            cpu.execute(1)
        assert cpu.r_pc() == 0x70000, ('linked-throw seed probe runaway', seed)
        if len(observed) < 4:
            observed.append((hex(seed), hex(memory.r32(RAM+0x5c)),
                             hex(memory.r16(ACTORS[slot]+0x60)),
                             memory.r16(ACTORS[slot]+0xf2)))
        if memory.r16(ACTORS[slot]+0x60) & 0xff == 0x82:
            return seed
    raise AssertionError(('no linked-throw miss seed', observed))

def fixture(selector, slot, latch, counter, frame, timer, facing, character=None,
            interrupt_kind=None, jump_contact=None, jump_attack=None,
            running_contact=None, jump10_contact=None, slam_kind=None):
    interrupt_kind = interrupt_kind or args.interrupt_kind
    jump_contact = jump_contact or args.jump_contact
    jump_attack = jump_attack or args.jump_attack
    running_contact = running_contact or args.running_contact
    jump10_contact = jump10_contact or args.jump10_contact
    slam_kind = slam_kind or args.slam_kind
    mem = bytearray(0x200000)
    mem[:len(rom)] = rom
    mem[RAM:RAM+0x4000] = rng.randbytes(0x4000)
    for i, address in enumerate(ACTORS):
        put(mem, address, 0x8000)
        put(mem, address+2, i % 12)
        put(mem, address+0x20, 0x8005)
        put(mem, address+0x26, ACTORS[(i+1)%9], 4)
    actor = ACTORS[slot]
    if character is not None:
        put(mem, actor+2, character)
        put(mem, ACTORS[(slot+1)%9]+2, (character+1)%12)
    if selector in (0x11, 0x14, 0x35, 0x55, 0x56, 0x5a, 0x5b):
        put(mem, RAM+0x7c, 0)
        put(mem, RAM+0x161, args.raw35_mode if selector == 0x35 else 0, 1)
    if selector in (0x0c, 0x0d, 0x0e, 0x10, 0x11, 0x12, 0x13, 0x14, 0x35) and args.sequence_only and args.interleave_resolver:
        # Start on a valid, unobstructed ring surface; random scratch bytes
        # in these fields would turn the trial into an invalid arena mode.
        put(mem, RAM+0x7e, 0)
        put(mem, actor+1, 0, 1)
        put(mem, actor+0x32, 0, 1)
        for off in (0x3e, 0x40, 0x42):
            put(mem, actor+off, 0)
    if selector == 0x35:
        # Isolate the action state machine; effect 15 gets its own queue test.
        for i in range(11):
            put(mem, RAM+0x1258+42*i, 0x80, 1)
    if selector == 0x44:
        put(mem, RAM+0x161, 0, 1)
        put(mem, RAM+0x162, 0)
        put(mem, RAM+0x7c, 0)
        if args.raw44_force_success:
            # The original $1129E regards the attacker's +$56 bit 7 as a
            # guaranteed success; keep the other RNG inputs unchanged.
            put(mem, actor+0x56, mem[actor+0x56]|0x80, 1)
        if args.raw44_force_fail:
            # The attacker's +$56 bit 7 makes $1129E succeed without rolling.
            # The linked target's bit 7 selects a second chance table; clear
            # it to make index 1's 14/16 threshold capable of failing.
            put(mem, actor+0x56, mem[actor+0x56]&0x7f, 1)
            target = ACTORS[(slot+1)%9]
            put(mem, target+0x56, mem[target+0x56]&0x7f, 1)
        if args.sequence_only and args.interleave_resolver:
            put(mem, RAM+0x7e, 0)
            put(mem, actor+0x32, 0, 1)
            for off in (0x3e, 0x40, 0x42):
                put(mem, actor+off, 0)
    if selector == 0x4e and args.sequence_only:
        # Exercise both apron sides and each stamina tier on legal input.
        variant = character if character is not None else 0
        put(mem, actor+0x33, (mem[actor+0x33]&0x7f)|(0x80 if variant&1 else 0), 1)
        put(mem, actor+0x34, mem[actor+0x34]&0xfe, 1)
        put(mem, actor+0x66, (20, 60, 100)[variant%3])
        put(mem, actor+0x72, 100)
    if selector == 0x8e:
        put(mem, RAM+0x92, ACTORS[(slot+1)%9], 4)
        put(mem, RAM+0x15d4, 0)
        if args.sequence_only:
            # Let the interlude pass its wait-for-idle gate on phase two.
            put(mem, RAM+0x15d5, 0)
            put(mem, RAM+0x16c4, 0)
            put(mem, ACTORS[(slot+1)%9]+0x20, 0)
        if slot == 0:
            put(mem, RAM+0x1610+31, 0x80, 1)
        elif slot == 1:
            mem[RAM+0x1600:RAM+0x1610] = b'\0'*16
            put(mem, RAM+0x1610+31, 0, 1)
        else:
            mem[RAM+0x1600:RAM+0x1610] = b'\xff'*16
            put(mem, RAM+0x1610+31, 0, 1)
        mem[0xc0000:0xc2000] = rng.randbytes(0x2000)
    if selector in (0x00, 0x11, 0x12, 0x13, 0x14, 0x35):
        for i, at in enumerate(ACTORS):
            put(mem, at+6, 0x260+i*8)
            put(mem, at+10, 0x160)
            put(mem, at+14, 0x140)
        if selector in (0x11, 0x14) and args.linked_downed:
            victim = ACTORS[(slot+1)%9]
            put(mem, victim+0x20, 4)
            put(mem, victim+0x64, 8)
            put(mem, victim+0x2e, 0)
        if selector in (0x11, 0x14) and args.linked_active:
            victim = ACTORS[(slot+1)%9]
            put(mem, victim+0x20, 5)
            put(mem, victim+0x60, 0x61)
            put(mem, victim+0x64, 0)
            put(mem, victim+0x2e, 0)
        if selector == 0x13 and args.raw13_downed:
            victim = ACTORS[(slot+1)%9]
            put(mem, victim+0x20, 4)
            put(mem, victim+0x64, 8)
        if selector == 0x12 and args.raw12_downed:
            victim = ACTORS[(slot+1)%9]
            put(mem, victim+0x20, 4)
            put(mem, victim+0x64, 8)
            put(mem, victim+6, 0x2a0)
        if selector == 0x12 and args.raw12_active:
            victim = ACTORS[(slot+1)%9]
            put(mem, victim+0x20, 5)
            put(mem, victim+0x60, 0x61)
            put(mem, victim+6, 0x2a0)
        if selector == 0x35 and args.sequence_only and args.interleave_resolver:
            victim = ACTORS[(slot+1)%9]
            put(mem, victim+6, args.raw35_target_x)
            put(mem, victim+0x20, 5 if args.raw35_active else 4)
            put(mem, victim+0x60, 0x61 if args.raw35_active else 0)
            put(mem, victim+0x64, 0 if args.raw35_active else 8)
            put(mem, victim+0x26, actor, 4)
            put(mem, victim+0x2e, 0)
    if selector == 0x00:
        # The unlatched idle action calls the arena boundary resolver.  Keep
        # its global arena mode and actor movement flags in a legal state.
        put(mem, RAM+0x7e, 0)
        put(mem, actor+0x32, 0)
        put(mem, actor+0x36, 0)
        put(mem, actor+0x3e, 0)
        put(mem, actor+0x40, 0)
        put(mem, actor+0x42, 0)
        put(mem, actor+0x56, 0x80 if args.idle_descriptor == 'alternate' else 0, 1)
        put(mem, actor+0x33, 0x20 if args.idle_descriptor == 'alternate' else 0, 1)
        if args.sequence_only:
            edge = args.idle_edge or 'center'
            x = {'center': 0x280, 'left': 0x180, 'right': 0x400,
                 'left-high': 0x180, 'right-high': 0x400,
                 'apron-left': 0x140, 'apron-right': 0x500,
                 'outer-left': 0x250, 'outer-right': 0x400}[edge]
            put(mem, actor+6, x)
            put(mem, actor+10, 0x160)
            put(mem, actor+14, 0x190 if edge.endswith('high') else 0x140)
            put(mem, actor+0x58, 0x1234)
            put(mem, RAM+0x7e, 2 if edge.startswith('outer') else 0)
            if edge.startswith('apron'):
                put(mem, actor+0x33, mem[actor+0x33] | 4, 1)
    if selector == 0x70:
        # The entrance action follows a real fixed object at +$76.
        fixed = RAM+0xf1c
        put(mem, actor+0x76, fixed, 4)
        if latch == 0x8005 and frame == 0xfe:
            # Terminal entry must skip the alternate descriptor even when
            # this byte would otherwise select it.
            put(mem, actor+0x75, 1, 1)
        put(mem, fixed+6, 0x260)
        put(mem, fixed+10, 0x160)
        put(mem, fixed+14, 0x140)
    if selector == 0x6b:
        put(mem, RAM+0x161, 0, 1)
        put(mem, actor+6, 0x260)
        put(mem, actor+10, 0x160)
        put(mem, actor+14, 0x140)
        put(mem, actor+0xbe, 0x260 if args.arrived and latch == 0x8005 else 0x42c)
        put(mem, actor+0xc0, 0x160 if args.arrived and latch == 0x8005 else 0x120)
    if selector == 0x69:
        put(mem, RAM+0x161, 2 if args.royal else 0, 1)
        put(mem, RAM+0x1828, 0)
        put(mem, actor+6, 0x260)
        put(mem, actor+10, 0x160)
        put(mem, actor+14, 0x140)
        put(mem, actor+0x74, 0)
    if selector == 0x68:
        put(mem, RAM+0x161, 1 if args.royal else 0, 1)
        put(mem, actor+6, 0x260)
        put(mem, actor+10, 0x160)
        put(mem, actor+14, 0x140)
        put(mem, actor+0xbe, 0x260 if args.arrived and latch == 0x8005 else 0x42c)
        put(mem, actor+0xc0, 0x160 if args.arrived and latch == 0x8005 else 0x120)
    if selector == 0x08:
        put(mem, RAM+0x7e, 0)
        put(mem, RAM+0x161, 0, 1)
        for i, at in enumerate(ACTORS):
            put(mem, at+6, 0x260+i*8)
            put(mem, at+10, 0x160)
            put(mem, at+14, 0x140)
        target = ACTORS[(slot+1)%9]
        put(mem, target+0x20, 4 if slot == 0 else 5 if slot == 1 else 0)
        put(mem, target+0x64, 8)
    if selector == 0x24:
        put(mem, RAM+0x7e, 0)
        put(mem, RAM+0x161, 0, 1)
        for i, at in enumerate(ACTORS):
            put(mem, at+6, 0x260+i*8)
            put(mem, at+10, 0x160)
            put(mem, at+14, 0x140)
        target = ACTORS[(slot+1)%9]
        put(mem, target+0x86, ACTORS[(slot+2)%9], 4)
    if selector in (0x15, 0x16):
        put(mem, RAM+0x7e, 0)
        put(mem, RAM+0x161, 0, 1)
        for i, at in enumerate(ACTORS):
            put(mem, at+6, 0x260+i*8)
            put(mem, at+10, 0x160)
            put(mem, at+14, 0x140)
        put(mem, actor+0x40, 0)
        put(mem, actor+0x42, 0)
        if selector == 0x15 and latch == 5 and counter == 2:
            # $14D14/$14D16 explicitly clear D1/D2 on this hold step.
            put(mem, actor+0x1f, 0x0c, 1)
        if selector == 0x16 and args.sequence_only:
            put(mem, actor+0x2c, 0x40)
            put(mem, actor+0xfe, 0)
    if selector == 0x17:
        put(mem, RAM+0x7e, 0)
        put(mem, RAM+0x161, 0, 1)
        for i, at in enumerate(ACTORS):
            put(mem, at+6, 0x260+i*8)
            put(mem, at+10, 0x160)
            put(mem, at+14, 0x140)
        put(mem, actor+0x40, 0)
        put(mem, actor+0x42, 0)
        put(mem, actor+0x36, 0x1234)
    if selector == 0x26:
        put(mem, RAM+0x7e, 0)
        put(mem, RAM+0x161, 0, 1)
        put(mem, RAM+0x162, 0)
        for i, at in enumerate(ACTORS):
            put(mem, at+6, 0x260+i*8)
            put(mem, at+10, 0x160)
            put(mem, at+14, 0x140)
        target = ACTORS[(slot+1)%9]
        put(mem, target+0x86, ACTORS[(slot+2)%9], 4)
        put(mem, actor+0xf8, 0)
    if selector == 0x4c:
        # Exercise TAG exchange with and without the credited palette route.
        put(mem, RAM+0x7c, 1 if args.credited else 0)
        put(mem, RAM+0x161, 0, 1)
        put(mem, actor+0x86, ACTORS[(slot+2)%9], 4)
        put(mem, actor+0x7a, ACTORS[(slot+1)%9], 4)
        if args.credited:
            for i, at in enumerate(ACTORS):
                put(mem, at+0x56, i%12)
            put(mem, ACTORS[(slot+1)%9]+0x70, 0)
            put(mem, ACTORS[(slot+2)%9]+0x70, 0)
            mem[0x180000:0x184000] = rng.randbytes(0x4000)
        if args.tag_special:
            target = ACTORS[(slot+1)%9]
            put(mem, actor+0x33, 0, 1)
            put(mem, target+0x20, 4)
            put(mem, target+0x64, 8)
            put(mem, target+6, 0x220)
            put(mem, target+10, 0x180)
            put(mem, target+0x2e, 0)
    if selector == 0x0a:
        put(mem, RAM+0x7c, 1 if args.interrupt_royal_credit else 0)
        put(mem, RAM+0x161, 1 if args.interrupt_royal_credit else 0, 1)
        target = ACTORS[(slot+1)%9]
        partner = ACTORS[(slot+2)%9]
        if args.interrupt_royal_credit:
            candidate = ACTORS[(slot+3)%9]
            second = ACTORS[(slot+4)%9]
            put(mem, partner+0x60, 0x53 if slot == 1 else 0x15)
            put(mem, partner+0x7e, candidate, 4)
            put(mem, partner+0xb6, 0, 1)
            put(mem, candidate+0xb5, 0x80, 1)
            put(mem, candidate+0xb6, 0, 1)
            put(mem, candidate+0x7e, second, 4)
            put(mem, second+0xb5, 0x80, 1)
            put(mem, second+0xb6, 0, 1)
        put(mem, target+0xfe, 0)
        put(mem, target+6, 0x280)
        put(mem, target+10, 0x160)
        put(mem, target+14, 0x140)
        put(mem, partner+6, 0x260)
        put(mem, partner+10, 0x160)
        put(mem, partner+14, 0x140)
        if interrupt_kind.startswith('ground'):
            put(mem, target+0x20, 4)
            put(mem, target+0x64, int(interrupt_kind[-1]))
        elif interrupt_kind != 'none':
            selected = {'hold48': 0x8048, 'hold0e': 0x800e,
                        'hold1a': 0x801a, 'hold09': 0x8009,
                        'hold61': 0x8061, 'hold49': 0x8049,
                        'hold48-q49': 0x8048}[interrupt_kind]
            put(mem, target+0x20, 5)
            put(mem, target+0x60, selected)
            put(mem, target+0x9c, 0, 4)
            if interrupt_kind == 'hold48-q49':
                alternate = ACTORS[(slot+3)%9]
                put(mem, target+0x9c, alternate, 4)
                put(mem, alternate+0x20, 5)
                put(mem, alternate+0x60, 0x49)
    if selector == 0x0c:
        put(mem, RAM+0x7e, 0)
        put(mem, RAM+0x161, 0, 1)
        for i, at in enumerate(ACTORS):
            put(mem, at+6, 0x260+i*8)
            put(mem, at+10, 0x160)
            put(mem, at+14, 0x140)
        put(mem, actor+0x40, 0)
        put(mem, actor+0x42, 0)
        put(mem, actor+0x36, 0)
        target = ACTORS[(slot+1)%9]
        put(mem, target+0x20, 5 if slot == 0 else 4)
        put(mem, target+0x60, 0x61 if slot == 0 else 0)
        if jump_contact != 'none':
            put(mem, actor+6, 0x280)
            put(mem, actor+0x37, 0x10, 1)
            put(mem, target+6, 0x260 if jump_contact != 'miss' else 0x400)
            put(mem, target+0x2e, 0)
            if jump_contact.startswith('ground'):
                put(mem, target+0x20, 4)
                put(mem, target+0x64, int(jump_contact[-1]))
            else:
                put(mem, target+0x20, 5)
                put(mem, target+0x60, 0x61 if jump_contact == 'counter61' else 0)
    if selector == 0x0d:
        put(mem, RAM+0x7e, 0)
        put(mem, RAM+0x161, 0, 1)
        for i, at in enumerate(ACTORS):
            put(mem, at+6, 0x260+i*8)
            put(mem, at+10, 0x160)
            put(mem, at+14, 0x140)
        put(mem, actor+0x34, 8 if slot == 8 else 0, 1)
        put(mem, actor+0x37, 0, 1)
        put(mem, actor+0x40, 0)
        put(mem, actor+0x42, 0)
        target = ACTORS[(slot+1)%9]
        put(mem, target+0x20, 5 if slot == 0 else 0)
        put(mem, target+0x60, 0x53 if slot == 0 else 0)
        if jump_attack != 'none':
            put(mem, actor+0x37, 0x10, 1)
            put(mem, actor+6, 0x280)
            if jump_attack == 'route0' and args.sequence_only and args.interleave_resolver:
                put(mem, actor+0x34, 8, 1)
            put(mem, target+6, 0x400 if jump_attack == 'miss' else 0x280)
            put(mem, target+10, 0x160)
            if jump_attack == 'miss' and args.sequence_only and args.interleave_resolver:
                put(mem, target+6, 0x280)
                put(mem, target+10, 0x160)
            put(mem, target+0x32, 0, 1)
            put(mem, target+0x74, 0)
            put(mem, target+0x20, 5 if jump_attack == 'route1' else 0)
            put(mem, target+0x60, 0x53 if jump_attack == 'route1' else 0)
            if jump_attack == 'miss' and args.sequence_only and args.interleave_resolver:
                put(mem, target+0x20, 4)
                put(mem, target+0x64, 8)
            if jump_attack == 'route0':
                first = ACTORS[(slot+2)%9]
                linked = ACTORS[(slot+3)%9]
                second = ACTORS[(slot+4)%9]
                put(mem, actor+0x86, first, 4)
                put(mem, first+0x26, linked, 4)
                put(mem, linked+0x86, second, 4)
                put(mem, second+0x70, 0)
    if selector == 0x0e:
        put(mem, RAM+0x7c, 0)
        put(mem, RAM+0x7e, 0)
        put(mem, RAM+0x161, 0, 1)
        for i, at in enumerate(ACTORS):
            put(mem, at+6, 0x260+i*8)
            put(mem, at+10, 0x160)
            put(mem, at+14, 0x140)
        put(mem, actor+0x36, 0)
        put(mem, actor+0x40, 0)
        put(mem, actor+0x42, 0)
        put(mem, actor+0xbe, 0x280)
        target = ACTORS[(slot+1)%9]
        put(mem, target+0x66, 48)
        put(mem, target+0xdc, 0)
        if running_contact != 'none':
            put(mem, actor+0x37, 0x10, 1)
            put(mem, actor+6, 0x280)
            put(mem, target+6, 0x260 if running_contact != 'miss' else 0x400)
            if running_contact == 'miss' and args.sequence_only and args.interleave_resolver:
                # Keep the target inside the ring for a complete trajectory;
                # the one-tick miss fixture's x=$400 is beyond the phase-2
                # destination reachable under arena collision.
                put(mem, target+6, 0x260)
                put(mem, target+10, 0x118)
                put(mem, target+0x60, 0)
            put(mem, target+0x2e, 0)
            if running_contact == 'special54':
                put(mem, target+0x20, 5)
                put(mem, target+0x60, 0x54)
            else:
                put(mem, target+0x20, 4)
                put(mem, target+0x64, 8)
                if running_contact == 'hit':
                    put(mem, target+0x26, actor, 4)
    if selector == 0x10:
        put(mem, RAM+0x7e, 0)
        put(mem, RAM+0x161, 0, 1)
        for i, at in enumerate(ACTORS):
            put(mem, at+6, 0x260+i*8)
            put(mem, at+10, 0x160)
            put(mem, at+14, 0x140)
        put(mem, actor+0x36, 0)
        put(mem, actor+0x40, 0)
        put(mem, actor+0x42, 0)
        target = ACTORS[(slot+1)%9]
        put(mem, target+0x20, 5 if slot == 0 else 4)
        put(mem, target+0x60, 0x61 if slot == 0 else 0)
        put(mem, target+0x64, 8 if slot == 1 else 0)
        if jump10_contact != 'none':
            put(mem, actor+0x37, 0x10, 1)
            put(mem, actor+6, 0x280)
            put(mem, target+6, 0x260 if jump10_contact != 'miss' else 0x400)
            if jump10_contact == 'miss' and args.sequence_only and args.interleave_resolver:
                put(mem, target+6, 0x260)
                put(mem, target+10, 0x118)
                put(mem, target+0x60, 0)
            put(mem, target+0x2e, 0)
            if jump10_contact.startswith('ground'):
                put(mem, target+0x20, 4)
                put(mem, target+0x64, int(jump10_contact[-1]))
            else:
                put(mem, target+0x20, 5)
                put(mem, target+0x60, 0x61 if jump10_contact == 'counter61' else 0)
    if args.victim_follow:
        # A state-4 owner needs a coherent animation latch and frame; the
        # ordinary attacker-only fixture leaves them random. The attacker's
        # random busy flag can otherwise hold recovery forever.
        victim = ACTORS[(slot+1)%9]
        counter_follow = selector == 0x0c and jump_contact == 'counter61' or (
            selector == 0x10 and jump10_contact == 'counter61')
        initial_victim_state = int.from_bytes(mem[victim+0x20:victim+0x22], 'big') & 0xff
        put(mem, actor+0x34, 8 if selector == 0x0d and jump_attack == 'route0' else 0, 1)
        put(mem, victim+0x1c, initial_victim_state)
        put(mem, victim+0x1e, initial_victim_state)
        put(mem, victim+1, 0, 1)
        put(mem, victim+0x22, 0)
        put(mem, victim+0x24, 0)
        put(mem, victim+0x32, 0, 1)
        put(mem, victim+0x34, 0, 1)
        for off in (0x3e, 0x40, 0x42):
            put(mem, victim+off, 0)
        put(mem, victim+0x44, 0)
        put(mem, victim+0x9a, 0)
        put(mem, victim+0xaa, 0)
        put(mem, victim+0xfe, 0)
        put(mem, victim+0x7a, actor, 4)
        put(mem, victim+0x92, actor, 4)
        if counter_follow:
            put(mem, victim+0x26, actor, 4)
            put(mem, victim+0x64, 0)
    if selector == 0x1e:
        put(mem, actor+0x92, 1 if slot == 1 else 0, 4)
        put(mem, actor+0x74, 1 if slot == 8 else 0)
        put(mem, actor+0x76, ACTORS[(slot+1)%9], 4)
    if selector == 0x33:
        put(mem, RAM+0x7e, 0)
        put(mem, RAM+0x161, 0, 1)
        put(mem, RAM+0x162, 0)
        for i, at in enumerate(ACTORS):
            put(mem, at+6, 0x260+i*8)
            put(mem, at+10, 0x160)
            put(mem, at+14, 0x140)
        put(mem, actor+0x32, 0)
        put(mem, actor+0x36, 0)
        put(mem, actor+0x40, 0)
        put(mem, actor+0x42, 0)
        put(mem, actor+0xf6, 0)
        target = ACTORS[(slot+1)%9]
        put(mem, target+0x86, ACTORS[(slot+2)%9], 4)
        if slam_kind == 'wall-left':
            put(mem, actor+6, 0x120)
        elif slam_kind == 'wall-right':
            put(mem, actor+6, 0x420)
        elif slam_kind == 'landing':
            put(mem, actor+6, 0x260)
            put(mem, actor+0x37, 0x10, 1)
    if selector == 0x37:
        put(mem, actor+0x86, ACTORS[(slot+2)%9], 4)
        for i, at in enumerate(ACTORS):
            put(mem, at+6, 0x260+i*8)
            put(mem, at+10, 0x160)
            put(mem, at+14, 0x140)
        partner = ACTORS[(slot+2)%9]
        put(mem, partner+0x32, 0 if slot == 0 else 2, 1)
    if selector == 0x61:
        # Vary the referee/assist flags while keeping the recovery timer
        # and TAG partner tier meaningful for both credited routes.
        put(mem, RAM+0x7c, 1 if args.tag_recovery_kind == 'credited' else 0)
        put(mem, RAM+0x161, 0, 1)
        put(mem, actor+0x56, (0, 0x80, 0xc0)[(0, 1, 8).index(slot)], 1)
        put(mem, actor+0xa3, 1, 1)
        put(mem, actor+0xaa, 5)
        partner = ACTORS[(slot+2)%9]
        put(mem, actor+0x86, partner, 4)
        put(mem, partner+0x70, (0, 1, 2)[(0, 1, 8).index(slot)])
    if selector == 0x59:
        put(mem, RAM+0x7c, 0)
        put(mem, RAM+0x161, 0, 1)
        partner = ACTORS[(slot+2)%9]
        target = ACTORS[(slot+1)%9]
        put(mem, actor+0x86, partner, 4)
        put(mem, target+0x86, partner, 4)
    if selector == 0x3d:
        put(mem, RAM+0x7c, 0)
        put(mem, actor+0x7a, ACTORS[(slot+1)%9], 4)
    if selector == 0x4d:
        # The transient effect allocator has its own ROM oracle. Keep its
        # compact slots occupied so this matrix isolates TAG entry movement.
        for i in range(11):
            put(mem, RAM+0x1258+42*i, 0x80, 1)
    if selector in (0x4a, 0x4b):
        put(mem, RAM+0x7c, 0)
        put(mem, RAM+0x161, 0, 1)
        target = ACTORS[(slot+1)%9]
        partner = ACTORS[(slot+2)%9]
        put(mem, actor+0x86, partner, 4)
        put(mem, actor+0x9c, 0, 4)
        put(mem, partner+0x34, (0, 0x40, 0x80)[(0, 1, 8).index(slot)], 1)
        put(mem, partner+0x7a, ACTORS[(slot+3)%9], 4)
        put(mem, target+0x86, ACTORS[(slot+3)%9], 4)
        if selector == 0x4a:
            put(mem, target+0x20, 5)
            put(mem, target+0x60, (0x48, 0x0e, 0x0f)[(0, 1, 8).index(slot)])
            put(mem, actor+0x56, 0, 1)
            put(mem, actor+0x32, 0x10 if slot != 0 else 0, 1)
            put(mem, actor+0xfe, 0)
    if selector == 0x48:
        put(mem, RAM+0x7c, 1 if args.pin_credited else 0)
        put(mem, RAM+0x7e, 0)
        put(mem, RAM+0x161, 0, 1)
        for i, at in enumerate(ACTORS):
            put(mem, at+6, 0x260+i*8)
            put(mem, at+10, 0x160)
            put(mem, at+14, 0x140)
        target = ACTORS[(slot+1)%9]
        partner = ACTORS[(slot+2)%9]
        put(mem, target+0x20, 5)
        put(mem, target+0x60, 0 if slot == 1 else 0x4a)
        if slot != 1:
            put(mem, target+0x26, actor, 4)
        put(mem, actor+0x86, partner, 4)
        put(mem, target+0x86, partner, 4)
        put(mem, actor+0x32, 0, 1)
        put(mem, actor+0x36, 0)
        put(mem, actor+0x37, 0 if slot == 1 else 0x10, 1)
        put(mem, actor+0x40, 0)
        put(mem, actor+0x42, 0)
        if args.pin_credited:
            put(mem, actor+0x33, 0 if slot == 1 else 1, 1)
            put(mem, target+0x32, 0, 1)
            put(mem, target+0x33, 0 if slot == 1 else 1, 1)
            put(mem, partner+0x32, 1 if slot == 8 else 0, 1)
            put(mem, partner+0x33, 0, 1)
            put(mem, partner+0x74, 0, 1)
    if selector == 0x51:
        put(mem, RAM+0x7c, 1 if args.splash_credited else 0)
        target = ACTORS[(slot+1)%9]
        put(mem, target+0x60, (0x0e, 0x48, 0x4a)[(0, 1, 8).index(slot)])
        put(mem, actor+0x64, 8 if slot == 0 else 1)
        put(mem, actor+0x37, 0x10 if slot == 0 else 0, 1)
        if args.splash_credited:
            partner = ACTORS[(slot+2)%9]
            put(mem, actor+0x33, 1, 1)
            put(mem, target+0x33, 1, 1)
            put(mem, target+0x26, actor, 4)
            put(mem, actor+0x86, partner, 4)
            put(mem, target+0x86, partner, 4)
            put(mem, partner+0x32, 1 if slot == 8 else 0, 1)
            put(mem, partner+0x33, 0, 1)
            put(mem, partner+0x74, 0, 1)
    if selector == 0x57:
        put(mem, RAM+0x7c, 1 if args.throw_credited else 0)
        put(mem, RAM+0x161, 0, 1)
        target = ACTORS[(slot+1)%9]
        put(mem, target+10, 0x160)
        put(mem, actor+0x33, 0x40, 1)
        put(mem, target+0x33, 0x40, 1)
        if args.throw_credited:
            partner = ACTORS[(slot+2)%9]
            put(mem, actor+0x33, 1, 1)
            put(mem, target+0x33, 1, 1)
            put(mem, actor+0x86, partner, 4)
            put(mem, target+0x86, partner, 4)
            put(mem, partner+0x32, 1, 1)
            put(mem, partner+0x33, 0, 1)
            put(mem, partner+0x34, 0x25, 1)
            put(mem, partner+0x74, 0, 1)
    if selector == 0x76:
        # The temporary splash sprite is checked with a render-enabled ROM
        # oracle; occupy compact slots here to isolate reaction state changes.
        for i in range(11):
            put(mem, RAM+0x1258+42*i, 0x80, 1)
    if selector in (0x65, 0x73, 0x74):
        # Their compact effects have a separate render-enabled ROM oracle.
        for i in range(11):
            put(mem, RAM+0x1258+42*i, 0x80, 1)
    if selector == 0x5e:
        put(mem, RAM+0x7c, 0)
        put(mem, RAM+0x161, 0, 1)
        partner = ACTORS[(slot+2)%9]
        put(mem, actor+0x86, partner, 4)
        put(mem, partner+0x34, (0, 0x40, 0x80)[(0, 1, 8).index(slot)], 1)
        put(mem, partner+0x7a, ACTORS[(slot+3)%9], 4)
        put(mem, actor+0x56, 0, 1)
        put(mem, actor+0x66, 0 if args.hold_zero_health else 30)
        put(mem, actor+0xa3, 1, 1)
        put(mem, actor+0xaa, 5)
    if selector in (0x81, 0x84, 0x85):
        for i, at in enumerate(ACTORS):
            put(mem, at+6, 0x260+i*8)
            put(mem, at+10, 0x160)
            put(mem, at+14, 0x140)
    if selector in (0x8b, 0x8c):
        put(mem, RAM+0x161, 0, 1)
        target = ACTORS[(slot+1)%9]
        put(mem, actor+0x86, ACTORS[(slot+2)%9], 4)
        put(mem, actor+0x7a, target, 4)
        put(mem, target+0x86, ACTORS[(slot+3)%9], 4)
        if selector == 0x8c:
            put(mem, actor+0xff, (3, 5, 0)[(0, 1, 8).index(slot)], 1)
    if selector == 0x71:
        for i in range(11):
            put(mem, RAM+0x1258+42*i, 0x80, 1)
        put(mem, actor+0x34, 0 if slot == 1 else 1, 1)
        put(mem, actor+0xe6, 1 if slot == 0 else 5 if slot == 8 else 0)
        put(mem, actor+0xc6, 0, 1)
        put(mem, actor+0x33, 0x80 if slot == 8 else 0, 1)
    if selector == 0x66:
        put(mem, RAM+0x7c, 1 if args.tag66_credited else 0)
        put(mem, RAM+0x161, 0, 1)
        if args.tag66_credited:
            for i, at in enumerate(ACTORS[:4]):
                put(mem, at+0x56, i)
            mem[0x180000:0x184000] = rng.randbytes(0x4000)
        partner = ACTORS[(slot+2)%9]
        put(mem, actor+0x86, partner, 4)
        put(mem, actor+0x33, 0x80 if slot == 1 else 1 if slot == 8 else 0, 1)
        put(mem, actor+0x56, 0x80 if slot == 1 else 0, 1)
        put(mem, actor+0x8a, 0x140020, 4)
        put(mem, partner+0x33, 0, 1)
        put(mem, partner+0x34, 7, 1)
        put(mem, partner+0x66, 50)
        put(mem, partner+0x72, 100)
        put(mem, partner+0x8a, 0x140022, 4)
        put(mem, partner+0xe6, 33)
    if selector == 0x77:
        put(mem, RAM+0x7c, 0)
        put(mem, RAM+0x161, 0, 1)
        partner = ACTORS[(slot+2)%9]
        target = ACTORS[(slot+1)%9]
        put(mem, actor+0x86, partner, 4)
        put(mem, target+0x86, partner, 4)
    if selector == 0x23:
        # Keep trajectory division in the valid 68000 range. Random actor
        # coordinates can overflow DIVS before the owner reaches its action.
        put(mem, RAM+0x161, raw23_mode, 1)
        put(mem, actor+0x2c, 0)
        target = ACTORS[(slot+1)%9]
        for at in (actor, target):
            put(mem, at+6, 0x280)
            put(mem, at+0xa, 0x100)
            put(mem, at+0xe, 0)
        put(mem, target+0x26, actor, 4)
        put(mem, target+0x2e, facing)
        if args.raw23_impact:
            put(mem, actor+0x37, 0x10, 1)
            put(mem, actor+0x33, 0, 1)
            put(mem, target+0x20, 4)
            put(mem, target+0x64, 8)
            if args.raw23_pin:
                put(mem, RAM+0x7c, 1)
                partner = ACTORS[(slot+2)%9]
                put(mem, actor+0x86, partner, 4)
                put(mem, actor+0x32, 0, 1)
                put(mem, actor+0x33, 1, 1)
                put(mem, target+0x33, 1, 1)
                put(mem, partner+0x32, 0, 1)
                put(mem, partner+0x33, 0, 1)
                put(mem, partner+0x70, 0)
                put(mem, partner+0x74, 0)
        if args.raw23_referee:
            put(mem, actor+0x36, 5 if facing else 1, 1)
            put(mem, actor+0x37, 1 if facing else 2, 1)
            for offset in (0x1150, 0x1180, 0x11b0, 0x11e0):
                put(mem, RAM+offset, 0)
        if args.raw23_contact3:
            put(mem, actor+0x36, 3, 1)
            put(mem, actor+0x37, 1 if facing else 2, 1)
        if args.raw23_release != 'none':
            put(mem, target+0x32, 0x10 if args.raw23_release == 'target' else 0, 1)
            put(mem, actor+0xfe, 0x80 if args.raw23_release == 'signal' else 0, 1)
            put(mem, actor+0x35, 1, 1)
    put(mem, actor+0x1c, latch)
    put(mem, actor+0x20, 0x8005)
    put(mem, actor+0x60, 0x8023 if selector == 0x23 and args.raw23_release != 'none'
        else selector)
    if selector == 0x61 and args.tag_recovery_kind == 'exit':
        put(mem, actor+0x60, 0x8061)
    put(mem, actor+0x26, ACTORS[(slot+1)%9], 4)
    if selector == 0x37 and args.null_link != 'none':
        put(mem, actor+0x26, 0 if args.null_link == 'zero' else 0x40000000, 4)
    put(mem, actor+0x44, counter if selector == 0x23 and args.raw23_phase == 'all'
        else int(args.raw23_phase) if selector == 0x23 and args.raw23_phase is not None
        else 6 if selector == 0x23 else counter)
    put(mem, actor+0x24, frame)
    put(mem, actor+0x22, timer)
    put(mem, actor+0x2e, facing)
    if selector == 0x7b and args.sequence_only:
        # The arcade hold counts down from three while its prior action is
        # five, then hands control to $7C on the fourth dispatch.
        put(mem, actor+0x1f, 5, 1)
        put(mem, actor+0x44, 3)
    if selector == 0x44:
        put(mem, actor+0xf2, args.raw44_index)
        if slot == 1:
            put(mem, ACTORS[(slot+1)%9]+2, 8)
    if selector in (0x11, 0x14) and frame == 3:
        target = ACTORS[(slot+1)%9]
        put(mem, actor+0x37, 0x10, 1)
        put(mem, target+6, int.from_bytes(mem[actor+6:actor+8], 'big'))
        put(mem, target+10, int.from_bytes(mem[actor+10:actor+12], 'big'))
        if slot == 0:
            put(mem, target+0x20, 4)
            put(mem, target+0x64, 8)
        elif slot == 1:
            put(mem, target+0x20, 5)
            put(mem, target+0x60, 0x61)
        else:
            put(mem, target+0x20, 0)
    if selector == 0x35 and frame == 3:
        target = ACTORS[(slot+1)%9]
        put(mem, actor+0x37, 0x10, 1)
        put(mem, target+6, int.from_bytes(mem[actor+6:actor+8], 'big'))
        put(mem, target+10, int.from_bytes(mem[actor+10:actor+12], 'big'))
        if slot == 0:
            put(mem, target+0x20, 4)
            put(mem, target+0x64, 8)
            put(mem, target+0x26, actor, 4)
        elif slot == 1:
            put(mem, target+0x20, 4)
            put(mem, target+0x64, 8)
            put(mem, target+0x26, ACTORS[3], 4)
        else:
            put(mem, target+0x20, 0)
    return mem

core = re.search(r'^CORE=(.*)$', (ROOT/'Makefile').read_text(), re.M)[1].split()
# Windows keeps a ctypes-loaded DLL open until process exit; preserve the
# differential result instead of replacing it with a temporary-file cleanup
# error. The OS can reclaim this directory after the Python process exits.
with tempfile.TemporaryDirectory(prefix='wf-reversal-', ignore_cleanup_errors=True) as directory:
    frozen = Path(directory)
    for part in ('src', 'include', 'generated'):
        shutil.copytree(ROOT/part, frozen/part)
    (frozen/'tests/r18').mkdir(parents=True)
    shutil.copy2(HERE/'action_reversal_family_oracle_bridge.c', frozen/'tests/r18')
    library = frozen/'reversal.so'
    subprocess.run(['cc', '-shared', '-fPIC', '-std=c11', '-O0' if args.fast else '-O2', '-UNDEBUG',
                    '-Wall', '-Wextra', '-Werror', '-Iinclude', *core,
                    'src/wf_r18_runtime_handoff.c',
                    'tests/r18/action_reversal_family_oracle_bridge.c',
                    '-o', str(library)], cwd=frozen, check=True)
    native = ctypes.CDLL(str(library))
    native.action_reversal_family_oracle.argtypes = [ctypes.c_void_p,
        ctypes.POINTER(ctypes.c_uint32), ctypes.c_uint,
        ctypes.POINTER(ctypes.c_uint32)]
    native.action_reversal_physics_oracle.argtypes = [ctypes.c_void_p,
        ctypes.POINTER(ctypes.c_uint32), ctypes.c_uint]
    native.action_reversal_resolver_oracle.argtypes = [ctypes.c_void_p,
        ctypes.POINTER(ctypes.c_uint32), ctypes.c_uint]
    machine = mus.Machine(mus.CPUType.M68000, 2048)
    cpu, memory = machine.cpu, machine.mem
    for page in range(0, len(rom), 0x10000):
        for width in range(3):
            memory.set_special_range_write_func(page, width, lambda address, value: None)
            memory.set_special_range_read_func(page, width,
                lambda address, n=1 << width: int.from_bytes(rom[address:address+n], 'big'))
    cases = 0
    total_steps = 0
    special_pcs = set()
    raw35_success_characters = set()
    raw35_pcs = set()
    raw35_branch_characters = {}
    sequence_phases = set()
    sequence_outcomes = []
    pin_hits = 0
    pin_hit_slots = set()
    selected = [args.selector] if args.selector is not None else FAMILY
    matrix = ((selector, slot, latch, counter, frame, timer, facing, character,
               interrupt_kind, jump_contact, jump_attack, running_contact,
               jump10_contact, slam_kind)
              for selector in selected
              for interrupt_kind in (INTERRUPT_VARIANTS if args.interrupt_kind == 'all'
                                     else (args.interrupt_kind,))
              for jump_contact in (JUMP_CONTACT_VARIANTS if args.jump_contact == 'all'
                                   else (args.jump_contact,))
              for jump_attack in (JUMP_ATTACK_VARIANTS if args.jump_attack == 'all'
                                  else (args.jump_attack,))
              for running_contact in (RUNNING_CONTACT_VARIANTS if args.running_contact == 'all'
                                      else (args.running_contact,))
              for jump10_contact in (JUMP10_VARIANTS if args.jump10_contact == 'all'
                                     else (args.jump10_contact,))
              for slam_kind in (SLAM_VARIANTS if args.slam_kind == 'all'
                                else (args.slam_kind,))
              for character in (range(12) if args.all_characters else (None,))
              for slot, latch, counter, frame, timer, facing in itertools.product(
                  (0, 1, 8), (5, 0x8005),
                  tuple(range(9)) if selector == 0x23 and args.raw23_phase == 'all'
                  else (0, 1, 2, 3, 80) if selector == 0x35
                  else (0, 1, 2, 3) if selector in (0x0e, 0x68, 0x8e) else (0, 1, 2, 80),
                  (0, 1, 2, 0xfe) if selector == 0x24
                  else (0, 1, 2, 0xfe) if selector == 0x4d
                  else (0, 1, 3, 5, 0xfe) if selector == 0x12
                  else (0, 1, 4, 0xfe) if selector == 0x48
                  else (0, 2, 3, 0xfe) if selector == 0x33
                  else (0, 1, 3, 0xfe) if selector in (0x00, 0x0d, 0x0e, 0x11, 0x14, 0x35)
                  else (0, 1, 2, 0xfe) if selector in (0x06, 0x72)
                  else (0, 4, 0xfe) if selector == 0x44 else (0, 1, 0xfe),
                  (0, 1), (0, 0x8000)))
    for selector, slot, latch, counter, frame, timer, facing, character, interrupt_kind, jump_contact, jump_attack, running_contact, jump10_contact, slam_kind in matrix:
        if interrupt_kind != 'none' and (latch, counter, frame, timer) != (0x8005, 0, 0, 0):
            continue
        if (jump_contact != 'none' and not (args.sequence_only and args.interleave_physics)
                and (latch, counter, frame, timer) != (0x8005, 0, 1, 0)):
            continue
        if (jump_attack != 'none' and not (args.sequence_only and args.interleave_physics)
                and (latch, counter, frame, timer) !=
                (0x8005, 0 if jump_attack == 'route0' else 1 if jump_attack == 'route1' else 2, 3, 0)):
            continue
        if (running_contact != 'none' and not (args.sequence_only and args.interleave_physics)
                and (latch, counter, frame, timer) != (0x8005, 3, 3, 0)):
            continue
        if (jump10_contact != 'none' and not (args.sequence_only and args.interleave_physics)
                and (latch, counter, frame, timer) != (0x8005, 0, 1, 0)):
            continue
        if args.object_release_only and not (
                latch == 0x8005 and counter == 0 and timer == 0 and
                ((slot == 1 and frame == 1) or (slot == 8 and frame == 0xfe))):
            continue
        if slam_kind != 'none' and (latch, counter, frame, timer) != (0x8005, 0, 3, 0):
            continue
        if args.pin_credited and (latch, frame, timer) != (0x8005, 4, 0):
            continue
        if args.splash_credited and (latch, counter, frame, timer) != (0x8005, 0, 0xfe, 0):
            continue
        if args.throw_credited and (latch, counter, frame, timer) != (5, 0, 0, 0):
            continue
        if args.hold_zero_health and (counter, frame) != (0, 0):
            continue
        if args.raw23_impact and (latch, counter, frame, timer) != (0x8005, 0, 0, 1):
            continue
        if args.raw23_referee and (latch, counter, frame, timer) != (0x8005, 0, 0, 1):
            continue
        if args.raw23_contact3 and (latch, counter, frame, timer) != (0x8005, 0, 0, 1):
            continue
        if args.raw23_release != 'none' and (latch, counter, frame, timer) != (0x8005, 0, 0, 1):
            continue
        sequence_counter = 2 if selector == 0x15 else 1 if selector == 0x77 else 0
        if args.sequence_only:
            if selector == 0x0e:
                if (slot, latch, frame, timer, facing) != (0, 5, 0, 0, 0) or (
                        args.interleave_physics and counter != 0):
                    continue
            elif selector == 0x8e:
                if (slot, latch, counter, frame, timer, facing) != (0, 5, args.raw8e_phase, 0, 0, 0):
                    continue
            elif selector == 0x10:
                if (latch, counter, frame, timer, facing) != (5, 0, 0, 0, 0) or (
                        args.interleave_physics and slot != 0):
                    continue
            elif (slot, latch, counter, frame, timer, facing) != (0, 5, sequence_counter, 0, 0, 0):
                continue
        mem = fixture(selector, slot, latch, counter, frame, timer, facing, character,
                      interrupt_kind, jump_contact, jump_attack, running_contact,
                      jump10_contact, slam_kind)
        regs = [rng.getrandbits(32) for _ in range(8)]
        if selector == 0x44 and args.raw44_force_fail:
            force_raw44_miss(mem, regs, cpu, memory, slot)
        memory.w_block(0, bytes(mem))
        buffer = ctypes.create_string_buffer(bytes(mem))
        dregs = (ctypes.c_uint32*8)(*regs)
        if args.trace_first and cases == 0:
            print('initial', [hex(x) for x in regs[:3]], flush=True)
        executed_steps = 0
        phase0_history = []
        phase0_handover = None
        royal_phase_history = []
        raw44_descriptors = set()
        raw44_selector_history = []
        idle_frame_history = []
        idle_descriptors = set()
        idle_entry = None
        running_phase_history = []
        for step in range(args.steps):
            if args.interleave_resolver:
                actor_addr = ACTORS[slot]
                state = memory.r16(actor_addr+0x20)
                if not state&0x8000:
                    memory.w16(actor_addr+0x1e, memory.r16(actor_addr+0x1c))
                    memory.w16(actor_addr+0x1c, state)
                    memory.w16(actor_addr+0x20, state|0x8000)
                memory.w16(actor_addr+0x36, 0)
                for i, value in enumerate(dregs):
                    cpu.w_reg(i, value)
                if buffer.raw[actor_addr+1]:
                    cpu.w_sp(0x1d0000)
                    memory.w32(0x1d0000, 0x70000)
                    cpu.w_pc(0x280dc)
                    cpu.w_reg(8, actor_addr)
                    for _ in range(5000):
                        if cpu.r_pc() == 0x70000:
                            break
                        cpu.execute(1)
                    assert cpu.r_pc() == 0x70000, (selector, step, 'ROM resolver runaway', hex(cpu.r_pc()))
                    if memory.r8(actor_addr+0x37):
                        for field, correction in ((6,0x38),(10,0x3a),(14,0x3c)):
                            memory.w16(actor_addr+field,
                                (memory.r16(actor_addr+field)+memory.r16(actor_addr+correction))&0xffff)
                status = native.action_reversal_resolver_oracle(buffer, dregs, slot)
                assert status == 0, (selector, step, 'native resolver blocked', status)
                got = buffer.raw[RAM:RAM+0x4000]
                want = bytes(memory.r_block(RAM, 0x4000))
                assert got == want, (selector, character, step, 'pre-action resolver RAM',
                    'xyz', [memory.r16(actor_addr+off) for off in (6,10,14)],
                    'flags32', hex(memory.r8(actor_addr+0x32)),
                    'arena', hex(memory.r16(RAM+0x7e)),
                    [(hex(RAM+i), x, y) for i, (x, y) in enumerate(zip(got, want)) if x != y][:12])
                assert list(dregs) == [cpu.r_reg(i) for i in range(8)], (
                    selector, step, 'pre-action resolver D0-D7', list(dregs), [cpu.r_reg(i) for i in range(8)])
            cpu.w_sp(0x1d0000)
            memory.w32(0x1d0000, 0x70000)
            cpu.w_pc(0x1c03e)
            cpu.w_reg(8, ACTORS[slot])
            for i, value in enumerate(dregs):
                cpu.w_reg(i, value)
            descriptor = None
            for _ in range(20000):
                if cpu.r_pc() == 0x70000:
                    break
                if args.tag_special and cpu.r_pc() in (0x1f760, 0x214c0):
                    special_pcs.add(cpu.r_pc())
                if args.interrupt_royal_credit and cpu.r_pc() in (0x21358, 0x21380):
                    special_pcs.add(cpu.r_pc())
                if args.trace_first and cases == 0 and step == 0 and cpu.r_pc() in (0x14b64,0x14bb0,0x14bba,0x10bd0,0x14bc0,0x14bd4,0x26ae,0x14bda,0x1c0c8):
                    print(hex(cpu.r_pc()),[hex(cpu.r_reg(i)) for i in range(3)],flush=True)
                if args.trace_first and cases == 0 and step == 0 and selector == 0x0a and 0x13744 <= cpu.r_pc() < 0x13880:
                    print('interrupt', hex(cpu.r_pc()),
                          'A', [hex(cpu.r_reg(i)) for i in range(8, 13)],
                          'D', [hex(cpu.r_reg(i)) for i in range(3)], flush=True)
                if cpu.r_pc() == 0x1c0c8:
                    descriptor = cpu.r_reg(9)
                if selector == 0x35 and cpu.r_pc() == 0x1785e and character is not None:
                    raw35_success_characters.add(character)
                if selector == 0x35 and 0x175c4 <= cpu.r_pc() <= 0x17910:
                    raw35_pcs.add(cpu.r_pc())
                    if character is not None:
                        raw35_branch_characters.setdefault(cpu.r_pc(), set()).add(character)
                cpu.execute(1)
            assert cpu.r_pc() == 0x70000, (selector, step, 'ROM runaway', hex(cpu.r_pc()))
            desc = ctypes.c_uint32()
            status = native.action_reversal_family_oracle(buffer, dregs, slot,
                                                            ctypes.byref(desc))
            tag = (hex(selector), slot, latch, counter, frame, timer, facing,
                   character, interrupt_kind, jump_contact, jump_attack,
                   running_contact, jump10_contact, slam_kind, step)
            assert status == 0, (tag, 'native blocked', hex(-status))
            assert desc.value == descriptor, (tag, 'descriptor', hex(desc.value), hex(descriptor))
            if selector == 0x00 and args.sequence_only:
                idle_descriptors.add(descriptor)
                actor_addr = ACTORS[slot]
                if step == 0:
                    idle_entry = (int.from_bytes(buffer.raw[actor_addr+6:actor_addr+8], 'big'),
                                  int.from_bytes(buffer.raw[actor_addr+0x58:actor_addr+0x5a], 'big'))
                idle_frame = int.from_bytes(buffer.raw[actor_addr+0x24:actor_addr+0x26], 'big')
                if not idle_frame_history or idle_frame_history[-1][1] != idle_frame:
                    idle_frame_history.append((step+1, idle_frame,
                        int.from_bytes(buffer.raw[actor_addr+4:actor_addr+6], 'big')))
            if selector == 0x44 and args.sequence_only:
                raw44_descriptors.add(descriptor)
            got = buffer.raw[RAM:RAM+0x4000]
            want = bytes(memory.r_block(RAM, 0x4000))
            assert got == want, (tag, 'RAM', [(hex(RAM+i), x, y) for i, (x,y)
                                         in enumerate(zip(got,want)) if x != y][:12])
            if selector == 0x0a and interrupt_kind != 'none':
                target = ACTORS[(slot+1)%9]
                if interrupt_kind.startswith('ground'):
                    result = buffer.raw[target+0x65]
                    assert result == int(interrupt_kind[-1])-2, (tag, 'ground branch', result)
                elif interrupt_kind == 'hold61':
                    result = int.from_bytes(buffer.raw[target+0x60:target+0x62], 'big')
                    assert result == 0x77, (tag, 'hold61 branch', hex(result))
                else:
                    changed = ACTORS[(slot+3)%9] if interrupt_kind == 'hold48-q49' else target
                    result = int.from_bytes(buffer.raw[changed+0x20:changed+0x22], 'big')
                    assert result & 0xff == 4, (tag, 'release branch', hex(result))
            if (selector == 0x0c and jump_contact != 'none' and
                    not (args.sequence_only and args.interleave_physics)):
                target = ACTORS[(slot+1)%9]
                if jump_contact.startswith('ground'):
                    result = int.from_bytes(buffer.raw[target+0x64:target+0x66], 'big')
                    assert result == int(jump_contact[-1])-2, (tag, 'jump ground', hex(result))
                elif jump_contact == 'counter61':
                    result = int.from_bytes(buffer.raw[target+0x60:target+0x62], 'big')
                    assert result == 0x77, (tag, 'jump counter', hex(result))
                else:
                    result = int.from_bytes(buffer.raw[ACTORS[slot]+0x20:ACTORS[slot]+0x22], 'big')
                    assert result & 0xff == 4, (tag, 'jump miss', hex(result))
            if (selector == 0x0d and jump_attack != 'none' and
                    not (args.sequence_only and args.interleave_physics)):
                target = ACTORS[(slot+1)%9]
                state = int.from_bytes(buffer.raw[target+0x20:target+0x22], 'big')
                state4 = int.from_bytes(buffer.raw[target+0x64:target+0x66], 'big')
                expected = {'route0': 0x1c, 'route1': 3, 'route2': 0x20,
                            'miss': None}[jump_attack]
                if expected is None:
                    assert state & 0xff == 0, (tag, 'jump attack miss', hex(state))
                else:
                    assert state & 0xff == 4 and state4 == expected, (
                        tag, 'jump attack hit', hex(state), hex(state4), hex(expected))
            if (selector == 0x0e and running_contact != 'none' and
                    not (args.sequence_only and args.interleave_physics)):
                if running_contact == 'hit':
                    target = ACTORS[(slot+1)%9]
                    result = int.from_bytes(buffer.raw[target+0x60:target+0x62], 'big')
                    assert result == 0x51, (tag, 'running hit', hex(result))
                else:
                    actor_addr = ACTORS[slot]
                    state = int.from_bytes(buffer.raw[actor_addr+0x20:actor_addr+0x22], 'big')
                    state4 = int.from_bytes(buffer.raw[actor_addr+0x64:actor_addr+0x66], 'big')
                    expected = 30 if running_contact == 'special54' else 5
                    assert state & 0xff == 4 and state4 == expected, (
                        tag, 'running miss', hex(state), hex(state4), expected)
            if (selector == 0x10 and jump10_contact != 'none' and
                    not (args.sequence_only and args.interleave_physics)):
                target = ACTORS[(slot+1)%9]
                if jump10_contact.startswith('ground'):
                    result = int.from_bytes(buffer.raw[target+0x64:target+0x66], 'big')
                    assert result == int(jump10_contact[-1])-2, (tag, 'jump10 ground', hex(result))
                elif jump10_contact == 'counter61':
                    result = int.from_bytes(buffer.raw[target+0x60:target+0x62], 'big')
                    assert result == 0x77, (tag, 'jump10 counter', hex(result))
                else:
                    actor_addr = ACTORS[slot]
                    state = int.from_bytes(buffer.raw[actor_addr+0x20:actor_addr+0x22], 'big')
                    state4 = int.from_bytes(buffer.raw[actor_addr+0x64:actor_addr+0x66], 'big')
                    assert state & 0xff == 4 and state4 == 5, (
                        tag, 'jump10 miss', hex(state), hex(state4))
            if selector == 0x1e and latch == 0x8005 and timer == 0 and (
                    (slot == 1 and frame == 1) or (slot == 8 and frame == 0xfe)):
                obj = ACTORS[(slot+1)%9]
                object_state = int.from_bytes(buffer.raw[obj+0x1c:obj+0x1e], 'big')
                owner_state = int.from_bytes(buffer.raw[ACTORS[slot]+0x20:ACTORS[slot]+0x22], 'big')
                assert object_state == 2 and owner_state & 0xff == 0, (
                    tag, 'object release', hex(object_state), hex(owner_state))
            if selector == 0x33 and slam_kind != 'none':
                target = ACTORS[(slot+1)%9]
                state = int.from_bytes(buffer.raw[target+0x20:target+0x22], 'big')
                state4 = int.from_bytes(buffer.raw[target+0x64:target+0x66], 'big')
                expected = 0x2005 if slam_kind == 'landing' else 0x2025
                assert state & 0xff == 4 and state4 == expected, (
                    tag, 'slam branch', hex(state), hex(state4), hex(expected))
            if selector == 0x37 and args.null_link != 'none' and latch == 5:
                actor_addr = ACTORS[slot]
                x = int.from_bytes(buffer.raw[actor_addr+6:actor_addr+8], 'big')
                expected = 0 if args.null_link == 'zero' else 0x654
                assert x == expected, (tag, 'null-link position', hex(x), hex(expected))
            if selector == 0x48 and args.pin_credited:
                held_selector = int.from_bytes(buffer.raw[ACTORS[slot]+0x60:ACTORS[slot]+0x62], 'big')
                if held_selector == 0x8048:
                    pin_hits += 1
                    pin_hit_slots.add(slot)
            if selector == 0x51 and args.splash_credited:
                owner_selector = int.from_bytes(buffer.raw[ACTORS[slot]+0x60:ACTORS[slot]+0x62], 'big')
                target_selector = int.from_bytes(buffer.raw[ACTORS[(slot+1)%9]+0x60:ACTORS[(slot+1)%9]+0x62], 'big')
                assert owner_selector == 0xc04a, (tag, 'splash owner TAG handoff', hex(owner_selector))
                if slot != 0:
                    assert target_selector == 0xc048, (tag, 'splash target TAG handoff', hex(target_selector))
            if selector == 0x57 and args.throw_credited:
                partner_flags = buffer.raw[ACTORS[(slot+2)%9]+0x34]
                assert partner_flags & 0x25 == 0x20, (
                    tag, 'TAG throw partner cleanup', hex(partner_flags))
            if selector in (0x65, 0x73, 0x74) and latch == 0x8005 and frame == 0xfe:
                next_selector = int.from_bytes(buffer.raw[ACTORS[slot]+0x60:ACTORS[slot]+0x62], 'big')
                expected = 0xc01c if selector == 0x65 else 0xc01d
                assert next_selector == expected, (tag, 'impact hold release', hex(next_selector))
            if selector in (0x81, 0x84, 0x85) and latch == 0x8005 and counter == 1:
                actor_addr = ACTORS[slot]
                target_addr = ACTORS[(slot+1)%9]
                state = int.from_bytes(buffer.raw[actor_addr+0x20:actor_addr+0x22], 'big')
                target_selector = int.from_bytes(buffer.raw[target_addr+0x60:target_addr+0x62], 'big')
                expected_state = 5 if selector == 0x81 else 4
                expected_target = 0x15 if selector == 0x81 else 0xc006
                assert state & 0xff == expected_state and target_selector == expected_target, (
                    tag, 'reversal transition', hex(state), hex(target_selector))
            if selector == 0x8b and latch == 0x8005 and frame == 0xfe:
                own = buffer.raw[ACTORS[slot]+0xa0]
                rival = buffer.raw[ACTORS[(slot+1)%9]+0xa0]
                assert (own, rival) == (0x40, 0x80), (tag, 'pin outcome', own, rival)
            if selector == 0x8c and latch == 0x8005 and timer == 0 and slot != 8:
                own = buffer.raw[ACTORS[slot]+0xa0]
                rival = buffer.raw[ACTORS[(slot+1)%9]+0xa0]
                expected = 0x40 if slot == 0 else 0x80
                assert (own, rival) == (0x80, expected), (tag, 'royal pin outcome', own, rival)
            if selector == 0x5e and args.hold_zero_health:
                recovery = int.from_bytes(buffer.raw[ACTORS[slot]+0xaa:ACTORS[slot]+0xac], 'big')
                expected = 0x100 if latch == 5 else 5
                assert recovery == expected, (tag, 'zero-health hold timer', recovery, expected)
            if selector == 0x23 and args.raw23_impact:
                target = ACTORS[(slot+1)%9]
                state = int.from_bytes(buffer.raw[target+0x20:target+0x22], 'big')
                state4 = int.from_bytes(buffer.raw[target+0x64:target+0x66], 'big')
                expected = 5 if args.raw23_pin else 4
                expected_state4 = 0 if args.raw23_pin else 7
                assert state & 0xff == expected and (args.raw23_pin or state4 == expected_state4), (
                    tag, 'aerial grapple impact', hex(state), hex(state4))
                if args.raw23_pin:
                    target_selector = int.from_bytes(buffer.raw[target+0x60:target+0x62], 'big')
                    assert target_selector == 0x64, (tag, 'TAG pin target', hex(target_selector))
            if selector == 0x23 and args.raw23_referee:
                first = (0x11b0, 0x11e0) if facing else (0x1150, 0x1180)
                flags = tuple(int.from_bytes(buffer.raw[RAM+off:RAM+off+2], 'big') for off in first)
                assert flags == (1, 1), (tag, 'referee contact handoff', flags)
            if selector == 0x23 and args.raw23_contact3:
                state4 = int.from_bytes(buffer.raw[ACTORS[slot]+0x64:ACTORS[slot]+0x66], 'big')
                substate = int.from_bytes(buffer.raw[ACTORS[slot]+0x44:ACTORS[slot]+0x46], 'big')
                assert state4 == 0x17 and substate & 0xf == 6, (
                    tag, 'contact-three fall', hex(state4), hex(substate))
            if selector == 0x23 and args.raw23_release != 'none':
                own_state = int.from_bytes(buffer.raw[ACTORS[slot]+0x20:ACTORS[slot]+0x22], 'big')
                if args.raw23_release == 'wait':
                    assert own_state & 0xff == 5, (tag, 'pin release wait', hex(own_state))
                else:
                    target = ACTORS[(slot+1)%9]
                    target_state4 = int.from_bytes(buffer.raw[target+0x64:target+0x66], 'big')
                    own_pin = buffer.raw[ACTORS[slot]+0x35]
                    assert own_state & 0xff == 7 and target_state4 == 9 and not (own_pin & 1), (
                        tag, 'pin release transition', hex(own_state), hex(target_state4), hex(own_pin))
            if selector == 0x61 and args.tag_recovery_kind == 'exit' and counter == 1:
                final_selector = int.from_bytes(buffer.raw[ACTORS[slot]+0x60:ACTORS[slot]+0x62], 'big')
                assert final_selector == 0x59, (tag, 'TAG recovery exit', hex(final_selector))
            if selector == 0x77 and latch == 0x8005 and frame == 0xfe:
                actor_addr = ACTORS[slot]
                target = ACTORS[(slot+1)%9]
                a_selector = int.from_bytes(buffer.raw[actor_addr+0x60:actor_addr+0x62], 'big')
                t_selector = int.from_bytes(buffer.raw[target+0x60:target+0x62], 'big')
                assert (a_selector, t_selector) == (0x8061, 0x8037), (
                    tag, 'TAG return transition', hex(a_selector), hex(t_selector))
            if selector == 0x8e:
                assert buffer.raw[0xc0000:0xc2000] == bytes(memory.r_block(0xc0000,0x2000)), (tag, 'VRAM clear')
                assert buffer.raw[0x182000:0x182800] == bytes(memory.r_block(0x182000,0x800)), (tag, 'palette')
                if args.sequence_only:
                    royal_phase = int.from_bytes(buffer.raw[ACTORS[slot]+0x44:ACTORS[slot]+0x46], 'big')
                    if not royal_phase_history or royal_phase_history[-1][1] != royal_phase:
                        royal_phase_history.append((step+1, royal_phase))
            if selector == 0x4c and args.credited:
                assert buffer.raw[0x180000:0x184000] == bytes(memory.r_block(0x180000,0x4000)), (tag, 'TAG palette')
            if selector == 0x66 and args.tag66_credited:
                assert buffer.raw[0x180000:0x184000] == bytes(memory.r_block(0x180000,0x4000)), (tag, 'TAG exchange palette')
            assert list(dregs) == [cpu.r_reg(i) for i in range(8)], (tag, 'D0-D7',
                [hex(x) for x in dregs], [hex(cpu.r_reg(i)) for i in range(8)])
            if args.interleave_physics:
                cpu.w_sp(0x1d0000)
                memory.w32(0x1d0000, 0x70000)
                cpu.w_pc(0x2208)
                cpu.w_reg(8, ACTORS[slot])
                for i, value in enumerate(dregs):
                    cpu.w_reg(i, value)
                for _ in range(5000):
                    if cpu.r_pc() == 0x70000:
                        break
                    cpu.execute(1)
                assert cpu.r_pc() == 0x70000, (tag, 'ROM physics runaway', hex(cpu.r_pc()))
                status = native.action_reversal_physics_oracle(buffer, dregs, slot)
                assert status == 0, (tag, 'native physics blocked', status)
                got = buffer.raw[RAM:RAM+0x4000]
                want = bytes(memory.r_block(RAM, 0x4000))
                assert got == want, (tag, 'post-physics RAM', [(hex(RAM+i), x, y)
                    for i, (x, y) in enumerate(zip(got, want)) if x != y][:12])
                assert list(dregs) == [cpu.r_reg(i) for i in range(8)], (tag, 'post-physics D0-D7')
            if args.sequence_only and selector in (0x0e, 0x11, 0x14, 0x35):
                at = ACTORS[slot]
                sequence_phases.add(tuple(int.from_bytes(buffer.raw[at+off:at+off+2], 'big')
                                          for off in (0x20, 0x60, 0x44, 0x24, 0x22)))
                if selector == 0x0e:
                    phase = int.from_bytes(buffer.raw[at+0x44:at+0x46], 'big') & 3
                    if not running_phase_history or running_phase_history[-1][1] != phase:
                        running_phase_history.append((step+1, phase))
                if args.trace_first and cases == 0 and selector == 0x35 and (
                        buffer.raw[at+0x37]&0x10 or buffer.raw[at+0x21] != 5 or step%20 == 0):
                    target_at = ACTORS[(slot+1)%9]
                    read_word = lambda at,off: int.from_bytes(buffer.raw[at+off:at+off+2], 'big')
                    print('raw35-sequence-event', step,
                          'state', hex(read_word(at,0x20)), 'phase', read_word(at,0x44),
                          'frame', read_word(at,0x24), 'contact', hex(read_word(at,0x36)),
                          'motion', buffer.raw[at+1], 'angle', hex(buffer.raw[at+0x2d]),
                          'facing', hex(buffer.raw[at+0x2e]), 'destination', read_word(at,0xbe),
                          'owner_xyz', [read_word(at,off) for off in (6,10,14)],
                          'target_xyz', [read_word(target_at,off) for off in (6,10,14)],
                          flush=True)
            executed_steps += 1
            if selector == 0x00 and args.sequence_only:
                state = int.from_bytes(buffer.raw[ACTORS[slot]+0x20:ACTORS[slot]+0x22], 'big')
                if state & 0xff != 5:
                    break
            if selector == 0x23 and args.sequence_only and args.raw23_phase == '0' and phase0_handover is None:
                at = ACTORS[slot]
                substate = int.from_bytes(buffer.raw[at+0x44:at+0x46], 'big') & 0xf
                if substate:
                    phase0_handover = (step+1, substate)
            if args.trace_first and selector == 0x23 and args.sequence_only and character == 0:
                at = ACTORS[slot]
                sample = (step+1,
                    int.from_bytes(buffer.raw[at+0x20:at+0x22], 'big'),
                    int.from_bytes(buffer.raw[at+0x44:at+0x46], 'big'),
                    int.from_bytes(buffer.raw[at+0x24:at+0x26], 'big'))
                if not phase0_history or sample[1:] != phase0_history[-1][1:]:
                    phase0_history.append(sample)
            if selector in (0x7e, 0x7f) and args.sequence_only:
                current = int.from_bytes(buffer.raw[ACTORS[slot]+0x60:ACTORS[slot]+0x62], 'big')
                if current & 0xff != selector:
                    break
            if selector in (0x55, 0x56, 0x5a, 0x5b, 0x78) and args.sequence_only:
                state = int.from_bytes(buffer.raw[ACTORS[slot]+0x20:ACTORS[slot]+0x22], 'big')
                if state & 0xff != 5:
                    break
            if selector in (0x11, 0x12, 0x13, 0x14, 0x35) and args.sequence_only:
                state = int.from_bytes(buffer.raw[ACTORS[slot]+0x20:ACTORS[slot]+0x22], 'big')
                if selector == 0x13 and args.raw13_downed:
                    victim = ACTORS[(slot+1)%9]
                    victim_state4 = int.from_bytes(buffer.raw[victim+0x64:victim+0x66], 'big')
                    if victim_state4 == 7 and not args.follow_impact:
                        break
                if selector == 0x12 and args.raw12_downed:
                    victim = ACTORS[(slot+1)%9]
                    victim_state4 = int.from_bytes(buffer.raw[victim+0x64:victim+0x66], 'big')
                    if victim_state4 == 6 and not args.follow_impact:
                        break
                if selector == 0x12 and args.raw12_active:
                    victim = ACTORS[(slot+1)%9]
                    victim_state4 = int.from_bytes(buffer.raw[victim+0x64:victim+0x66], 'big')
                    if victim_state4 == 0x77 and not args.follow_impact:
                        break
                if state & 0xff != 5:
                    break
            if selector in (0x0c, 0x0d, 0x0e, 0x10) and args.sequence_only and args.interleave_physics:
                actor_addr = ACTORS[slot]
                state = int.from_bytes(buffer.raw[actor_addr+0x20:actor_addr+0x22], 'big')
                current_selector = int.from_bytes(buffer.raw[actor_addr+0x60:actor_addr+0x62], 'big')
                if state & 0xff != 5 or current_selector & 0xff != selector:
                    break
            if selector == 0x44 and args.sequence_only:
                current = int.from_bytes(buffer.raw[ACTORS[slot]+0x60:ACTORS[slot]+0x62], 'big')
                state = int.from_bytes(buffer.raw[ACTORS[slot]+0x20:ACTORS[slot]+0x22], 'big')
                if not raw44_selector_history or raw44_selector_history[-1][1] != current & 0xff:
                    raw44_selector_history.append((step+1, current & 0xff))
                if current & 0xff not in (selector, 0x82) or state & 0xff != 5:
                    break
            if selector == 0x4e and args.sequence_only:
                state = int.from_bytes(buffer.raw[ACTORS[slot]+0x20:ACTORS[slot]+0x22], 'big')
                if state & 0xff != 5:
                    break
        if args.sequence_only and selector in (0x7b, 0x7c) and args.steps >= (4 if selector == 0x7b else 11):
            final_selector = int.from_bytes(buffer.raw[ACTORS[slot]+0x60:ACTORS[slot]+0x62], 'big')
            expected = 0x7c if selector == 0x7b else 0x17
            assert final_selector == expected, (hex(selector), character, 'handover',
                                                hex(final_selector), hex(expected))
        if args.sequence_only and selector == 0x8e:
            substate = int.from_bytes(buffer.raw[ACTORS[slot]+0x44:ACTORS[slot]+0x46], 'big')
            if args.raw8e_phase in (0, 1) and args.steps == 70:
                assert substate == args.raw8e_phase + 1, ('Royal animation phase', substate)
            if args.raw8e_phase == 2 and args.steps == 1:
                assert substate == 3, ('Royal wait-for-idle handover', substate)
            if args.raw8e_phase == 3 and args.steps == 512:
                next_selector = int.from_bytes(buffer.raw[ACTORS[(slot+1)%9]+0x60:ACTORS[(slot+1)%9]+0x62], 'big')
                end_flag = int.from_bytes(buffer.raw[RAM+0x16c4:RAM+0x16c6], 'big')
                assert next_selector == 0x79 and end_flag & 0x100, (
                    'Royal interlude terminal handover', hex(next_selector), hex(end_flag))
        if args.sequence_only and selector == 0x4e and args.steps >= 40:
            final_state = int.from_bytes(buffer.raw[ACTORS[slot]+0x20:ACTORS[slot]+0x22], 'big')
            initial_x = int.from_bytes(mem[ACTORS[slot]+6:ACTORS[slot]+8], 'big')
            side = bool(mem[ACTORS[slot]+0x33]&0x80)
            expected_x = (initial_x + (-56 if side else 56)) & 0xffff
            final_x = int.from_bytes(buffer.raw[ACTORS[slot]+6:ACTORS[slot]+8], 'big')
            tier = int.from_bytes(buffer.raw[ACTORS[slot]+0x70:ACTORS[slot]+0x72], 'big')
            expected_tier = 2 - ((character if character is not None else 0)%3)
            assert (final_state, final_x, tier) == (0, expected_x, expected_tier), (
                'apron return completion', hex(final_state), hex(final_x), tier,
                hex(expected_x), expected_tier)
        if args.arrived and selector == 0x6b and latch == 0x8005:
            state = int.from_bytes(buffer.raw[ACTORS[slot]+0x20:ACTORS[slot]+0x22], 'big')
            assert state == 0, (hex(selector), character, 'arrival', hex(state))
        if args.sequence_only and selector == 0x24 and args.steps >= 63:
            state = int.from_bytes(buffer.raw[ACTORS[slot]+0x20:ACTORS[slot]+0x22], 'big')
            assert state == 0, (hex(selector), character, 'throw complete', hex(state))
        if (args.sequence_only and selector == 0x23 and args.raw23_phase == '0'
                and args.steps >= (74 if raw23_mode & 2 else 75)):
            if phase0_history:
                print('phase0-history', phase0_history, flush=True)
            expected = 8 if raw23_mode & 2 else 1
            assert phase0_handover is not None and phase0_handover[1] == expected and phase0_handover[0] <= (74 if raw23_mode & 2 else 75), (
                hex(selector), character, 'phase0 first handover', phase0_handover, expected)
            if raw23_mode & 2:
                state = int.from_bytes(buffer.raw[ACTORS[slot]+0x20:ACTORS[slot]+0x22], 'big')
                assert state & 0xff in (4, 7), (
                    hex(selector), character, 'scene terminal state', hex(state))
        if args.sequence_only and selector == 0x17 and args.steps >= 39:
            state = int.from_bytes(buffer.raw[ACTORS[slot]+0x20:ACTORS[slot]+0x22], 'big')
            assert state == 0, (hex(selector), character, 'release complete', hex(state))
        if args.sequence_only and selector in (0x7d, 0x80, 0x81, 0x82) and args.steps >= 80:
            own_selector = int.from_bytes(buffer.raw[ACTORS[slot]+0x60:ACTORS[slot]+0x62], 'big')
            target_selector = int.from_bytes(buffer.raw[ACTORS[(slot+1)%9]+0x60:ACTORS[(slot+1)%9]+0x62], 'big')
            assert (own_selector, target_selector) == (0xc07b, 0x15), (
                hex(selector), character, 'reversal handover', hex(own_selector), hex(target_selector))
        if args.sequence_only and selector in (0x84, 0x85) and args.steps >= 80:
            own_state = int.from_bytes(buffer.raw[ACTORS[slot]+0x20:ACTORS[slot]+0x22], 'big')
            own_state4 = int.from_bytes(buffer.raw[ACTORS[slot]+0x64:ACTORS[slot]+0x66], 'big')
            target_selector = int.from_bytes(buffer.raw[ACTORS[(slot+1)%9]+0x60:ACTORS[(slot+1)%9]+0x62], 'big')
            assert (own_state & 0xff, own_state4, target_selector) == (4, 14, 0xc006), (
                hex(selector), character, 'reversal landing', hex(own_state),
                hex(own_state4), hex(target_selector))
        if args.sequence_only and selector in (0x86, 0x87) and args.steps >= 72:
            own_state = int.from_bytes(buffer.raw[ACTORS[slot]+0x20:ACTORS[slot]+0x22], 'big')
            own_state4 = int.from_bytes(buffer.raw[ACTORS[slot]+0x64:ACTORS[slot]+0x66], 'big')
            target_selector = int.from_bytes(buffer.raw[ACTORS[(slot+1)%9]+0x60:ACTORS[(slot+1)%9]+0x62], 'big')
            assert (own_state & 0xff, own_state4, target_selector) == (4, 14, 0xc006), (
                hex(selector), character, '72-frame release', hex(own_state),
                hex(own_state4), hex(target_selector))
        if args.sequence_only and selector in (0x55, 0x56, 0x5a, 0x5b):
            own_state = int.from_bytes(buffer.raw[ACTORS[slot]+0x20:ACTORS[slot]+0x22], 'big')
            target_state = int.from_bytes(buffer.raw[ACTORS[(slot+1)%9]+0x20:ACTORS[(slot+1)%9]+0x22], 'big')
            expected = 0 if selector in (0x55, 0x5b) else 7
            assert (own_state & 0xff, target_state & 0xff) == (expected, 4), (
                hex(selector), character, 'throw release', executed_steps,
                hex(own_state), hex(target_state))
        if args.sequence_only and selector == 0x89 and args.steps >= 100:
            own_selector = int.from_bytes(buffer.raw[ACTORS[slot]+0x60:ACTORS[slot]+0x62], 'big')
            assert own_selector & 0x8000, (
                hex(selector), character, 'one-time partner push', hex(own_selector))
        if args.sequence_only and selector in (0x7e, 0x7f):
            own_selector = int.from_bytes(buffer.raw[ACTORS[slot]+0x60:ACTORS[slot]+0x62], 'big')
            target_selector = int.from_bytes(buffer.raw[ACTORS[(slot+1)%9]+0x60:ACTORS[(slot+1)%9]+0x62], 'big')
            assert (own_selector, target_selector) == (0xc07b, 0x15), (
                hex(selector), character, 'linked hold release', executed_steps,
                hex(own_selector), hex(target_selector))
        if args.sequence_only and selector == 0x78:
            own_state = int.from_bytes(buffer.raw[ACTORS[slot]+0x20:ACTORS[slot]+0x22], 'big')
            assert own_state & 0xff == 7, (hex(selector), character,
                'release from final pose', executed_steps, hex(own_state))
        victim_at_impact = (int.from_bytes(buffer.raw[ACTORS[(slot+1)%9]+0x64:
                              ACTORS[(slot+1)%9]+0x66], 'big')
                            if args.victim_follow else None)
        victim_selector_at_impact = (int.from_bytes(buffer.raw[ACTORS[(slot+1)%9]+0x60:
                                       ACTORS[(slot+1)%9]+0x62], 'big')
                                     if args.victim_follow else None)
        victim_steps = 0
        if args.victim_follow:
            target_slot = (slot+1)%9
            target_addr = ACTORS[target_slot]
            for victim_step in range(args.victim_follow):
                victim_before = tuple(memory.r16(target_addr+off) for off in (0x1c, 0x20, 0x22, 0x24, 0x64))
                state = memory.r16(target_addr+0x20)
                if not state&0x8000:
                    memory.w16(target_addr+0x1e, memory.r16(target_addr+0x1c))
                    memory.w16(target_addr+0x1c, state)
                    memory.w16(target_addr+0x20, state|0x8000)
                memory.w16(target_addr+0x36, 0)
                for i, value in enumerate(dregs):
                    cpu.w_reg(i, value)
                if buffer.raw[target_addr+1]:
                    cpu.w_sp(0x1d0000)
                    memory.w32(0x1d0000, 0x70000)
                    cpu.w_pc(0x280dc)
                    cpu.w_reg(8, target_addr)
                    for _ in range(5000):
                        if cpu.r_pc() == 0x70000:
                            break
                        cpu.execute(1)
                    assert cpu.r_pc() == 0x70000, (selector, character, victim_step,
                                                   'victim resolver ROM runaway', hex(cpu.r_pc()))
                    if memory.r8(target_addr+0x37):
                        for field, correction in ((6,0x38),(10,0x3a),(14,0x3c)):
                            memory.w16(target_addr+field,
                                (memory.r16(target_addr+field)+memory.r16(target_addr+correction))&0xffff)
                status = native.action_reversal_resolver_oracle(buffer, dregs, target_slot)
                assert status == 0, (selector, character, victim_step,
                                     'victim resolver native blocked', status)
                assert buffer.raw[RAM:RAM+0x4000] == bytes(memory.r_block(RAM, 0x4000)), (
                    selector, character, victim_step, 'victim pre-action resolver RAM')
                assert list(dregs) == [cpu.r_reg(i) for i in range(8)], (
                    selector, character, victim_step, 'victim pre-action resolver D0-D7')
                victim_pcs = deque(maxlen=20)
                cpu.w_sp(0x1d0000)
                memory.w32(0x1d0000, 0x70000)
                cpu.w_pc(0x1c03e)
                cpu.w_reg(8, target_addr)
                for i, value in enumerate(dregs):
                    cpu.w_reg(i, value)
                for _ in range(20000):
                    if cpu.r_pc() == 0x70000:
                        break
                    victim_pcs.append(cpu.r_pc())
                    cpu.execute(1)
                assert cpu.r_pc() == 0x70000, (selector, character, victim_step,
                                               'victim ROM runaway', hex(cpu.r_pc()),
                                               victim_before, [hex(pc) for pc in victim_pcs],
                                               [hex(cpu.r_reg(i)) for i in range(8, 13)])
                victim_descriptor = ctypes.c_uint32()
                status = native.action_reversal_family_oracle(
                    buffer, dregs, target_slot, ctypes.byref(victim_descriptor))
                assert status == 0, (selector, character, victim_step,
                                     'victim native blocked', hex(-status))
                got = buffer.raw[RAM:RAM+0x4000]
                want = bytes(memory.r_block(RAM, 0x4000))
                assert got == want, (selector, character, victim_step, 'victim RAM',
                                     [(hex(RAM+i), x, y) for i, (x, y) in
                                      enumerate(zip(got, want)) if x != y][:12])
                assert list(dregs) == [cpu.r_reg(i) for i in range(8)], (
                    selector, character, victim_step, 'victim D0-D7')
                cpu.w_sp(0x1d0000)
                memory.w32(0x1d0000, 0x70000)
                cpu.w_pc(0x2208)
                cpu.w_reg(8, target_addr)
                for i, value in enumerate(dregs):
                    cpu.w_reg(i, value)
                for _ in range(5000):
                    if cpu.r_pc() == 0x70000:
                        break
                    cpu.execute(1)
                assert cpu.r_pc() == 0x70000, (selector, character, victim_step,
                                               'victim physics ROM runaway', hex(cpu.r_pc()))
                status = native.action_reversal_physics_oracle(buffer, dregs, target_slot)
                assert status == 0, (selector, character, victim_step,
                                     'victim physics native blocked', status)
                assert buffer.raw[RAM:RAM+0x4000] == bytes(memory.r_block(RAM, 0x4000)), (
                    selector, character, victim_step, 'victim post-physics RAM')
                assert list(dregs) == [cpu.r_reg(i) for i in range(8)], (
                    selector, character, victim_step, 'victim post-physics D0-D7')
                victim_steps += 1
                if (memory.r16(target_addr+0x20)&0xff == 0 or
                        (selector in (0x0c, 0x10) and
                         (args.jump_contact == 'counter61' or args.jump10_contact == 'counter61') and
                         memory.r16(target_addr+0x60)&0xff == 0x61 and
                         memory.r16(ACTORS[slot]+0x60)&0xff == 0x37)):
                    break
        total_steps += executed_steps
        if args.sequence_only and selector == 0x00:
            at = ACTORS[slot]
            state = int.from_bytes(buffer.raw[at+0x20:at+0x22], 'big')
            expected = 0x1286c if args.idle_descriptor == 'alternate' else 0x12850
            outcome = {'character': character, 'steps': executed_steps,
                       'state': state, 'descriptors': [hex(value) for value in sorted(idle_descriptors)],
                       'frame_history': idle_frame_history, 'entry_x_vx': idle_entry}
            sequence_outcomes.append(outcome)
            assert state == 0 and executed_steps < args.steps, ('idle terminal', outcome)
            assert idle_descriptors == {expected}, ('idle descriptor choice', outcome)
            assert [frame for _, frame, _ in idle_frame_history] == [0, 1, 2, 3, 4, 0xfe], (
                'idle animation frames', outcome)
            expected_entry = {
                'center': (0x280, 0x1234), 'left': (0x1f6, 0),
                'right': (0x328, 0), 'left-high': (0x180, 0),
                'right-high': (0x400, 0), 'apron-left': (0x171, 0),
                'apron-right': (0x3a8, 0), 'outer-left': (0x288, 0),
                'outer-right': (0x3e8, 0)}[args.idle_edge or 'center']
            assert idle_entry == expected_entry, ('idle boundary response', outcome, expected_entry)
        if args.sequence_only and selector in (0x11, 0x12, 0x13, 0x14, 0x35):
            at = ACTORS[slot]
            word = lambda off: int.from_bytes(buffer.raw[at+off:at+off+2], 'big')
            sequence_outcomes.append({'character': character, 'steps': executed_steps,
                'state': word(0x20), 'selector': word(0x60), 'phase': word(0x44),
                'x': word(6), 'y': word(10), 'z': word(14),
                'frame': word(0x24), 'timer': word(0x22),
                'victim_steps': victim_steps if args.victim_follow else None,
                'victim_at_impact': victim_at_impact,
                'victim_selector_at_impact': victim_selector_at_impact,
                'victim_state_after_follow': int.from_bytes(buffer.raw[ACTORS[(slot+1)%9]+0x20:ACTORS[(slot+1)%9]+0x22], 'big')
                    if args.victim_follow else None,
                'victim_subtype_after_follow': int.from_bytes(buffer.raw[ACTORS[(slot+1)%9]+0x64:ACTORS[(slot+1)%9]+0x66], 'big')
                    if args.victim_follow else None,
                'target_state': int.from_bytes(buffer.raw[ACTORS[(slot+1)%9]+0x20:ACTORS[(slot+1)%9]+0x22], 'big')
                    if selector in (0x11, 0x14, 0x35) else None,
                'target_selector': int.from_bytes(buffer.raw[ACTORS[(slot+1)%9]+0x60:ACTORS[(slot+1)%9]+0x62], 'big')
                    if selector in (0x11, 0x14, 0x35) else None,
                'target_state4': int.from_bytes(buffer.raw[ACTORS[(slot+1)%9]+0x64:ACTORS[(slot+1)%9]+0x66], 'big')
                    if selector in (0x11, 0x12, 0x13, 0x14, 0x35) else None})
        if args.sequence_only and selector in (0x0c, 0x0d, 0x0e, 0x10) and args.interleave_physics:
            at = ACTORS[slot]
            target = ACTORS[(slot+1)%9]
            word = lambda address: int.from_bytes(buffer.raw[address:address+2], 'big')
            sequence_outcomes.append({'character': character, 'steps': executed_steps,
                'state': word(at+0x20), 'selector': word(at+0x60),
                'phase': word(at+0x44), 'state4': word(at+0x64),
                'x': word(at+6), 'y': word(at+10),
                'phase_history': running_phase_history,
                'victim_steps': victim_steps if args.victim_follow else None,
                'victim_at_impact': victim_at_impact,
                'victim_selector_at_impact': victim_selector_at_impact,
                'victim_state_after_follow': word(target+0x20) if args.victim_follow else None,
                'target_state': word(target+0x20), 'target_selector': word(target+0x60),
                'target_state4': word(target+0x64)})
        if args.sequence_only and selector == 0x8e:
            target = ACTORS[(slot+1)%9]
            outcome = {'character': character, 'steps': executed_steps,
                'phase_history': royal_phase_history,
                'next_selector': int.from_bytes(buffer.raw[target+0x60:target+0x62], 'big'),
                'end_flag': int.from_bytes(buffer.raw[RAM+0x16c4:RAM+0x16c6], 'big')}
            sequence_outcomes.append(outcome)
            if args.interleave_resolver and args.steps == 653 and args.raw8e_phase == 0:
                assert [phase for _, phase in royal_phase_history] == [0, 1, 2, 3], (
                    character, 'Royal phase handoffs', royal_phase_history)
                assert outcome['next_selector'] == 0x79 and outcome['end_flag'] & 0x100, (
                    character, 'Royal terminal handoff', outcome)
        if args.sequence_only and selector == 0x44:
            at = ACTORS[slot]
            target = ACTORS[(slot+1)%9]
            word = lambda address: int.from_bytes(buffer.raw[address:address+2], 'big')
            sequence_outcomes.append({'character': character, 'steps': executed_steps,
                'state': word(at+0x20), 'selector': word(at+0x60),
                'frame': word(at+0x24), 'timer': word(at+0x22),
                'target_state': word(target+0x20), 'target_state4': word(target+0x64),
                'descriptors': [hex(value) for value in sorted(raw44_descriptors)],
                'selector_history': raw44_selector_history})
        cases += 1
        if args.limit is not None and cases >= args.limit:
            break
    if args.tag_special:
        assert {0x1f760, 0x214c0} <= special_pcs, ('TAG rescue ROM path', sorted(special_pcs))
    if args.interrupt_royal_credit:
        assert 0x21358 in special_pcs, 'credited Royal cleanup ROM path not reached'
        if args.limit is None:
            assert 0x21380 in special_pcs, 'selector 0x53 Royal pair cleanup ROM path not reached'
    if args.pin_credited:
        assert pin_hits, 'credited TAG pin success path was not reached'
        if args.limit is None:
            assert {0, 8} <= pin_hit_slots, ('credited TAG pin slots', sorted(pin_hit_slots))
    if args.selector == 0x35 and args.all_characters and args.limit is None and not args.sequence_only:
        assert raw35_success_characters == set(range(12)), (
            'aerial success and selector bit-four clear', sorted(raw35_success_characters))
        required = ({0x175ce, 0x177a0, 0x1785e, 0x178be, 0x178cc}
                    if args.raw35_mode & 2 else
                    {0x17604, 0x1769c, 0x17776, 0x1785e, 0x178be, 0x178cc})
        assert required <= raw35_pcs, (
            'aerial ROM branches not exercised',
            [hex(pc) for pc in sorted(required - raw35_pcs)])
        for pc in required:
            assert raw35_branch_characters.get(pc, set()) == set(range(12)), (
                'aerial ROM branch missing characters', hex(pc),
                sorted(raw35_branch_characters.get(pc, set())))
    if (args.selector == 0x35 and args.all_characters and args.limit is None
            and args.sequence_only and args.interleave_resolver):
        if args.raw35_active:
            assert not raw35_success_characters, (
                'interleaved active-target false impacts', sorted(raw35_success_characters))
            assert all(outcome['state']&0xff == 4 and outcome['target_state']&0xff == 5
                       and outcome['target_selector'] == 0x61 for outcome in sequence_outcomes), (
                'interleaved active-target miss and landing', sequence_outcomes)
        elif args.raw35_target_x == 0x310 and args.raw35_mode & 2:
            assert not raw35_success_characters, (
                'interleaved rope-side false impacts', sorted(raw35_success_characters))
            assert all(outcome['state']&0xff == 4 and outcome['target_state4'] == 8
                       for outcome in sequence_outcomes), (
                'interleaved rope-side miss and landing', sequence_outcomes)
        elif args.raw35_target_x in (0x2c0, 0x310):
            assert raw35_success_characters == set(range(12)), (
                'interleaved aerial impact missing characters', sorted(raw35_success_characters))
            assert all(outcome['state']&0xff == 7 and
                       (outcome['victim_at_impact'] == 26
                        if args.victim_follow else outcome['target_state4'] == 26)
                       for outcome in sequence_outcomes), (
                'interleaved aerial terminal state', sequence_outcomes)
    if (args.selector in (0x11, 0x14) and args.all_characters and args.limit is None
            and args.sequence_only and args.interleave_resolver):
        if args.linked_downed:
            expected_victim = 6 if args.selector == 0x11 else 7
            assert all(outcome['state']&0xff == 7 and
                       (outcome['victim_at_impact'] == expected_victim
                        if args.victim_follow else outcome['target_state4'] == expected_victim)
                       and outcome['frame']&0xff == 0xfe for outcome in sequence_outcomes), (
                'interleaved linked impact and recovery', sequence_outcomes)
            if args.victim_follow:
                assert all(outcome['victim_state_after_follow']&0xff == 0 and
                           outcome['victim_steps'] < args.victim_follow
                           for outcome in sequence_outcomes), (
                    'interleaved victim knockdown, rise and idle handoff', sequence_outcomes)
        elif args.linked_active:
            expected_selector = 0x77 if args.selector == 0x11 else 0x61
            assert all(outcome['state']&0xff == 4 and outcome['target_state']&0xff == 5
                       and outcome['target_selector'] == expected_selector for outcome in sequence_outcomes), (
                'interleaved active-opponent result and landing', sequence_outcomes)
        else:
            assert {outcome['state']&0xff for outcome in sequence_outcomes} == {4}, (
                'interleaved running terminal state', sequence_outcomes)
    if (args.selector == 0x13 and args.all_characters and args.limit is None
            and args.sequence_only and args.interleave_resolver):
        if args.follow_impact:
            assert all(outcome['state']&0xff == 7 and
                       (outcome['victim_at_impact'] == 7
                        if args.victim_follow else outcome['target_state4'] == 7)
                       and outcome['frame']&0xff == 0xfe for outcome in sequence_outcomes), (
                'aerial impact recovery', sequence_outcomes)
        elif args.raw13_downed:
            assert all(outcome['state']&0xff == 5 and outcome['target_state4'] == 7
                       for outcome in sequence_outcomes), ('aerial downed-opponent impact', sequence_outcomes)
        else:
            assert all(outcome['state']&0xff == 4
                       for outcome in sequence_outcomes), ('aerial fall without downed target', sequence_outcomes)
    if (args.selector == 0x12 and args.all_characters and args.limit is None
            and args.sequence_only and args.interleave_resolver):
        if args.follow_impact:
            expected = 6 if args.raw12_downed else 0x77
            assert all(outcome['state']&0xff == 7 and
                       (outcome['victim_at_impact'] == expected
                        if args.victim_follow else outcome['target_state4'] == expected)
                       and outcome['frame']&0xff == 0xfe for outcome in sequence_outcomes), (
                'multiphase leap impact recovery', sequence_outcomes)
        elif args.raw12_downed:
            assert all(outcome['state']&0xff == 5 and outcome['target_state4'] == 6
                       for outcome in sequence_outcomes), ('multiphase leap impact', sequence_outcomes)
        elif args.raw12_active:
            assert all(outcome['state']&0xff == 5 and outcome['target_state4'] == 0x77
                       for outcome in sequence_outcomes), ('multiphase leap active contact', sequence_outcomes)
        else:
            assert all(outcome['state']&0xff == 4
                       for outcome in sequence_outcomes), ('multiphase leap landing', sequence_outcomes)
    if args.victim_follow and args.all_characters and args.limit is None:
        if args.selector in (0x0c, 0x10) and (
                args.jump_contact == 'counter61' or args.jump10_contact == 'counter61'):
            assert all(outcome['victim_selector_at_impact']&0xff == 0x77 and
                       outcome['victim_state_after_follow']&0xff == 5 and
                       outcome['target_selector']&0xff == 0x61 and
                       outcome['selector']&0xff == 0x37 and
                       outcome['victim_steps'] < args.victim_follow
                       for outcome in sequence_outcomes), (
                'counter contact return handoff', sequence_outcomes)
        else:
            assert all(outcome['victim_state_after_follow']&0xff == 0 and
                       outcome['victim_steps'] < args.victim_follow
                       for outcome in sequence_outcomes), (
                'victim complete recovery to idle', sequence_outcomes)
    if (args.selector == 0x0e and args.all_characters and args.limit is None and
            args.sequence_only and args.interleave_resolver and args.running_contact != 'none'):
        assert all([phase for _, phase in outcome['phase_history']] == [0, 1, 2, 3] and
                   outcome['steps'] < args.steps for outcome in sequence_outcomes), (
            'running attack all four phases', sequence_outcomes)
        if args.running_contact == 'hit':
            assert all(outcome['state']&0xff == 7 and
                       (outcome['victim_selector_at_impact'] == 0x51 if args.victim_follow
                        else outcome['target_selector'] == 0x51)
                       for outcome in sequence_outcomes), (
                'running attack hit handoff', sequence_outcomes)
        else:
            expected = 30 if args.running_contact == 'special54' else 5
            assert all(outcome['state']&0xff == 4 and outcome['state4'] == expected and
                       (outcome['target_selector'] == 0x54 if args.running_contact == 'special54'
                        else outcome['target_state']&0xff == 4)
                       for outcome in sequence_outcomes), (
                'running attack missed contact', sequence_outcomes)
    if (args.selector == 0x0c and args.all_characters and args.limit is None and
            args.sequence_only and args.interleave_resolver and args.jump_contact != 'none'):
        assert all(outcome['steps'] < args.steps for outcome in sequence_outcomes), (
            'jump action did not terminate', sequence_outcomes)
        if args.jump_contact.startswith('ground'):
            expected = int(args.jump_contact[-1]) - 2
            assert all(outcome['state']&0xff == 7 and
                       (outcome['victim_at_impact'] == expected if args.victim_follow
                        else outcome['target_state']&0xff == 4 and outcome['target_state4'] == expected)
                       for outcome in sequence_outcomes), ('jump grounded hit', sequence_outcomes)
        elif args.jump_contact == 'counter61':
            assert all((outcome['victim_selector_at_impact']&0xff == 0x77
                        if args.victim_follow else
                        outcome['state']&0xff == 7 and outcome['target_state']&0xff == 5 and
                        outcome['target_selector']&0xff == 0x77)
                       for outcome in sequence_outcomes), ('jump counter contact', sequence_outcomes)
        else:
            assert all(outcome['state']&0xff == 4 and outcome['state4'] == 5 and
                       outcome['target_selector'] == 0
                       for outcome in sequence_outcomes), ('jump missed contact', sequence_outcomes)
    if (args.selector == 0x10 and args.all_characters and args.limit is None and
            args.sequence_only and args.interleave_resolver and args.jump10_contact != 'none'):
        assert all(outcome['steps'] < args.steps for outcome in sequence_outcomes), (
            'second jump action did not terminate', sequence_outcomes)
        if args.jump10_contact.startswith('ground'):
            expected = int(args.jump10_contact[-1]) - 2
            assert all(outcome['state']&0xff == 7 and
                       (outcome['victim_at_impact'] == expected if args.victim_follow
                        else outcome['target_state']&0xff == 4 and outcome['target_state4'] == expected)
                       for outcome in sequence_outcomes), ('second jump grounded hit', sequence_outcomes)
        elif args.jump10_contact == 'counter61':
            assert all((outcome['victim_selector_at_impact']&0xff == 0x77
                        if args.victim_follow else
                        outcome['state']&0xff == 7 and outcome['target_state']&0xff == 5 and
                        outcome['target_selector']&0xff == 0x77)
                       for outcome in sequence_outcomes), ('second jump counter contact', sequence_outcomes)
        else:
            assert all(outcome['state']&0xff == 4 and outcome['state4'] == 5 and
                       outcome['target_selector'] == 0
                       for outcome in sequence_outcomes), ('second jump missed contact', sequence_outcomes)
    if (args.selector == 0x0d and args.all_characters and args.limit is None and
            args.sequence_only and args.interleave_resolver and args.jump_attack != 'none'):
        expected = {'route0': 28, 'route1': 3, 'route2': 32, 'miss': 8}[args.jump_attack]
        assert all(outcome['state']&0xff == 0 and
                   (outcome['victim_at_impact'] == expected if args.victim_follow
                    else outcome['target_state']&0xff == 4 and outcome['target_state4'] == expected) and
                   outcome['steps'] < args.steps for outcome in sequence_outcomes), (
            'aerial attack route and idle handoff', sequence_outcomes)
    if (args.selector == 0x44 and args.all_characters and args.limit is None
            and args.sequence_only and args.interleave_resolver):
        if args.raw44_force_fail:
            assert all(outcome['steps'] == 81 and outcome['selector'] == 0xc07b and
                       outcome['selector_history'] == [(1, 0x82), (81, 0x7b)]
                       for outcome in sequence_outcomes), ('linked-throw miss and recovery', sequence_outcomes)
        elif args.raw44_force_success or args.raw44_index == 0:
            assert all(outcome['steps'] == 94 and outcome['state'] == 0 and
                       outcome['target_state']&0xff == 4 and
                       outcome['target_state4'] == 0x2005
                       for outcome in sequence_outcomes), ('linked-throw success', sequence_outcomes)
            assert '0x1865c' in sequence_outcomes[7]['descriptors'], (
                'linked-throw special target pose', sequence_outcomes[7])
    assert cases > 0, ('no differential fixtures selected', args.selector, args.sequence_only)
    assert bytes(memory.r_block(0, len(rom))) == rom
    machine.cleanup()
result = {'status': 'PASS', 'cases': cases,
          'steps_per_case': args.steps,
          'idle_descriptor': args.idle_descriptor if args.selector == 0x00 else None,
          'idle_edge': (args.idle_edge or 'center') if args.selector == 0x00 and args.sequence_only else None,
          'interleaved_physics': args.interleave_physics,
          'interleaved_resolver': args.interleave_resolver,
          'raw35_mode': args.raw35_mode if args.selector == 0x35 else None,
          'raw35_target_x': args.raw35_target_x if args.selector == 0x35 and args.interleave_resolver else None,
          'raw35_active': args.raw35_active if args.selector == 0x35 else None,
          'raw44_index': args.raw44_index if args.selector == 0x44 else None,
          'raw44_force_fail': args.raw44_force_fail if args.selector == 0x44 else None,
          'raw44_force_success': args.raw44_force_success if args.selector == 0x44 else None,
          'raw13_downed': args.raw13_downed if args.selector == 0x13 else None,
          'raw12_downed': args.raw12_downed if args.selector == 0x12 else None,
          'raw12_active': args.raw12_active if args.selector == 0x12 else None,
          'running_contact': args.running_contact if args.selector == 0x0e else None,
          'jump_contact': args.jump_contact if args.selector == 0x0c else None,
          'jump10_contact': args.jump10_contact if args.selector == 0x10 else None,
          'jump_attack': args.jump_attack if args.selector == 0x0d else None,
          'follow_impact': args.follow_impact if args.selector in (0x12, 0x13) else None,
          'linked_downed': args.linked_downed if args.selector in (0x11, 0x14) else None,
          'linked_active': args.linked_active if args.selector in (0x11, 0x14) else None,
          'victim_follow': args.victim_follow if args.selector in (0x0c, 0x0d, 0x0e, 0x10, 0x11, 0x12, 0x13, 0x14, 0x35) else None,
          'total_steps': total_steps,
          'total_victim_steps': (sum(outcome['victim_steps'] for outcome in sequence_outcomes)
                                 if args.victim_follow else None),
          'sequence_phases': [list(phase) for phase in sorted(sequence_phases)],
          'sequence_outcomes': sequence_outcomes,
          'raw35_rom_pcs': [hex(pc) for pc in sorted(raw35_pcs)] if args.selector == 0x35 else None,
          'raw35_success_characters': sorted(raw35_success_characters) if args.selector == 0x35 else None,
          'selectors': [hex(value) for value in selected]}
if args.report:
    result.update(rom_sha1=hashlib.sha1(rom).hexdigest(),
                  bridge_sha256=hashlib.sha256((HERE/'action_reversal_family_oracle_bridge.c').read_bytes()).hexdigest(),
                  test_sha256=hashlib.sha256(Path(__file__).read_bytes()).hexdigest(),
                  compared=(['RAM16KB and D0-D7 after every action tick',
                             'descriptor, visual pose, frame and timer after every action tick']
                            if args.selector == 0x00 else
                            ['RAM16KB and D0-D7 after every action and resolver tick',
                             'foreground video, palette and action descriptor after every action tick']
                            if args.selector == 0x8e else
                            ['RAM16KB and D0-D7 after every action, resolver and physics tick',
                             'action descriptor after every action tick']),
                  limits=(['One constructed idle animation per character and descriptor',
                           'No scheduler or physics runs between action ticks',
                           'Visual tile pixels and Saturn VDP1 output are not compared here']
                          if args.selector == 0x00 else
                          ['One constructed Royal interlude per character, not a complete Royal match',
                           'Visual tiles and animation artwork are not compared here',
                           'Only the state latch, arena resolver and action owners are interleaved']
                          if args.selector == 0x8e else
                          ['One constructed linked throw per character and outcome, not a complete match',
                           'The miss seed is selected by probing the original ROM for each character',
                           'Visual tiles and animation artwork are not compared here',
                           'Only the state latch, arena resolver and action owners are interleaved']
                          if args.selector == 0x44 else
                          ['One constructed on-ring launch per character, not a complete match',
                           'Visual tiles and animation artwork are not compared here',
                           'Only the action, state latch, arena resolver and physics owners are interleaved']))
    args.report.parent.mkdir(parents=True, exist_ok=True)
    args.report.write_text(json.dumps(result, indent=2)+'\n')
print(json.dumps(result))
