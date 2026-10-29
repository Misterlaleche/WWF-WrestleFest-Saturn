"""Saturn-native assets must reproduce original independent pixel references."""
import hashlib
import importlib.util
import json
from pathlib import Path
import sys
import unittest
import zipfile

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / "source" / "tools"))
from arcade_graphics import Graphics, read_legacy_state
from test_arcade_graphics import ROM, STATES


class NativeAssets(unittest.TestCase):
    def test_original_pixels_survive_native_conversion_and_delta_replay(self):
        self.assertIsNotNone(importlib.util.find_spec("native_assets"), "Native Saturn asset compiler is missing")
        from native_assets import compile_sequence, NativeReplay
        gfx = Graphics(ROM)
        with zipfile.ZipFile(STATES) as z:
            refs = json.loads(z.read("references.json"))
            states = [read_legacy_state(z, key) for key in refs]
        compiled = compile_sequence(gfx, states)
        replay = NativeReplay(compiled)
        for expected in refs.values():
            self.assertTrue(replay.advance())
            rgb = replay.render()
            self.assertEqual(hashlib.sha256(rgb.tobytes()).hexdigest(), expected["rgb_sha256"])
        self.assertFalse(replay.advance())


if __name__ == "__main__":
    unittest.main()
