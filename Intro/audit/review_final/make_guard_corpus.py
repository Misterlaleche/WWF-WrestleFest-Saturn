#!/usr/bin/env python3
"""Independent bitstream fixtures for ASan/UBSan decoder review.

Records: BE32 input length, BE16 output capacity, BE16 expected sample count;
expected count FFFF means compatibility only, zero requires rejection.
Successful fixtures append independently encoded BE16 expected PCM words.
"""
import importlib.util
import json
import random
import struct
from pathlib import Path

HERE = Path(__file__).resolve().parent
ROOT = HERE.parents[1]
spec = importlib.util.spec_from_file_location("equivalence", ROOT / "audit/audio/verify_decoder_equivalence.py")
encoder = importlib.util.module_from_spec(spec)
spec.loader.exec_module(encoder)
rng = random.Random(0x52455634)
counts = {"valid": 0, "must_reject": 0, "compatibility": 0}

with (HERE / "guard_corpus.bin").open("wb") as output:
    def add(data, capacity=1024, expected=None, reject=False):
        n = len(expected) if expected is not None else 0 if reject else 0xFFFF
        output.write(struct.pack(">IHH", len(data), capacity, n))
        output.write(data)
        if expected is not None:
            output.write(struct.pack(">" + str(n) + "h", *expected))
        counts["valid" if expected is not None else "must_reject" if reject else "compatibility"] += 1

    for order in (1, 2):
        for k in range(17):
            vectors = [
                [-32768, 32767, -32768],
                [32767, -32768, 32767],
                [-32768] * 5,
                [32767] * 5,
                [0] * order,
            ]
            for n in (3, 4, 8, 9, 17, 31, 1024):
                walk = [rng.randrange(-1000, 1001)]
                while len(walk) < n:
                    walk.append(walk[-1] + rng.randrange(-8, 9))
                vectors.append(walk)
            for values in vectors:
                data = encoder.packet(values, order, k)
                add(data, len(values), values)
                add(data, len(values) - 1, reject=True)
                # Match the shortened payload field, so rejection exercises
                # body bounds rather than only the header-length mismatch.
                cuts = sorted({0, 1, 7, 8, 9, len(data) // 2, len(data) - 1})
                for cut in cuts:
                    if cut >= len(data):
                        continue
                    truncated = bytearray(data[:cut])
                    if cut >= 8:
                        truncated[2:4] = struct.pack(">H", cut - 8)
                    add(bytes(truncated), len(values), reject=True)
                # Canonical streams allow at most seven zero padding bits,
                # so a complete additional byte must be rejected.
                extra = bytearray(data + b"\0")
                extra[2:4] = struct.pack(">H", len(extra) - 8)
                add(bytes(extra), len(values), reject=True)
                bit_count = 0
                for i in range(order, len(values)):
                    predictor = values[i - 1] if order == 1 else 2 * values[i - 1] - values[i - 2]
                    residual = values[i] - predictor
                    u = residual * 2 if residual >= 0 else -2 * residual - 1
                    bit_count += (u >> k) + 1 + k
                if bit_count % 8:
                    bad_padding = bytearray(data)
                    bad_padding[-1] |= 1
                    add(bytes(bad_padding), len(values), reject=True)

            maximum = 131070 if order == 1 else 262140
            # Excessive quotient, both with and without a terminal one.
            for terminator in ("", "1"):
                bits = "0" * ((maximum >> k) + 1) + terminator + "0" * k
                bits += "0" * ((-len(bits)) % 8)
                payload = b"\0\0" * order + bytes(int(bits[i:i + 8], 2) for i in range(0, len(bits), 8))
                data = struct.pack(">HHBBBB", order + 1, len(payload), order, k, 0, 0) + payload
                add(data, order + 1, reject=True)

    # Arbitrary malformed bodies exercise partial-output equivalence, not
    # only acceptance and sample counts. The C runner compares every word.
    for _ in range(20000):
        n = rng.randrange(0, 1030)
        order = rng.randrange(4)
        k = rng.randrange(18)
        payload = rng.randbytes(rng.randrange(0, 300))
        data = struct.pack(">HHBBBB", n, len(payload), order, k, 0, 0) + payload
        add(data, rng.choice((0, 1, 2, 3, 17, 1024)))

    # Every shipped block is allocated exactly to its declared byte size.
    data = (ROOT / "source/cd/TITLE02.RIC").read_bytes()
    pcm = (ROOT / "audit/audio/TITLE02.PCM").read_bytes()
    offset = 32
    sample = 0
    while offset < len(data):
        n, size = struct.unpack_from(">HH", data, offset)
        values = list(struct.unpack_from(">" + str(n) + "h", pcm, 2 * sample))
        add(data[offset:offset + 8 + size], n, values)
        offset += 8 + size
        sample += n
    assert offset == len(data) and sample * 2 == len(pcm)

(HERE / "guard_corpus.json").write_text(json.dumps(counts, indent=2) + "\n")
print(json.dumps(counts, indent=2))
