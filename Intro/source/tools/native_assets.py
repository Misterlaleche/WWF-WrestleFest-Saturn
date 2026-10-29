#!/usr/bin/env python3
"""Compile original video state into resident Saturn cell/sprite commands.

SEQ.DAT stores changes to tile maps and palette words, native sprite lists and
original scroll values. It contains no encoded screenshots or framebuffer video.
"""
import argparse
from dataclasses import dataclass
from functools import lru_cache
import hashlib
import json
from pathlib import Path
import struct
from dataclasses import replace
import numpy as np
from PIL import Image
from arcade_graphics import (Graphics, CAPTURE_RECORD_BYTES, read_capture_record,
                             sprite_records, pack_4bpp, pack_vdp2_pattern, rgb444)

BASES = (0, 0x20000, 0x40000)
MAP_BASES = (0x60000, 0x62000, 0x64000)
PALETTE_BASES = (256, 512, 0)
HEADER = struct.Struct(">IHHHH4HIHHI")


@dataclass
class Compiled:
    textures: list
    sprites: bytes
    sequence: bytes
    metadata: dict


def _map_codes(kind, raw):
    w = np.frombuffer(raw, ">u2")
    if kind == 0:
        return w[:1024] & 4095
    w = w.reshape(-1, 2)
    if kind == 1:
        return w[:, 1] & 4095
    return (w[:, 0] & 255) | ((w[:, 1] & 15) << 8)


def _runs(previous, current):
    changed = np.flatnonzero(previous != current)
    result = []
    if not len(changed):
        return result
    start = last = int(changed[0])
    for index in map(int, changed[1:]):
        if index - last <= 2:
            last = index
        else:
            result.append((start, current[start:last + 1]))
            start = last = index
    result.append((start, current[start:last + 1]))
    return result


