#!/usr/bin/env python3
from pathlib import Path
import argparse,hashlib,importlib.util,json,shutil,subprocess,tempfile,unittest

ROOT=Path(__file__).resolve().parents[1]
parser=argparse.ArgumentParser(add_help=False)
parser.add_argument('--source',type=Path,default=ROOT)
parser.add_argument('--assets',type=Path,default=ROOT/'cd')
parser.add_argument('--manifest',type=Path)
options,unittest_args=parser.parse_known_args()
SOURCE=options.source.resolve();ASSETS=options.assets.resolve()
MANIFEST=(options.manifest or SOURCE/'asset-lock.json').resolve()
SPEC=importlib.util.spec_from_file_location('gate',SOURCE/'tools/check_raw_disc_assets.py')
gate=importlib.util.module_from_spec(SPEC);SPEC.loader.exec_module(gate)


class Packaging(unittest.TestCase):
    def setUp(self):
        self.temporary=tempfile.TemporaryDirectory(prefix='raw-disc-gate-')
        self.path=Path(self.temporary.name);self.cd=self.path/'cd';self.cd.mkdir()
        for name in gate.NAMES:shutil.copy2(ASSETS/name,self.cd/name)
        self.lock=self.path/'asset-lock.json';shutil.copy2(MANIFEST,self.lock)
    def tearDown(self):self.temporary.cleanup()
    def relock(self,name):
        lock=json.loads(self.lock.read_text());data=(self.cd/name).read_bytes()
        lock['files'][name]={'bytes':len(data),'sha256':hashlib.sha256(data).hexdigest()}
        self.lock.write_text(json.dumps(lock))
    def test_current_four(self):
        self.assertEqual(gate.check(self.cd,self.lock)['status'],'PASS')
    def test_each_jolz_rejected_even_with_matching_manifest(self):
        for name in gate.NAMES:
            with self.subTest(name=name):
                original=(self.cd/name).read_bytes();(self.cd/name).write_bytes(b'JOLZ'+len(original).to_bytes(4,'big')+original)
                self.relock(name)
                with self.assertRaisesRegex(ValueError,'JoLZ'):gate.check(self.cd,self.lock)
                (self.cd/name).write_bytes(original);self.relock(name)
    def test_manifest_drift(self):
        data=bytearray((self.cd/'SFX.DAT').read_bytes());data[-1]^=1;(self.cd/'SFX.DAT').write_bytes(data)
        with self.assertRaisesRegex(ValueError,'asset-lock'):gate.check(self.cd,self.lock)
    def test_invalid_native_magic(self):
        data=bytearray((self.cd/'SPRITES.WFP').read_bytes());data[:4]=b'NOPE';(self.cd/'SPRITES.WFP').write_bytes(data);self.relock('SPRITES.WFP')
        with self.assertRaisesRegex(ValueError,'native format'):gate.check(self.cd,self.lock)
    def test_missing(self):
        (self.cd/'SCENES.WFP').unlink()
        with self.assertRaises(OSError):gate.check(self.cd,self.lock)
    def test_legacy_eight_k_raw_pack(self):
        spec=importlib.util.spec_from_file_location('builder',SOURCE/'tools/build_assets.py')
        builder=importlib.util.module_from_spec(spec);spec.loader.exec_module(builder)
        # Valid 8 KiB format generated locally; no historical work-tree fixture.
        packed=builder.build_sprite_pack(bytes(builder.SPRITE_BYTES),8192)
        (self.cd/'SPRITES.WFP').write_bytes(packed);self.relock('SPRITES.WFP')
        self.assertEqual(gate.check(self.cd,self.lock)['status'],'PASS')
    def test_make_gate_before_image_recipe(self):
        make=(SOURCE/'makefile').read_text();start=make.index('# BEGIN WF RAW DISC PACKAGING GATE')
        end=make.index('# END WF RAW DISC PACKAGING GATE',start)+len('# END WF RAW DISC PACKAGING GATE')
        block=make[start:end];marker=self.path/'image-created'
        mf=self.path/'probe.mk'
        mf.write_text('TARGET2 := '+str(marker)+'\nASSETS_DIR := '+str(self.cd)+'\nWF_ASSET_LOCK := '+str(self.lock)+'\n'+block+'\n'+str(marker)+':\n\ttouch '+str(marker)+'\n')
        cmd=['make','-f',str(mf),'WF_RAW_DISC_ASSETS=1',str(marker)]
        good=subprocess.run(cmd,cwd=SOURCE,capture_output=True,text=True);self.assertEqual(good.returncode,0,good.stderr);self.assertTrue(marker.exists());marker.unlink()
        original=(self.cd/'SFX.DAT').read_bytes();(self.cd/'SFX.DAT').write_bytes(b'JOLZ'+original);self.relock('SFX.DAT')
        bad=subprocess.run(cmd,cwd=SOURCE,capture_output=True,text=True);self.assertNotEqual(bad.returncode,0);self.assertFalse(marker.exists())
        # Opt-out preserves the old generic package path, including JoLZ.
        off=subprocess.run([*cmd[:3],'WF_RAW_DISC_ASSETS=0',str(marker)],cwd=SOURCE,capture_output=True,text=True)
        self.assertEqual(off.returncode,0,off.stderr);self.assertTrue(marker.exists())

if __name__=='__main__':unittest.main(argv=[__file__,*unittest_args])
