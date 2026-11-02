"""Private-delivery boundaries, with synthetic PCM only."""
import copy
import importlib.util
import json
from pathlib import Path
import struct
import tempfile
import unittest
import wave
import zipfile

TOOL = Path(__file__).resolve().parents[1] / 'tools/package_complete_music.py'
SPEC = importlib.util.spec_from_file_location('complete_package', TOOL)
package = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(package)


def full_manifest():
    pairs = [{'kind': 'music', 'command': c, 'seconds': 608, 'frames': 608 * 44100,
              'runtime_status': 'PASS', 'complete_recorded_window': True, 'native_audit_revalidated': True,
              'requested_events': [{'sample': 0, 'word': c}, {'sample': 26460000, 'word': 31}]}
             for c in range(1, 19)]
    pairs.append({'kind': 'transitions', 'command': 1, 'seconds': 32, 'frames': 32 * 44100,
                  'requested_events': [{'sample': i * 3 * 44100, 'word': word}
                    for i, word in enumerate((1, 6, 18, 31, 3, 31, 2, 1, 31))],
                  'runtime_status': 'PASS', 'complete_recorded_window': True, 'native_audit_revalidated': True})
    return {'complete_capture_export': True, 'source_stable': True, 'normalization': False,
            'gain': 1, 'resampling': False, 'time_stretch': False, 'waveform_fitted_alignment': False,
            'complete_18_music_collection': True, 'diagnostic_only': False,
            'native_runtime_passed': True, 'pairs': pairs,
            'source_sha256': {'tools/prepare_music_audio_pairs.py': package.digest(
                package.PACKAGE / 'tools/prepare_music_audio_pairs.py')}}


