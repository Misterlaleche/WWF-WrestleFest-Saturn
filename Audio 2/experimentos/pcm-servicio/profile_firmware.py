"""Unmodified SCSP ELF in local M68K/DSP stepper; read-only instruction probes.

This measures the HOST feeder, not the historical Saturn run's364 late counts.
No Saturn emulator, guest patch, firmware rebuild or tolerance change.
"""
import argparse
import collections
import csv
import ctypes as C
import hashlib
import importlib.util
import json
import re
from pathlib import Path
import sys
import time

HERE = Path(__file__).resolve().parent
REPAIR = HERE.parents[2]
REPO = REPAIR.parents[2]
WORKSPACE = REPO.parent
sys.path.insert(0, str(WORKSPACE / "toolchains/sources/machine68k-0.4.1/build/lib.win-amd64-cpython-312"))
sys.path.insert(0, str(REPO / "tests/audio/native-driver"))
import machine68k
import test_music_firmware as original
from machine68k import Register

original.BUILD = REPAIR / "firmware"
original.PACK = REPAIR / "pcm/firmware-tests/input/inputs/SFX.DAT"
original.LIB = REPAIR / "pcm/hardware-adapter/dsp-hardware.dll"
original.YM = REPAIR / "host-tests/ym_oracle.dll"
original.TRACES = REPAIR / "pcm/firmware-tests/input/traces"


def sha(p):
    return hashlib.sha256(p.read_bytes()).hexdigest()


def signed(value):
    value &= 65535
    return value - 65536 if value & 32768 else value


