#!/usr/bin/env python3
"""ROM graphics decoder and independent arcade reference reconstruction.

The layouts and layer/sprite rules follow MAME's BSD-licensed ddragon3 driver.
Only original ROM bitplanes and captured device state are inputs; screenshots
are comparison data and never become Saturn textures.
"""
from dataclasses import dataclass
from functools import lru_cache
from pathlib import Path
import struct
import zipfile
import zlib
import numpy as np

SPRITE_ROMS = [
    ("31j3.ic9", 0xe395cf1d), ("31j2.ic8", 0xb5a97465),
    ("31j5.ic11", 0x2ce545e8), ("31j4.ic10", 0x00edb66a),
    ("31j6.ic12", 0x79956cf8), ("31j7.ic13", 0x74d774c3),
    ("31j9.ic15", 0xdd387289), ("31j8.ic14", 0x44abe127),
]
CAPTURE_RECORD_BYTES = 41528


@dataclass
class State:
    bg0: bytes
    bg1: bytes
    text: bytes
    sprites: bytes
    palette: np.ndarray
    scroll: tuple
    priority: int = 0x78
    flip: int = 0
    frame: int = 0
    time: float = 0.0
    pc: int = 0
    phase: int = 0
    flags: tuple = ()


class Graphics:
    def __init__(self, rom):
        with zipfile.ZipFile(rom) as z:
            parts = []
            for name, expected_crc in SPRITE_ROMS:
                data = z.read(name)
                if len(data) != 0x100000 or zlib.crc32(data) != expected_crc:
                    raise ValueError("Unexpected source graphics ROM: " + name)
                parts.append(data)
            self.raw_sprites = np.frombuffer(b"".join(parts), np.uint8).reshape(4, 65536, 32)
            self.font = z.read("31e12-0.ic33")
            self.bg = z.read("31j1.ic2") + z.read("31j0.ic1")
        if zlib.crc32(self.font) != 0x06f22615:
            raise ValueError("Unexpected original font ROM")
        if zlib.crc32(self.bg[:0x40000]) != 0x82ed7155 or zlib.crc32(self.bg[0x40000:]) != 0x8a12b450:
            raise ValueError("Unexpected original background ROM")

    @lru_cache(maxsize=None)
    def tile(self, kind, code):
        if kind == "sprite":
            bits = np.unpackbits(self.raw_sprites[:, code, :], axis=1)
            bits = bits.reshape(4, 2, 16, 8).transpose(0, 2, 1, 3).reshape(4, 16, 16)
            return np.sum(bits * np.array([8, 4, 2, 1], np.uint8)[:, None, None], axis=0, dtype=np.uint8)
        size = 8 if kind == "text" else 16
        result = np.zeros((size, size), np.uint8)
        for y in range(size):
            for x in range(size):
                if kind == "text":
                    byte = self.font[code * 32 + (x // 2) * 8 + y]
                    for p in range(4):
                        result[y, x] |= ((byte >> (7 - (1 - x % 2) - p * 2)) & 1) << (3 - p)
                else:
                    off = (code & 4095) * 64 + y * 2 + (32 if x >= 8 else 0)
                    for p, plane in enumerate((1, 0, 0x40001, 0x40000)):
                        result[y, x] |= ((self.bg[off + plane] >> (7 - x % 8)) & 1) << (3 - p)
        return result

    @lru_cache(maxsize=256)
    def tilemap(self, kind, raw):
        size = 8 if kind == "text" else 16
        columns = 64 if kind == "text" else 32
        out = np.zeros((size * 32, size * columns), np.uint16)
        words = np.frombuffer(raw, ">u2")
        if kind != "bg1":
            words = words.reshape(-1, 2)
        else:
            words = words[:1024]
        for i, w in enumerate(words):
            if kind == "bg1":
                code, bank, base, flags = int(w) & 4095, int(w) >> 12, 0xc00, 0
            elif kind == "bg0":
                flags, code = map(int, w)
                bank, base = flags & 15, 0x1000
            else:
                a, b = map(int, w)
                code, bank, base, flags = (a & 255) | ((b & 15) << 8), (b >> 4) & 15, 0, 0
            tile = self.tile("text" if kind == "text" else "background", code)
            if flags & 0x40:
                tile = tile[:, ::-1]
            if flags & 0x80:
                tile = tile[::-1]
            x, y = (i % columns) * size, (i // columns) * size
            out[y:y + size, x:x + size] = tile.astype(np.uint16) + base + bank * 16
        return out


def read_legacy_state(z, name):
    read = lambda suffix: z.read(name + suffix)
    pri, flip = map(int, read(".meta").split())
    return State(read(".bg0"), read(".bg1"), read(".fg"), read(".spr"),
                 np.frombuffer(read(".pal"), ">u2"), tuple(map(int, np.frombuffer(read(".scroll"), ">u2"))),
                 pri, flip, int(name[1:]))


def read_capture_record(raw):
    if len(raw) != CAPTURE_RECORD_BYTES:
        raise ValueError("Truncated original video-state record")
    index, frame, time, pc = struct.unpack_from(">IIdI", raw)
    values = struct.unpack_from(">14H", raw, 20)
    offset = 64
    blocks = []
    for size in (4096, 4096, 8192, 8192, 16384):
        blocks.append(raw[offset:offset + size])
        offset += size
    return State(*blocks[:4], np.frombuffer(blocks[4], ">u2"), values[:4],
                 values[7], values[5], index, time, pc, values[10], values)


def sprite_records(state, mask_chain=True):
    """Flatten original 16-pixel chains to native sprite objects, preserving order."""
    if state.flip:
        raise ValueError("Flipped cabinet capture is not part of this frontend")
    for w in np.frombuffer(state.sprites, ">u2").reshape(-1, 8):
        w = list(map(int, w))
        attr = w[1]
        if not attr & 1:
            continue
        x = (w[5] & 255) | ((attr & 4) << 6)
        if x > 496:
            x -= 512
        y = ((256 - ((w[0] & 255) | ((attr & 2) << 7))) & 511) - 16
        chain = (attr >> 5) & 7
        code = (w[2] & 255) | ((w[3] & 255) << 8)
        if mask_chain:
            code &= ~((1 << chain.bit_length()) - 1)
        for c in range(chain + 1):
            yy = (y - chain * 16 + c * 16 if attr & 8 else y - c * 16) - 8
            if x >= 320 or x <= -16 or yy >= 240 or yy <= -16:
                continue
            yield code + c, x, yy, w[4] & 15, (1 if attr & 16 else 0) | (2 if attr & 8 else 0)


def render_indices(gfx, state):
    if state.flip:
        raise ValueError("Unsupported flipped cabinet")
    if state.priority not in (0x78, 0x7b, 0x7c):
        return np.zeros((240, 320), np.uint16)
    fgx, fgy, bgx, bgy = state.scroll
    if state.priority != 0x78:
        fgx, fgy, bgx, bgy = bgx, bgy, fgx, fgy
    fg = gfx.tilemap("bg0", state.bg0)[(np.arange(240) + 8 + fgy) % 512][:, (np.arange(320) + fgx) % 512]
    bg = gfx.tilemap("bg1", state.bg1)[(np.arange(240) + 8 + bgy) % 512][:, (np.arange(320) + bgx) % 512]
    result = (bg if state.priority == 0x78 else fg).copy()
    def overlay(layer):
        mask = (layer & 15) != 0
        result[mask] = layer[mask]
    if state.priority != 0x7c:
        overlay(fg if state.priority == 0x78 else bg)
    for code, x, y, bank, flip in sprite_records(state):
        tile = gfx.tile("sprite", code)
        if flip & 1:
            tile = tile[:, ::-1]
        if flip & 2:
            tile = tile[::-1]
        x0, y0, x1, y1 = max(0, x), max(0, y), min(320, x + 16), min(240, y + 16)
        cut = tile[y0 - y:y1 - y, x0 - x:x1 - x]
        mask = cut != 0
        result[y0:y1, x0:x1][mask] = 0x400 + bank * 16 + cut[mask].astype(np.uint16)
    if state.priority == 0x7c:
        overlay(bg)
    overlay(gfx.tilemap("text", state.text)[8:248, :320])
    return result


def rgb444(words):
    return np.stack(((words & 15) * 17, ((words >> 4) & 15) * 17, ((words >> 8) & 15) * 17), axis=-1).astype(np.uint8)


def render_rgb(gfx, state):
    return rgb444(state.palette[render_indices(gfx, state)])


def pack_4bpp(tile):
    return ((tile[:, ::2] << 4) | tile[:, 1::2]).tobytes()


def pack_vdp2_pattern(tile):
    """16×16 VDP2 patterns contain four consecutive 8×8 characters."""
    if tile.shape == (8, 8):
        return pack_4bpp(tile)
    return b"".join(pack_4bpp(tile[y:y + 8, x:x + 8]) for y in (0, 8) for x in (0, 8))
