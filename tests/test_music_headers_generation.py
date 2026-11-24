import hashlib, importlib.util, json, shutil, struct, subprocess, sys, tempfile, unittest
from pathlib import Path

ROOT=Path(__file__).resolve().parents[1]
TOOL=ROOT/'tools/generate_music_headers.py'
ASSETS=ROOT/'cd'
spec=importlib.util.spec_from_file_location('headers',TOOL)
headers=importlib.util.module_from_spec(spec);spec.loader.exec_module(headers)

def wav(payload=b'\x00\x01'*16):
    return struct.pack('<4sI4s4sIHHIIHH4sI',b'RIFF',36+len(payload),b'WAVE',b'fmt ',16,1,1,22050,44100,2,16,b'data',len(payload))+payload

class GenerationTests(unittest.TestCase):
    def setUp(self):
        self.temp=tempfile.TemporaryDirectory();self.root=Path(self.temp.name);self.cd=self.root/'cd';self.cd.mkdir()
        for cue in range(1,19):(self.cd/f'MU{cue:02X}.WAV').write_bytes(wav())
        self.h=self.root/'headers.h';self.m=self.root/'headers.json'
    def tearDown(self):self.temp.cleanup()
    def test_generate_verify_exact(self):
        rows=headers.generate(self.cd,self.h,self.m)
        self.assertEqual(len(rows),18);self.assertEqual(sum(len(bytes.fromhex(r['header_hex']))+4 for r in rows),864)
        headers.verify(self.cd,self.h,self.m)
    def test_bad_headers(self):
        for offset in (0,8,12,16,20,22,24,28,32,34,36,40):
            with self.subTest(offset=offset):
                b=bytearray(wav());b[offset]^=0xFF
                with self.assertRaises(ValueError):headers.inspect('MU01.WAV',b)
    def test_truncated(self):
        for n in (0,12,36,43,44,63,75):
            with self.subTest(n=n),self.assertRaises(ValueError):headers.inspect('MU01.WAV',wav()[:n])
    def test_missing_cue(self):
        (self.cd/'MU12.WAV').unlink()
        with self.assertRaises(FileNotFoundError):headers.generate(self.cd,self.h,self.m)
        self.assertFalse(self.h.exists())
    def test_extra_cue(self):
        (self.cd/'MU13.WAV').write_bytes(wav())
        with self.assertRaises(ValueError):headers.generate(self.cd,self.h,self.m)
    def test_same_size_pcm_drift(self):
        headers.generate(self.cd,self.h,self.m);p=self.cd/'MU01.WAV';b=bytearray(p.read_bytes());b[-1]^=1;p.write_bytes(b)
        with self.assertRaises(ValueError):headers.verify(self.cd,self.h,self.m)
    def test_generated_header_drift(self):
        headers.generate(self.cd,self.h,self.m);self.h.write_text(self.h.read_text()+'\n')
        with self.assertRaises(ValueError):headers.verify(self.cd,self.h,self.m)
    def test_provenance_drift(self):
        headers.generate(self.cd,self.h,self.m);d=json.loads(self.m.read_text());d['files'][0]['size']+=2;self.m.write_text(json.dumps(d))
        with self.assertRaises(ValueError):headers.verify(self.cd,self.h,self.m)
    def test_odd_payload(self):
        with self.assertRaises(ValueError):headers.inspect('MU01.WAV',wav(b'123'))
    def test_real_18_wavs(self):
        real=ASSETS
        if not real.exists():self.fail('Asset directory missing: '+str(real))
        rows=headers.generate(real,self.h,self.m);headers.verify(real,self.h,self.m)
        self.assertEqual(len(rows),18)
    def test_make_gate_prevents_object_and_iso_recipes(self):
        headers.generate(self.cd,self.root/'port/wf_jo_music_headers.h',self.root/'tools/music-headers.json')
        shutil.copy2(TOOL,self.root/'tools/generate_music_headers.py')
        make=(TOOL.parents[1]/'makefile').read_text()
        block=make[make.index('# BEGIN WF MUSIC HEADER VERIFICATION'):make.index('# END WF MUSIC HEADER VERIFICATION')]
        (self.root/'probe.mk').write_text('ASSETS_DIR := cd\nTARGET := program-created\nTARGET2 := image-created\n'+block+'\nport/wf_jo_audio.o:\n\ttouch $@\nprogram-created:\n\ttouch $@\nimage-created:\n\ttouch $@\n')
        cmd=['make','-f','probe.mk','port/wf_jo_audio.o','program-created','image-created']
        good=subprocess.run(cmd,cwd=self.root,capture_output=True,text=True)
        self.assertEqual(good.returncode,0,good.stderr)
        mtimes={p:(self.root/p).stat().st_mtime_ns for p in ('port/wf_jo_audio.o','program-created','image-created')}
        repeat=subprocess.run(cmd,cwd=self.root,capture_output=True,text=True);self.assertEqual(repeat.returncode,0)
        self.assertEqual(mtimes,{p:(self.root/p).stat().st_mtime_ns for p in mtimes})
        for p in mtimes:(self.root/p).unlink()
        f=self.cd/'MU11.WAV';data=bytearray(f.read_bytes());data[-1]^=1;f.write_bytes(data)
        bad=subprocess.run(cmd,cwd=self.root,capture_output=True,text=True)
        self.assertNotEqual(bad.returncode,0)
        self.assertTrue(all(not (self.root/p).exists() for p in mtimes))

if __name__=='__main__':
    import argparse
    p=argparse.ArgumentParser();p.add_argument('--source',type=Path,default=ROOT);p.add_argument('--assets',type=Path);p.add_argument('--output',type=Path,default=ROOT/'build/music-headers-generation');a=p.parse_args()
    TOOL=a.source.resolve()/'tools/generate_music_headers.py';ASSETS=(a.assets or a.source/'cd').resolve()
    spec=importlib.util.spec_from_file_location('headers_under_test',TOOL);headers=importlib.util.module_from_spec(spec);spec.loader.exec_module(headers)
    a.output.mkdir(parents=True,exist_ok=True)
    with (a.output/'tests.log').open('w') as log:r=unittest.TextTestRunner(stream=log,verbosity=2).run(unittest.defaultTestLoader.loadTestsFromTestCase(GenerationTests))
    (a.output/'report.json').write_text(json.dumps({'status':'PASS' if r.wasSuccessful() else 'FAIL','tests':r.testsRun,'source':str(a.source.resolve()),'assets':str(ASSETS),'generator_sha256':hashlib.sha256(TOOL.read_bytes()).hexdigest()},indent=2)+'\n')
    print(r.testsRun,'tests', 'PASS' if r.wasSuccessful() else 'FAIL');raise SystemExit(not r.wasSuccessful())
