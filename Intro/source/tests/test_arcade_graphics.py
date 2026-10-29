"""Independent reference hashes recorded before WP0 implementation.

These fail if ROM bitplane order, palette order, sprite compositing, crop or
original CREDIT/INSERT COIN text is changed.
"""
import hashlib
import importlib.util
import json
import os
from pathlib import Path
import sys
import unittest
import zipfile

ROOT = Path(__file__).resolve().parents[2]
TOOLS = ROOT / "source" / "tools"
sys.path.insert(0, str(TOOLS))
ROM = Path(os.environ.get("WP0_ROM", ROOT.parent / "project_sources" / "01-wwfwfest.zip"))
STATES = Path(__file__).resolve().parent / "fixtures" / "intro_sources.zip"


class OriginalArcadePixels(unittest.TestCase):
    def test_four_original_rgb_hashes(self):
        spec = importlib.util.find_spec("arcade_graphics")
        self.assertIsNotNone(spec, "ROM-native graphic reconstruction is missing")
        from arcade_graphics import Graphics, read_legacy_state, render_rgb

        gfx = Graphics(ROM)
        with zipfile.ZipFile(STATES) as z:
            references = json.loads(z.read("references.json"))
            for name, expected in references.items():
                with self.subTest(frame=name):
                    rgb = render_rgb(gfx, read_legacy_state(z, name))
                    self.assertEqual(rgb.shape, (240, 320, 3))
                    self.assertEqual(hashlib.sha256(rgb.tobytes()).hexdigest(),
                                     expected["rgb_sha256"])


if __name__ == "__main__":
    unittest.main()