class CompleteMusicPackaging(unittest.TestCase):
    def test_rejects_false_duration_fitted_alignment_and_incomplete_transition(self):
        with tempfile.TemporaryDirectory() as temp:
            root = Path(temp)
            manifest = root / 'public-manifest.json'
            for mutate in ('four_frames_as_608s', 'fitted', 'two_transition_events', 'missing_transition_time'):
                bad = full_manifest()
                if mutate == 'four_frames_as_608s': bad['pairs'][0]['frames'] = 4
                elif mutate == 'fitted': bad['waveform_fitted_alignment'] = True
                elif mutate == 'two_transition_events': bad['pairs'][-1]['requested_events'] = [
                    {'sample': 0, 'word': 1}, {'sample': 44100, 'word': 31}]
                else: bad['pairs'][-1].pop('seconds')
                manifest.write_text(json.dumps(bad))
                with self.subTest(mutate=mutate), self.assertRaises(ValueError):
                    package.load_manifest(root, True)

    def test_pcm_hash_and_duration_must_match_the_real_wav(self):
        with tempfile.TemporaryDirectory() as temp:
            root = Path(temp)
            (root / 'wav').mkdir()
            path = root / 'wav/native.wav'
            values = struct.pack('<8h', 0, 0, 111, -111, -32768, 32767, 23, 31)
            with wave.open(str(path), 'wb') as sink:
                sink.setparams((2, 2, 44100, 0, 'NONE', 'not compressed'))
                sink.writeframes(values)
            pair = {'seconds': 608, 'frames': 4, 'native_wav': {
                'file': path.name, 'sha256': package.digest(path), 'bytes': path.stat().st_size,
                'frames': 4, 'seconds': 608, 'channels': 2, 'bits': 16, 'sample_rate': 44100,
                'pcm_sha256': '0' * 64, 'source_pcm_bytes_match': True}}
            with self.assertRaises(ValueError):
                package.wav_member(root, pair, 'native_wav', 'safe.wav')

    def test_partial_failed_or_relabelled_export_cannot_be_packaged_as_complete(self):
        with tempfile.TemporaryDirectory() as temp:
            root = Path(temp)
            manifest = root / 'public-manifest.json'
            good = full_manifest()
            manifest.write_text(json.dumps(good))
            self.assertEqual(len(package.load_manifest(root, True)[1]['pairs']), 19)
            for mutate in ('missing', 'failed', 'short', 'fitted', 'old_exporter', 'wrong_fade'):
                bad = copy.deepcopy(good)
                if mutate == 'missing': bad['pairs'].pop()
                elif mutate == 'failed': bad['pairs'][0]['runtime_status'] = 'FAIL'
                elif mutate == 'short': bad['pairs'][0]['seconds'] = 38
                elif mutate == 'fitted': bad['time_stretch'] = True
                elif mutate == 'old_exporter': bad['source_sha256'].clear()
                else: bad['pairs'][0]['requested_events'][1]['sample'] = 44100
                manifest.write_text(json.dumps(bad))
                with self.subTest(mutate=mutate), self.assertRaises(ValueError):
                    package.load_manifest(root, True)

    def test_reference_and_stereo_native_pcm_are_preserved_in_verified_zip(self):
        with tempfile.TemporaryDirectory() as temp:
            root = Path(temp)
            (root / 'wav').mkdir()
            path = root / 'wav/native.wav'
            values = struct.pack('<8h', 0, 0, 111, -111, -32768, 32767, 23, 31) * 11025
            with wave.open(str(path), 'wb') as sink:
                sink.setparams((2, 2, 44100, 0, 'NONE', 'not compressed'))
                sink.writeframes(values)
            pair = {'seconds': 1, 'frames': 44100, 'native_wav': {
                    'file': path.name, 'sha256': package.digest(path), 'bytes': path.stat().st_size,
                    'frames': 44100, 'seconds': 1, 'channels': 2, 'bits': 16, 'sample_rate': 44100,
                    'pcm_sha256': package.hashlib.sha256(values).hexdigest(), 'source_pcm_bytes_match': True}}
            member = package.wav_member(root, pair, 'native_wav', 'AUDIO2-COMPARATIVAS/MUSICAS/test.wav')
            wrong_pcm = copy.deepcopy(pair)
            wrong_pcm['native_wav']['pcm_sha256'] = '0' * 64
            with self.assertRaises(ValueError):
                package.wav_member(root, wrong_pcm, 'native_wav', 'safe.wav')
            meta = {'AUDIO2-COMPARATIVAS/LEEME.txt': b'Synthetic private test\n'}
            first = package.write_zip(root / 'first.zip', [member], meta)
            second = package.write_zip(root / 'second.zip', [member], meta)
            self.assertEqual(first['sha256'], second['sha256'])
            self.assertEqual(first['wav_files'], 1)
            with zipfile.ZipFile(root / 'first.zip') as archive:
                self.assertEqual(archive.read(member['member']), path.read_bytes())
                self.assertIn('AUDIO2-COMPARATIVAS/SHA256SUMS', archive.namelist())
            path.write_bytes(path.read_bytes()[:-1] + b'\x01')
            with self.assertRaises(ValueError):
                package.wav_member(root, pair, 'native_wav', member['member'])

    def test_paths_cannot_escape_private_pair_directory(self):
        with tempfile.TemporaryDirectory() as temp:
            root = Path(temp)
            for name in ('../escape.wav', '/escape.wav', '..', ''):
                pair = {'native_wav': {'file': name}}
                with self.subTest(name=name), self.assertRaises(ValueError):
                    package.wav_member(root, pair, 'native_wav', 'safe.wav')
            self.assertEqual(list(root.iterdir()), [])

    def test_archive_does_not_overwrite_existing_output_or_accept_duplicate_members(self):
        with tempfile.TemporaryDirectory() as temp:
            root = Path(temp)
            source = root / 'source'; source.write_bytes(b'synthetic')
            member = {'member': 'file', 'source': source, 'sha256': package.digest(source), 'bytes': 9}
            target = root / 'output.zip'
            package.write_zip(target, [member], {})
            before = package.digest(target)
            with self.assertRaises(FileExistsError): package.write_zip(target, [member], {})
            self.assertEqual(before, package.digest(target))
            with self.assertRaises(ValueError): package.write_zip(root / 'bad.zip', [member, member], {})
            self.assertFalse((root / 'bad.zip').exists())


if __name__ == '__main__':
    unittest.main()