def compile_sequence(gfx, states, audio_start_frame=0):
    used = [set(), set(), set(), set()]
    seen = [set(), set(), set()]
    max_sprites = 0
    for state in states:
        if state.flip:
            raise ValueError("Flipped cabinet state in sequence")
        for kind, raw in enumerate((state.bg1, state.bg0, state.text)):
            if raw not in seen[kind]:
                seen[kind].add(raw)
                used[kind].update(map(int, np.unique(_map_codes(kind, raw))))
        sprites = list(sprite_records(state))
        used[3].update(r[0] for r in sprites)
        max_sprites = max(max_sprites, len(sprites))
    codes = [sorted(s) for s in used]
    lookup = [{code: i for i, code in enumerate(values)} for values in codes]
    tex = [b"".join(pack_vdp2_pattern(gfx.tile("text" if kind == 2 else "background", code))
                    for code in values) for kind, values in enumerate(codes[:3])]
    sprtex = b"".join(pack_4bpp(gfx.tile("sprite", code)) for code in codes[3])
    for data in tex:
        if len(data) > 0x20000:
            raise ValueError("VDP2 atlas exceeds reserved VRAM bank")
    if len(sprtex) + (max_sprites + 4) * 32 > 0x80000:
        raise ValueError("VDP1 command/texture budget exceeded")

    @lru_cache(maxsize=256)
    def native_map(kind, raw):
        source_codes = _map_codes(kind, raw)
        w = np.frombuffer(raw, ">u2")
        index = np.array([lookup[kind][int(code)] for code in source_codes], np.uint32)
        result = (BASES[kind] >> 5) + index * (1 if kind == 2 else 4)
        if kind == 0:
            banks = w[:1024] >> 12
        elif kind == 1:
            attr = w.reshape(-1, 2)[:, 0].astype(np.uint32)
            banks = attr & 15
            result |= ((attr >> 6) & 1) << 30
            result |= ((attr >> 7) & 1) << 31
        else:
            banks = (w.reshape(-1, 2)[:, 1] >> 4) & 15
        result |= ((banks.astype(np.uint32) + PALETTE_BASES[kind] // 16) << 16)
        # Source text wraps every256px; VDP2's512px page repeats both halves.
        return np.tile(result, 2) if kind == 2 else result

    old_maps = [np.full(n, 0xffffffff, np.uint32) for n in (1024, 1024, 4096)]
    old_palette = np.full(1024, 0xffff, np.uint16)
    old_sprites = None
    stream = bytearray()
    maximum_packet = 0
    max_palette_updates = 0
    map_writes = 0
    frame_offsets = []
    for state in states:
        payload = bytearray()
        map_runs = 0
        for kind, raw in enumerate((state.bg1, state.bg0, state.text)):
            current = native_map(kind, raw)
            for first, values in _runs(old_maps[kind], current):
                payload.extend(struct.pack(">4H", kind, first, len(values), 0))
                payload.extend(values.astype(">u4").tobytes())
                map_runs += 1
                map_writes += len(values)
            old_maps[kind] = current
        pal = np.concatenate((state.palette[:256], state.palette[0xc00:0xd00],
                              state.palette[0x1000:0x1100], state.palette[0x400:0x500]))
        changed = np.flatnonzero(pal != old_palette)
        max_palette_updates = max(max_palette_updates, len(changed))
        for i in map(int, changed):
            payload.extend(struct.pack(">HH", i, int(pal[i])))
        old_palette = pal
        sprite_blob = b"".join(struct.pack(">HhhH", lookup[3][code], x, y, bank | (flip << 4))
                               for code, x, y, bank, flip in sprite_records(state))
        sprite_count = 0xffff
        if sprite_blob != old_sprites:
            payload.extend(sprite_blob)
            sprite_count = len(sprite_blob) // 8
            old_sprites = sprite_blob
        packet_size = HEADER.size + len(payload)
        assert not packet_size % 4
        frame_offsets.append(len(stream))
        stream.extend(HEADER.pack(packet_size, map_runs, len(changed), sprite_count, state.priority,
                                  *state.scroll, state.frame,
                                  2 if state.frame == audio_start_frame else 0, 0, 1))
        stream.extend(payload)
        maximum_packet = max(maximum_packet, packet_size)
    meta = dict(version=1, source="Original ROM bitplanes and full-rate original video device state",
                width=320, height=240, cram_mode=2, color_format="RGB888, original4bit channels expanded x17",
                frames=len(states), first_frame=states[0].frame, last_frame=states[-1].frame,
                arcade_clock_hz=7000000, arcade_htotal=448, arcade_vtotal=272,
                native_texture_bytes=list(map(len, tex)), native_sprite_texture_bytes=len(sprtex),
                tile_counts=list(map(len, codes)), max_sprites=max_sprites,
                map_bases=list(MAP_BASES), texture_bases=list(BASES),
                sequence_bytes=len(stream), largest_packet_bytes=maximum_packet,
                map_words_updated=map_writes, max_palette_updates=max_palette_updates,
                audio_start_source_frame=audio_start_frame,
                native_frame_offsets=frame_offsets,
                texture_source_ids=codes)
    return Compiled(tex, sprtex, bytes(stream), meta)


def _unpack_cell(data):
    packed = np.frombuffer(data, np.uint8).reshape(8, 4)
    result = np.empty((8, 8), np.uint8)
    result[:, ::2], result[:, 1::2] = packed >> 4, packed & 15
    return result


class NativeReplay:
    """Software interpretation of Saturn PNDs/4bpp data, not original ROM codes."""
    def __init__(self, compiled):
        self.compiled = compiled
        self.offset = 0
        self.maps = [np.zeros(n, np.uint32) for n in (1024, 1024, 4096)]
        self.palette = np.zeros(1024, np.uint16)
        self.sprites = []
        self.scroll = (0, 0, 0, 0)
        self.priority = 0
        self.dirty = [True] * 3
        self.map_pixels = [None] * 3

    def advance(self):
        data = self.compiled.sequence
        if self.offset == len(data):
            return False
        start = self.offset
        size, runs, colors, count, self.priority, *rest = HEADER.unpack_from(data, start)
        self.scroll = rest[:4]
        self.source_frame = rest[4]
        self.audio_command = rest[5]
        pos = start + HEADER.size
        for _ in range(runs):
            kind, first, length, _ = struct.unpack_from(">4H", data, pos)
            pos += 8
            self.maps[kind][first:first + length] = np.frombuffer(data, ">u4", length, pos)
            self.dirty[kind] = True
            pos += length * 4
        for _ in range(colors):
            index, value = struct.unpack_from(">HH", data, pos)
            self.palette[index] = value
            pos += 4
        if count != 0xffff:
            self.sprites = [struct.unpack_from(">HhhH", data, pos + i * 8) for i in range(count)]
            pos += count * 8
        if pos != start + size:
            raise ValueError("Native frame record has inconsistent bounds")
        self.offset = pos
        return True

    @lru_cache(maxsize=8192)
    def pattern(self, kind, word):
        address = (word & 0x7fff) * 32 - BASES[kind]
        if kind == 2:
            result = _unpack_cell(self.compiled.textures[kind][address:address + 32])
        else:
            result = np.empty((16, 16), np.uint8)
            for i in range(4):
                y, x = (i // 2) * 8, (i % 2) * 8
                result[y:y + 8, x:x + 8] = _unpack_cell(self.compiled.textures[kind][address + i * 32:address + (i + 1) * 32])
        if word & (1 << 30):
            result = result[:, ::-1]
        if word & (1 << 31):
            result = result[::-1]
        return result.astype(np.uint16) + ((word >> 16) & 127) * 16

    def layer(self, kind):
        if self.dirty[kind]:
            size = 8 if kind == 2 else 16
            columns = 512 // size
            result = np.empty((512, 512), np.uint16)
            for i, word in enumerate(self.maps[kind]):
                y, x = (i // columns) * size, (i % columns) * size
                result[y:y + size, x:x + size] = self.pattern(kind, int(word))
            self.map_pixels[kind] = result
            self.dirty[kind] = False
        return self.map_pixels[kind]

    def render(self):
        if self.priority not in (0x78, 0x7b, 0x7c):
            return np.zeros((240, 320, 3), np.uint8)
        fgx, fgy, bgx, bgy = self.scroll
        if self.priority != 0x78:
            fgx, fgy, bgx, bgy = bgx, bgy, fgx, fgy
        bg = self.layer(0)[(np.arange(240) + 8 + bgy) % 512][:, (np.arange(320) + bgx) % 512]
        fg = self.layer(1)[(np.arange(240) + 8 + fgy) % 512][:, (np.arange(320) + fgx) % 512]
        result = (bg if self.priority == 0x78 else fg).copy()
        def overlay(layer):
            mask = (layer & 15) != 0
            result[mask] = layer[mask]
        if self.priority != 0x7c:
            overlay(fg if self.priority == 0x78 else bg)
        for index, x, y, attr in self.sprites:
            packed = np.frombuffer(self.compiled.sprites[index * 128:(index + 1) * 128], np.uint8).reshape(16, 8)
            tile = np.empty((16, 16), np.uint8)
            tile[:, ::2], tile[:, 1::2] = packed >> 4, packed & 15
            if attr & 16:
                tile = tile[:, ::-1]
            if attr & 32:
                tile = tile[::-1]
            x0, y0, x1, y1 = max(0, x), max(0, y), min(320, x + 16), min(240, y + 16)
            cut = tile[y0 - y:y1 - y, x0 - x:x1 - x]
            mask = cut != 0
            result[y0:y1, x0:x1][mask] = 768 + (attr & 15) * 16 + cut[mask].astype(np.uint16)
        if self.priority == 0x7c:
            overlay(bg)
        overlay(self.layer(2)[8:248, :320])
        return rgb444(self.palette[result])


def save_compiled(compiled, destination):
    destination.mkdir(parents=True, exist_ok=True)
    files = dict(zip(("BG1.TEX", "BG0.TEX", "FONT.TEX"), compiled.textures))
    files.update({"SPR.TEX": compiled.sprites, "SEQ.DAT": compiled.sequence})
    for name, data in files.items():
        (destination / name).write_bytes(data)
    metadata = compiled.metadata.copy()
    metadata["files_sha256"] = {name: hashlib.sha256(data).hexdigest() for name, data in files.items()}
    (destination.parent / "native_assets.json").write_text(json.dumps(metadata, indent=2) + "\n")
    lines = ["/* Generated by tools/native_assets.py; original-ROM asset boundaries. */", "#pragma once"]
    for name, value in (("FRAME_COUNT", metadata["frames"]), ("VIDEO_BYTES", len(compiled.sequence)),
                        ("SPRITE_BYTES", len(compiled.sprites)), ("MAX_SPRITES", metadata["max_sprites"]),
                        ("FIRST_SOURCE_FRAME", metadata["first_frame"]),
                        ("LAST_SOURCE_FRAME", metadata["last_frame"]),
                        ("AUDIO_START_SOURCE_FRAME", metadata["audio_start_source_frame"])):
        lines.append(f"#define WP0_{name} {value}U")
    for i, name in enumerate(("BG1", "BG0", "FONT")):
        lines.append(f"#define WP0_{name}_BYTES {len(compiled.textures[i])}U")
    (destination.parent / "assets_generated.h").write_text("\n".join(lines) + "\n")


def main():
    p = argparse.ArgumentParser()
    p.add_argument("rom", type=Path)
    p.add_argument("capture", type=Path)
    p.add_argument("destination", type=Path)
    p.add_argument("--first", type=int, required=True)
    p.add_argument("--last", type=int, required=True)
    p.add_argument("--audio-start", type=int, default=0)
    p.add_argument("--check-frames", type=Path)
    p.add_argument("--corrections", type=Path)
    a = p.parse_args()
    raw = a.capture.read_bytes()
    if len(raw) % CAPTURE_RECORD_BYTES:
        raise ValueError("Capture must contain complete fixed-size records")
    states = [read_capture_record(raw[(i - 1) * CAPTURE_RECORD_BYTES:i * CAPTURE_RECORD_BYTES])
              for i in range(a.first, a.last + 1)]
    if a.corrections:
        corrections = json.loads(a.corrections.read_text())
        if hashlib.sha256(raw).hexdigest() != corrections["capture_sha256"]:
            raise ValueError("Raster evidence belongs to another original capture")
        by_frame = {s.frame: s for s in states}
        for item in corrections["corrections"]:
            if item["frame"] not in by_frame:
                continue
            state = by_frame[item["frame"]]
            if item["operation"] == "text_words":
                text = bytearray(state.text)
                if list(struct.unpack_from(">HH", text, item["byte_offset"])) != item["words_before"]:
                    raise ValueError("Raster correction precondition mismatch")
                struct.pack_into(">HH", text, item["byte_offset"], *item["words_displayed"])
                by_frame[state.frame] = replace(state, text=bytes(text))
            elif item["operation"] == "text_copy_from_frame":
                by_frame[state.frame] = replace(state, text=by_frame[item["source_frame"]].text)
            else:
                raise ValueError("Unknown raster correction operation")
        states = [by_frame[s.frame] for s in states]
    result = compile_sequence(Graphics(a.rom), states, a.audio_start)
    save_compiled(result, a.destination)
    if a.check_frames:
        replay = NativeReplay(result)
        mismatches = []
        while replay.advance():
            reference_path = a.check_frames / f"f{replay.source_frame:04d}.png"
            expected = np.asarray(Image.open(reference_path).convert("RGB"))
            actual = replay.render()
            different = int(np.count_nonzero(np.any(actual != expected, axis=2)))
            if different:
                mismatches.append({"frame": replay.source_frame, "pixels": different})
                if len(mismatches) <= 4:
                    Image.fromarray(actual).save(a.destination.parent / f"native_mismatch_{replay.source_frame:04d}.png")
        report = {"frames_compared": len(states), "frames_differing": len(mismatches), "mismatches": mismatches}
        (a.destination.parent / "pixel_validation.json").write_text(json.dumps(report, indent=2) + "\n")
        print(json.dumps({k:v for k,v in report.items() if k != "mismatches"}))
    print(json.dumps({k:v for k,v in result.metadata.items() if k not in ("native_frame_offsets", "texture_source_ids")}, indent=2))


if __name__ == "__main__":
    main()
