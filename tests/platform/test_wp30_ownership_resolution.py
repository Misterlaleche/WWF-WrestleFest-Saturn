import copy
import importlib.util
import json
import pathlib
import unittest

ROOT=pathlib.Path(__file__).resolve().parents[2]
spec=importlib.util.spec_from_file_location("v",ROOT/"tools"/"validate_wp30_ownership_resolution.py")
v=importlib.util.module_from_spec(spec)
spec.loader.exec_module(v)
BASE=json.loads((ROOT/"manifests"/"wp30-orphan-resolution-r04.json").read_text())

class TestOwnership(unittest.TestCase):
    def test_pass(self):
        self.assertEqual(v.validate(copy.deepcopy(BASE)),[])
    def test_missing(self):
        m=copy.deepcopy(BASE)
        m["rows"].pop()
        self.assertTrue(v.validate(m))
    def test_cpu_owner(self):
        m=copy.deepcopy(BASE)
        m["rows"][3]["runtime_owner"]="integration-master-r01"
        self.assertTrue(v.validate(m))
    def test_false_close(self):
        m=copy.deepcopy(BASE)
        m["rows"][0]["implementation_status"]="HARDWARE_PASS"
        self.assertTrue(v.validate(m))

if __name__=="__main__":
    unittest.main()
