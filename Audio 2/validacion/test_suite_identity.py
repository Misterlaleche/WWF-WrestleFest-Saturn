"""A changed per-track input must stop the suite before any emulator is loaded."""
from pathlib import Path
import hashlib,json,sys,tempfile,types,unittest
from track_suite import run

HERE=Path(__file__).resolve().parent
ABC_SHA='ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad'

class SuiteIdentity(unittest.TestCase):
    def fixture(self,folder):
        source=folder/'MUSIC.ELF';source.write_bytes(b'abc')
        marker=folder/'emulator-loaded'
        plan=dict(pins={},suite_inputs={str(source):ABC_SHA},jobs=[dict(track=1,
            out=str(folder/'cold'),command=[sys.executable,'-B','-c',
                'from pathlib import Path; Path('+repr(str(marker))+').write_text("loaded")'])])
        return source,marker,plan

    def execute(self,folder,plan):
        (folder/'execution-plan.json').write_text(json.dumps(plan))
        run(types.SimpleNamespace(execute=True,suite=folder,tracks=None))

    def test_changed_track_image_rejects_before_loading_core(self):
        with tempfile.TemporaryDirectory(dir=HERE) as tmp:
            folder=Path(tmp);source,marker,plan=self.fixture(folder)
            source.write_bytes(b'abd')
            with self.assertRaises(ValueError):self.execute(folder,plan)
            self.assertFalse(marker.exists())
            self.assertFalse((folder/'cold').exists())

    def test_missing_track_input_rejects_before_loading_core(self):
        with tempfile.TemporaryDirectory(dir=HERE) as tmp:
            folder=Path(tmp);source,marker,plan=self.fixture(folder)
            source.unlink()
            with self.assertRaises(ValueError):self.execute(folder,plan)
            self.assertFalse(marker.exists())

    def test_old_unpinned_plan_cannot_run(self):
        with tempfile.TemporaryDirectory(dir=HERE) as tmp:
            folder=Path(tmp);source,marker,plan=self.fixture(folder)
            del plan['suite_inputs']
            with self.assertRaises(ValueError):self.execute(folder,plan)
            self.assertFalse(marker.exists())

    def test_unchanged_track_input_runs_the_requested_job(self):
        with tempfile.TemporaryDirectory(dir=HERE) as tmp:
            folder=Path(tmp);source,marker,plan=self.fixture(folder)
            self.execute(folder,plan)
            self.assertEqual(marker.read_text(),'loaded')

if __name__=='__main__':unittest.main()