class Probe(original.MusicFirmware):
    def __init__(self, quantum):
        super().__init__("runTest")
        self.quantum = 256  # Existing boot's bounded loop is sized for256-cycle calls.
        self.requested_quantum = quantum
        self.max_execute_cycles = 0
        self.events = {}
        self.consumed = []
        self.active = None
        self.pcm_controls = []
        self.routine_calls = []
        self.routine_stack = []
        self.mail_commands = []
        self.all_pcm_controls = []
        self.first_outputs = []
        self.pending_output = {}
        self.underrun_observations = []
        self.last_underruns = 0

    def step(self):
        ticks = self.cpu.execute(self.quantum).cycles
        self.max_execute_cycles = max(self.max_execute_cycles, ticks)
        self.cycles += ticks
        while self.frames * 256 < self.cycles:
            self.d.dsp_step()
            self.frames += 1
            for voice, start in list(self.pending_output.items()):
                if self.d.dsp_output(voice):
                    self.first_outputs.append(dict(start, first_output_frame=self.frames,
                                                   first_output_dsp=self.mem.r16(0xb010)))
                    del self.pending_output[voice]
        if hasattr(self, 'due_pc'):
            engine=self.symbols['wf_sfx_engine']
            underruns=self.mem.r32(engine+32)
            if underruns!=self.last_underruns:
                self.last_underruns=underruns
                self.underrun_observations.append(dict(count=underruns,frame=self.frames,
                    cpu_cycle=self.cycles,pc=self.cpu.r_pc(),
                    pcm=[dict(counters=list(__import__('struct').unpack('>6I',self.mem.r_block(engine+40+v*44,24))),
                              flags=self.mem.r_block(engine+40+v*44+38,6).hex()) for v in range(4)]))

    def command(self, code, argument=0):
        start = dict(code=code, argument=argument, request=(self.mem.r16(0xc048)+1)&65535,
                     start_cpu_cycles=self.cycles, start_frame=self.frames,
                     start_dsp=self.mem.r16(0xb010))
        result = super().command(code, argument)
        start.update(result=result, ack_cpu_cycles=self.cycles, ack_frame=self.frames,
                     ack_dsp=self.mem.r16(0xb010), ack_cycles=self.cycles-start["start_cpu_cycles"],
                     ack_fs=signed(self.mem.r16(0xb010)-start["start_dsp"]))
        self.mail_commands.append(start)
        return result

    def activate(self):
        self.quantum = self.requested_quantum
        self.max_execute_cycles = 0
        self.routine_entries = {self.symbols[name]: name for name in
                                ("wf_sfx_decode_block", "wf_scsp_sfx_work")}
        # Bind to the exact binary operand sequence. GCC keeps due inD1,
        # event inA0 and consumer inD0 for both frozen implementations.
        low = self.mem.r_block(0, 0x3000)
        sites = list(re.finditer(bytes.fromhex("32390000b01032509249"), low))
        assert len(sites) == 1
        self.due_pc = sites[0].start()+10
        assert low[self.due_pc]==0x6b
        branch_bytes=2 if low[self.due_pc+1] else 4
        assert low[self.due_pc+branch_bytes:self.due_pc+branch_bytes+4] == bytes.fromhex("0c410100")
        stores = list(re.finditer(b"\x33[\xc0-\xc7]\x00\x00\xb0\x22", low))
        assert len(stores) == 2
        self.consume_pcs = {site.start() for site in stores}
        self.cpu.set_instr_hook_callback(self.instruction)
        old_write = self.mmio_write

        def write(a, value):
            if a in (0x10070a, 0x10070c, 0x10070e, 0x100710):
                voice = (a-0x10070a)//2
                control = dict(voice=voice, value=value, frame=self.frames,
                               dsp_now=self.mem.r16(0xb010), cpu_cycles=self.cycles,
                               mail_request=self.mem.r16(0xc048),
                               mail_ack=self.mem.r16(0xc04a), mail_code=self.mem.r16(0xc04c),
                               mail_argument=self.mem.r16(0xc04e))
                if self.active is not None:
                    control.update(music_generation=self.active["generation"],
                                   music_index=self.active["index"], music_when=self.active["when"])
                self.all_pcm_controls.append(control)
                if value:
                    self.pending_output[voice] = control
                else:
                    self.pending_output.pop(voice, None)
            # DSP PCM3 gain/enable stores are real playback control, unlike kind4.
            if a in (0x100708, 0x100710) and self.active is not None:
                row = dict(index=self.active["index"], generation=self.active["generation"],
                           address=a, value=value, dsp_now=self.mem.r16(0xb010),
                           cpu_cycle_lower_bound=self.cycles,
                           delay_fs=signed(self.mem.r16(0xb010) - self.active["when"]))
                self.pcm_controls.append(row)
            old_write(a, value)

        self.mem.set_special_range_write_funcs(0x100000, 1, None, write, None)

    def instruction(self, pc):
        while self.routine_stack and pc == self.routine_stack[-1]["return_pc"]:
            call = self.routine_stack.pop()
            call["end_cpu_cycle_lower_bound"] = self.cycles
            self.routine_calls.append(call)
        if pc in self.routine_entries:
            sp = self.cpu.r_reg(Register.A7)
            call = dict(name=self.routine_entries[pc], start_cpu_cycle_lower_bound=self.cycles,
                        return_pc=self.mem.r32(sp))
            if call["name"] == "wf_sfx_decode_block":
                call["count"] = self.mem.r32(sp + 16)
                call["decoder"] = self.mem.r32(sp + 4)
                call["destination"] = self.mem.r32(sp + 12)
            self.routine_stack.append(call)
        if pc == self.due_pc:
            # SUB.W has already computed the exact due used by guest branches.
            address = self.cpu.r_reg(Register.A0)
            index = self.cpu.r_reg(Register.D0) & 65535
            generation = self.mem.r16(address + 6)
            row = self.events[(generation, index)]
            due = signed(self.cpu.r_reg(Register.D1))
            observation = dict(due_fs=due, dsp_now=self.mem.r16(0xb010),
                               cpu_cycle_lower_bound=self.cycles)
            row.setdefault("first_poll", observation)
            if due >= 0:
                assert "ready_poll" not in row
                row["ready_poll"] = observation
                self.active = row
        elif pc in self.consume_pcs:
            assert self.active is not None
            row = self.active
            row["consume"] = dict(dsp_now=self.mem.r16(0xb010),
                                  delay_fs=signed(self.mem.r16(0xb010) - row["when"]),
                                  cpu_cycle_lower_bound=self.cycles)
            self.consumed.append(row)
            self.active = None

    def feed(self, events, generation, seconds):
        index = 0
        frame_base = self.frames
        clock_base = self.mem.r16(0xb010)
        origin = frame_base + 512  # Existing feeder's unchanged lead.
        end = self.frames + 44100 * seconds
        direct = []
        direct_index = 0
        first = len(self.consumed)
        controls = len(self.pcm_controls)
        routines = len(self.routine_calls)
        old_late = self.mem.r16(0xb028)
        self.mem.w16(0xb024, generation)
        while self.frames < end:
            while index < len(events):
                relative, kind, reg, value = events[index]
                when = origin + relative
                if when > self.frames + 512:
                    break
                if kind == 0x80:
                    direct.append((when, reg, value))
                    index += 1
                    continue
                producer = self.mem.r16(0xb020)
                consumer = self.mem.r16(0xb022)
                if (producer - consumer) & 65535 >= 256:
                    break
                timestamp = (clock_base + when - frame_base) & 65535
                address = 0xb100 + (producer & 255) * 8
                self.mem.w_block(address, __import__("struct").pack(">4H", timestamp, kind * 256 + reg, value, generation))
                self.events[(generation, producer)] = dict(
                    index=producer, generation=generation, kind=kind, value=value, when=timestamp,
                    publication=dict(dsp_now=self.mem.r16(0xb010), frame=self.frames,
                                     due_fs=signed(self.mem.r16(0xb010)-timestamp),
                                     cpu_cycle_lower_bound=self.cycles))
                self.mem.w16(0xb020, (producer + 1) & 65535)
                index += 1
            while direct_index < len(direct) and direct[direct_index][0] <= self.frames:
                _, reg, value = direct[direct_index]
                self.mmio_write(0x100000 + reg * 2, value)
                direct_index += 1
            self.step()
            assert self.mem.r16(0xc046) == 2
        rows = self.consumed[first:]
        predicted = sum((r["ready_poll"]["due_fs"] > 256) + (r["ready_poll"]["due_fs"] > 2205) for r in rows)
        delta = (self.mem.r16(0xb028) - old_late) & 65535
        assert predicted == delta, (predicted, delta)
        by_kind = {}
        for kind in sorted({r["kind"] for r in rows}):
            sample = [r for r in rows if r["kind"] == kind]
            by_kind[kind] = dict(count=len(sample),
                late_gt256=sum(r["ready_poll"]["due_fs"] > 256 for r in sample),
                late_gt2205=sum(r["ready_poll"]["due_fs"] > 2205 for r in sample),
                max_publication_due_fs=max(r["publication"]["due_fs"] for r in sample),
                max_first_poll_due_fs=max(r["first_poll"]["due_fs"] for r in sample),
                max_ready_due_fs=max(r["ready_poll"]["due_fs"] for r in sample),
                max_consume_delay_fs=max(r["consume"]["delay_fs"] for r in sample),
                max_publish_to_first_poll_cycles_lower_bound=max(r["first_poll"]["cpu_cycle_lower_bound"]-r["publication"]["cpu_cycle_lower_bound"] for r in sample),
                max_ready_to_consume_cycles_lower_bound=max(r["consume"]["cpu_cycle_lower_bound"]-r["ready_poll"]["cpu_cycle_lower_bound"] for r in sample))
        return dict(generation=generation, seconds=seconds, late_counter_delta=delta,
                    by_kind=by_kind, consumed=rows, pcm3_controls=self.pcm_controls[controls:],
                    routine_calls=self.routine_calls[routines:],
                    pcm_engine_underruns=self.mem.r32(self.symbols["wf_sfx_engine"]+32))


