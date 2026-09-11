import copy
import importlib.util
import json
import pathlib
import unittest

ROOT = pathlib.Path(__file__).resolve().parents[2]
SPEC = importlib.util.spec_from_file_location("validator", ROOT / "tools" / "validate_final_memory_manifest.py")
validator = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(validator)
BASE = json.loads((ROOT / "manifests" / "final-memory-manifest.r03.json").read_text())

class TestValidator(unittest.TestCase):
    def test_candidate_structural_pass(self):
        errors, blockers = validator.validate(copy.deepcopy(BASE), final=False)
        self.assertEqual(errors, [])
        self.assertIn("VDP1_FINAL_PARTITION_OPEN", blockers)
        self.assertIn("HWRAM_LINK_MAP_OPEN", blockers)

    def test_candidate_final_fails_closed(self):
        errors, _ = validator.validate(copy.deepcopy(BASE), final=True)
        self.assertTrue(any("FINAL blocked" in e for e in errors))

    def test_overlap_rejected(self):
        m = copy.deepcopy(BASE)
        m["cram"]["regions"][1]["start"] = "0x200"
        errors, _ = validator.validate(m, final=False)
        self.assertTrue(any("simultaneous overlap" in e for e in errors))

    def test_lwram_dma_rejected(self):
        m = copy.deepcopy(BASE)
        m["dma"]["transfers"] = [{"name":"bad","src":"0x00201000","dst":"0x25E00000","bytes":4096,"class":"graphics_scu"}]
        errors, _ = validator.validate(m, final=False)
        self.assertTrue(any("DMA touches LWRAM" in e for e in errors))

    def test_graphics_dma_requires_hwram_source(self):
        m = copy.deepcopy(BASE)
        m["dma"]["transfers"] = [{"name":"bad","src":"0x25C10000","dst":"0x25E00000","bytes":4096,"class":"graphics_scu"}]
        errors, _ = validator.validate(m, final=False)
        self.assertTrue(any("graphics SCU DMA source not bounded HWRAM" in e for e in errors))

    def test_cdfs_cap_rejected(self):
        m = copy.deepcopy(BASE)
        m["cdfs_entries"] = 385
        errors, _ = validator.validate(m, final=False)
        self.assertTrue(any("cdfs_entries" in e for e in errors))

if __name__ == "__main__":
    unittest.main()
