"""Independent artifact checks: wrong atlas cells or geometry must fail."""
import csv
import hashlib
import json
import io
import struct
import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parent
OUT = ROOT / "generated"


class AssetsTest(unittest.TestCase):
    def test_static_wordmarks_use_one_native_sprite(self):
        m = json.loads((OUT / "assets.json").read_text())
        static_frames = [q["frame"] for q in m["name_frames"]]
        static_frames += [m["credit_frame"], m["vs_frame"]]
        for index in static_frames:
            frame = m["frames"][index]
            self.assertEqual(frame["count"], 1, frame["label"])
            piece = m["pieces"][frame["first"]]
            self.assertEqual(piece["width"] % 8, 0)
            self.assertGreater(piece["width"], 0)
            self.assertLessEqual(piece["width"], 504)
            self.assertGreater(piece["height"], 0)
            self.assertLessEqual(piece["height"], 255)
            self.assertEqual(piece["flip"], 0)
            self.assertEqual(piece["flags"], 0)

    def test_generated_atlas_matches_authority(self):
        self.assertTrue((OUT / "assets.json").exists(), "ROM asset export has not been generated")
        manifest = json.loads((OUT / "assets.json").read_text())
        atlas = (OUT / "TEX.BIN").read_bytes()
        hashes = {r["code"]: r["sha256"] for r in csv.DictReader(io.StringIO((ROOT / "workpack_25/tables/sprite_tile_hashes.csv").read_text()))}
        for tile in manifest["tiles"]:
            if tile["kind"] == "sprite":
                actual = atlas[tile["atlas"] * 128:(tile["atlas"] + 1) * 128]
                self.assertEqual(hashlib.sha256(actual).hexdigest(), hashes[f'0x{tile["source"]:04X}'])
        expected = {}
        for r in csv.DictReader(io.StringIO((ROOT / "workpack_25/tables/metasprite_expanded_pieces.csv").read_text())):
            key = (int(r["set"], 16), int(r["pose"]))
            if (key[0] < 12 and key[1] in {0, 12, 89, 90, 91, 92, 93}) or key[0] in {12,31}:
                expected.setdefault(key, []).append(r)
        for pose in manifest["poses"]:
            rows = expected[(pose["set"], pose["pose"])]
            self.assertEqual(len(pose["pieces"]), len(rows))
            for p, r in zip(pose["pieces"], rows):
                self.assertEqual((p["source"],p["x"],p["y"],p["flip"]), (int(r["tile_code"],16),int(r["rel_x"]),int(r["rel_y"]),int(r["flip_x"])|int(r["flip_y"])<<1))
        presenter = [p for p in manifest["poses"] if p["set"] == 31]
        self.assertEqual({p["pose"] for p in presenter}, set(range(21)))
        self.assertLessEqual(len(atlas) + 1024 * 32 + 32, 524288)
        self.assertEqual(len((OUT / "CRAM.BIN").read_bytes()), 4096)

    def test_palette_preserves_every_original_channel(self):
        self.assertTrue((OUT / "assets.json").exists(), "ROM asset export has not been generated")
        m = json.loads((OUT / "assets.json").read_text())
        cram = struct.unpack('>1024I', (OUT / "CRAM.BIN").read_bytes())
        for bank in m["palette_banks"]:
            for i, raw in enumerate(bank["rgb444"]):
                r,g,b = raw&15,(raw>>4)&15,(raw>>8)&15
                # Supplied Yaul rgb888_t is big-endian bytes [CC,B,G,R].
                self.assertEqual(cram[bank["base"]+i], (b*17<<16)|(g*17<<8)|(r*17))


if __name__ == "__main__":
    unittest.main()
