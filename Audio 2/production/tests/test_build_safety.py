import importlib.util,os,tempfile,unittest
from pathlib import Path

PACKAGE=Path(__file__).resolve().parents[1]
spec=importlib.util.spec_from_file_location('audio2_build',PACKAGE/'tools/build_saturn.py')
module=importlib.util.module_from_spec(spec);spec.loader.exec_module(module)

class BuildSafety(unittest.TestCase):
    def test_resource_output_alias_is_rejected_before_any_write(self):
        with tempfile.TemporaryDirectory() as d:
            root=Path(d);p=root/'SOUND.ROM';p.write_bytes(bytes(range(256))*256)
            before=p.read_bytes()
            with self.assertRaisesRegex(ValueError,'must not alias'):
                module.build(PACKAGE,root,root,root/'input.ELF',root/'absent-sdk',1,35)
            self.assertEqual(p.read_bytes(),before)
            self.assertFalse((root/'assets.S').exists())
    def test_firmware_output_alias_is_rejected_before_any_write(self):
        with tempfile.TemporaryDirectory() as d:
            root=Path(d);fw=root/'SCSP.ELF';fw.write_bytes(b'unchanged fixture')
            with self.assertRaisesRegex(ValueError,'must not alias'):
                module.build(PACKAGE,root,root/'resources',fw,root/'absent-sdk',1,35)
            self.assertEqual(fw.read_bytes(),b'unchanged fixture')
    def test_hard_link_alias_rejected(self):
        with tempfile.TemporaryDirectory() as d:
            root=Path(d);source=root/'resources';out=root/'out';source.mkdir();out.mkdir()
            rom=source/'SOUND.ROM';rom.write_bytes(b'x'*65536);os.link(rom,out/'SOUND.ROM')
            with self.assertRaisesRegex(ValueError,'must not alias'):
                module.build(PACKAGE,out,source,root/'input.ELF',root/'absent-sdk',1,35)
            self.assertEqual(rom.stat().st_size,65536)
if __name__=='__main__':unittest.main()