def events_for(cmd, seconds, include_masks):
    result = []
    ym = C.CDLL(str(original.YM))
    position = [0]
    key = [0] * 8

    def converted(address, value, user):
        result.append((position[0], 0x80, (address - 0x100000) // 2, value))
        return True

    output = C.CFUNCTYPE(C.c_bool, C.c_uint32, C.c_uint16, C.c_void_p)(converted)
    ym.ym_reset(output)
    result.clear()
    summary = json.loads((original.TRACES / "capture-summary.json").read_text())
    name = f"cmd{cmd:02x}"
    base = summary["captures"][name]["origin_cycle"]
    with (original.TRACES / (name + ".csv")).open() as f:
        for e in csv.DictReader(f):
            if e["kind"] not in ("ym_write", "oki_write", "bank_write") or e["phase"] == "boot":
                continue
            position[0] = (int(e["cycle"]) - base) * 44100 // 3579545
            if position[0] > 44100 * seconds:
                continue
            kind = {"ym_write": 0, "oki_write": 1, "bank_write": 2}[e["kind"]]
            reg, value = int(e["reg"]), int(e["data"])
            if kind == 0:
                assert ym.ym_write(reg, value)
                if reg == 8 and key[value & 7] != (value >> 3) & 15:
                    key[value & 7] = (value >> 3) & 15
                    if include_masks:
                        mask = sum((1 << c) for c in range(8) if key[c])
                        result.append((position[0], 4, 0, mask))
            else:
                result.append((position[0], kind, reg, value))
    return result


def main():
    p = argparse.ArgumentParser()
    p.add_argument("--quantum", type=int, default=256)
    p.add_argument("--commands", default="1,2,3")
    p.add_argument("--seconds", type=int, default=3)
    p.add_argument("--include-masks", action="store_true")
    p.add_argument("--mail-catalogue", action="store_true")
    p.add_argument("--build", type=Path)
    p.add_argument("--elf-sha")
    p.add_argument("--out", type=Path, required=True)
    args = p.parse_args()
    if args.build:
        original.BUILD=args.build.resolve()
        assert args.elf_sha and sha(original.BUILD/'SCSP.ELF')==args.elf_sha
    inputs = [original.BUILD / "SCSP.ELF", original.BUILD / "symbols.json", original.PACK,
              original.LIB, original.YM, Path(machine68k.__file__), Path(__file__),
              REPO / "src/audio/native/wf_music_firmware.c", Path(original.__file__)]
    if not args.build:
        assert sha(inputs[0]) == "b882bec5cc5e3a1148ae8ae72cb05820d01cd5f8f125bf24ea2dd2edb8e5bb9c"
    else:
        inputs.append(original.BUILD/'BUILD.json')
    started = time.perf_counter()
    probe = Probe(args.quantum)
    probe.setUp()
    try:
        probe.activate()
        tracks = {}
        if args.mail_catalogue:
            for cmd in range(0x20, 0x82):
                assert probe.command(7, 0) == 0
                assert probe.command(11, (3 << 12) | (1 << 11) | (20 << 4) | 3) == 0
                result = probe.command(7, cmd)
                assert result in (0, 4)
                end = probe.frames + 512
                while probe.frames < end:
                    probe.step()
            print("all98 mailbox commands measured", flush=True)
        for cmd in (map(int, args.commands.split(",")) if args.commands else []):
            for voice in range(3):
                assert probe.command(11, (voice << 12) | (1 << 11) | (20 << 4) | 3) == 0
            result = probe.feed(events_for(cmd, args.seconds, args.include_masks), cmd, args.seconds)
            tracks[cmd] = result
            print(json.dumps(dict(cmd=cmd, late=result["late_counter_delta"], by_kind=result["by_kind"])), flush=True)
        report = dict(scope="HOST_ONLY_NOT_ATTRIBUTION_OF_HISTORICAL_SATURN_COUNTER",
                      unmodified_guest=True, no_saturn_emulator=True,
                      quantum_cpu_cycles=args.quantum, seconds_per_track=args.seconds,
                      include_mask_events=args.include_masks, lead_fs=512, soft_deadline_fs=256,
                      hard_diagnostic_threshold_fs=2205,
                      max_execute_cycles=probe.max_execute_cycles,
                      due_probe_pc=probe.due_pc, consumer_store_pcs=sorted(probe.consume_pcs),
                      cpu_cycle_measurement="instruction-hook cycle is lower bound within current execute; uncertainty <=max_execute_cycles",
                      step_clock_ratio="11289600 CPUcycles /256 per DSP sample =44100Fs",
                      inputs={str(x): sha(x) for x in inputs}, tracks=tracks,
                      mail_commands=probe.mail_commands, pcm_controls=probe.all_pcm_controls,
                      first_nonzero_outputs=probe.first_outputs,
                      underrun_observations=probe.underrun_observations,
                      wall_seconds=time.perf_counter()-started)
        args.out.parent.mkdir(parents=True, exist_ok=True)
        args.out.write_text(json.dumps(report, indent=2)+"\n")
    finally:
        probe.cpu.set_instr_hook_callback(None)
        probe.tearDown()


if __name__ == "__main__":
    main()
