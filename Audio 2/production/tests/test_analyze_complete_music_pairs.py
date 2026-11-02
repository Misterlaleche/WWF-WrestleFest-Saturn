"""Synthetic-only checks for bounded analysis of exported PCM16 WAV pairs."""
import hashlib
import importlib.util
import json
from pathlib import Path
import tempfile
import unittest
from unittest import mock
import wave

import numpy as np
from scipy import signal

ROOT = Path(__file__).resolve().parents[1]
SPEC = importlib.util.spec_from_file_location(
    "analyze_complete_music_pairs", ROOT / "tools/analyze_complete_music_pairs.py")
ANALYSIS = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(ANALYSIS)
RATE = 44100


def sha(path):
    return hashlib.sha256(Path(path).read_bytes()).hexdigest()


def write_wav(path, samples, rate=RATE):
    samples = np.asarray(samples, dtype="<i2")
    if samples.ndim == 1:
        samples = samples[:, None]
    payload = samples.tobytes()
    with wave.open(str(path), "wb") as out:
        out.setparams((samples.shape[1], 2, rate, len(samples), "NONE", "not compressed"))
        out.writeframes(payload)
    return {"file": path.name, "sha256": sha(path),
            "pcm_sha256": hashlib.sha256(payload).hexdigest(),
            "frames": len(samples), "sample_rate": rate, "bits": 16,
            "channels": samples.shape[1], "seconds": len(samples) / rate,
            "bytes": path.stat().st_size}


def make_pair(directory, native, reference, command=1, transitions=False):
    suffix = "TRANSITIONS" if transitions else f"{command:02d}"
    native_meta = write_wav(directory / f"SATURN-MEDNAFEN-{suffix}.wav", native)
    reference_meta = write_wav(directory / f"ORIGINAL-BOARD-HOST-{suffix}.wav", reference)
    return {"id": "transitions" if transitions else f"command-{command:02X}",
            "kind": "transitions" if transitions else "music", "command": command,
            "hex": f"0x{command:02X}", "frames": len(reference),
            "seconds": len(reference) / RATE, "runtime_status": "PASS",
            "complete_recorded_window": True, "native_wav": native_meta,
            "original_host_wav": reference_meta,
            "native_window": {"documented_native_music_lead_preserved_frames": 4096,
                              "native_music_lead_removed_frames": 0,
                              "waveform_fitted_alignment": False}}


def make_manifest(directory, pairs, diagnostic=False):
    value = {"schema_version": 1, "tool_version": "1.1.0",
             "kind": "DIAGNOSTIC_NATIVE_ORIGINAL_AUDIO_PAIR" if diagnostic else
                     "FULL_NATIVE_ORIGINAL_MUSIC_PAIR_COLLECTION",
             "complete_capture_export": True, "complete_18_music_collection": not diagnostic,
             "source_stable": True, "diagnostic_only": diagnostic,
             "native_runtime_passed": True, "normalization": False, "gain": 1,
             "resampling": False, "time_stretch": False,
             "waveform_fitted_alignment": False, "pairs": pairs}
    path = directory / "public-manifest.json"
    path.write_text(json.dumps(value, indent=2) + "\n")
    return path


class CompletePairAnalysisTests(unittest.TestCase):
    def setUp(self):
        self.temp = tempfile.TemporaryDirectory()
        self.addCleanup(self.temp.cleanup)
        self.root = Path(self.temp.name)
        self.wav = self.root / "pairs/wav"
        self.wav.mkdir(parents=True)

    def analyze(self, pair, block=997):
        return ANALYSIS.analyze_pair(pair, self.wav, block_frames=block)

    def test_full_channel_support_rms_and_rails_are_exact(self):
        native = np.zeros((10007, 2), dtype=np.int16)
        native[17, 0] = -32768
        native[998, 0] = 32767
        native[8000, 1] = -2
        native[9001, 0] = 4
        pair = make_pair(self.wav, native, np.zeros(len(native), dtype=np.int16))
        result = self.analyze(pair)
        stats = result["native"]["full_signal"]
        self.assertEqual(stats["support"]["first_nonzero_frame"], 17)
        self.assertEqual(stats["support"]["last_nonzero_frame"], 9001)
        self.assertEqual(stats["support"]["trailing_zero_frames"], 1005)
        left, right = stats["channels"]
        self.assertEqual(left["peak_abs"], 32768)
        self.assertEqual(left["clip_rail_samples"], 2)
        self.assertEqual(left["positive_rail_samples"], 1)
        self.assertEqual(left["negative_rail_samples"], 1)
        self.assertEqual(right["support"]["first_nonzero_frame"], 8000)
        self.assertEqual(right["support"]["last_nonzero_frame"], 8000)
        for i, measured in enumerate(stats["channels"]):
            expected = np.sqrt(np.mean(native[:, i].astype(np.float64) ** 2))
            self.assertAlmostEqual(measured["rms"], expected, places=12)

    def test_arithmetic_mono_is_analysis_only(self):
        base = np.arange(-1000, 1000, dtype=np.int16)
        native = np.column_stack((base, -base))
        pair = make_pair(self.wav, native, np.zeros(len(base), dtype=np.int16))
        before = {p.name: sha(p) for p in self.wav.iterdir()}
        result = self.analyze(pair, block=73)
        self.assertGreater(result["native"]["full_signal"]["channels"][0]["rms"], 0)
        self.assertEqual(result["comparisons"]["same_epoch"]["raw"]["rmse"], 0)
        self.assertIsNone(result["comparisons"]["same_epoch"]["raw"]["correlation"])
        self.assertFalse(result["comparisons"]["known_native_lead_4096"]["available"])
        self.assertEqual(before, {p.name: sha(p) for p in self.wav.iterdir()})

    def test_raw_error_and_correlation_match_numpy(self):
        rng = np.random.default_rng(208)
        reference = rng.integers(-12000, 12000, 17003, dtype=np.int16)
        native = np.column_stack((reference.astype(np.int32) + 101,
                                  reference.astype(np.int32) - 39)).astype(np.int16)
        pair = make_pair(self.wav, native, reference)
        result = self.analyze(pair)
        raw = result["comparisons"]["same_epoch"]["raw"]
        actual = native.astype(np.float64).mean(axis=1)
        expected = reference.astype(np.float64)
        self.assertAlmostEqual(raw["rmse"], np.sqrt(np.mean((actual - expected) ** 2)), places=12)
        self.assertAlmostEqual(raw["mean_error"], 31, places=12)
        self.assertAlmostEqual(raw["correlation"], np.corrcoef(actual, expected)[0, 1], places=12)

    def test_known_native_lead_is_explicit_and_uses_equal_shared_frames(self):
        rng = np.random.default_rng(904)
        reference = rng.integers(-10000, 10000, 19001, dtype=np.int16)
        delayed = np.concatenate((np.zeros(4096, dtype=np.int16), reference[:-4096]))
        pair = make_pair(self.wav, np.column_stack((delayed, delayed)), reference)
        result = self.analyze(pair, block=2039)
        fixed = result["comparisons"]["known_native_lead_4096"]
        self.assertTrue(fixed["available"])
        self.assertEqual(fixed["native_start_frame"], 4096)
        self.assertEqual(fixed["reference_start_frame"], 0)
        self.assertEqual(fixed["frames_compared"], len(reference) - 4096)
        self.assertEqual(fixed["raw"]["rmse"], 0)
        self.assertAlmostEqual(fixed["raw"]["correlation"], 1)
        self.assertGreater(result["comparisons"]["same_epoch"]["raw"]["rmse"], 100)

    def test_envelope_lag_is_observational_with_positive_native_delay(self):
        rng = np.random.default_rng(11)
        amplitudes = rng.integers(500, 15000, 90)
        carrier = np.sin(2 * np.pi * 1000 * np.arange(90 * 441) / RATE)
        reference = np.rint(np.repeat(amplitudes, 441) * carrier).astype(np.int16)
        delayed = np.concatenate((np.zeros(5 * 441, dtype=np.int16), reference[:-5 * 441]))
        pair = make_pair(self.wav, np.column_stack((delayed, delayed)), reference)
        envelope = self.analyze(pair, block=571)["comparisons"]["same_epoch"]["envelope"]
        self.assertEqual(envelope["window_frames"], 441)
        self.assertEqual(envelope["best_native_delay_bins"], 5)
        self.assertEqual(envelope["best_native_delay_ms"], 50)
        self.assertAlmostEqual(envelope["best_correlation"], 1, places=12)
        self.assertFalse(envelope["lag_applied_to_output"])
        native_env = np.sqrt(np.mean(delayed.reshape(-1, 441).astype(np.float64) ** 2, axis=1))
        reference_env = np.sqrt(np.mean(reference.reshape(-1, 441).astype(np.float64) ** 2, axis=1))
        self.assertAlmostEqual(envelope["zero_lag_correlation"],
                               np.corrcoef(native_env, reference_env)[0, 1], places=12)

    def test_streaming_welch_matches_scipy_over_the_full_window(self):
        rng = np.random.default_rng(123)
        count = 17003
        reference = np.rint(1200 + 7000 * np.sin(2 * np.pi * 997 * np.arange(count) / RATE)
                            + rng.normal(0, 300, count)).astype(np.int16)
        native = np.column_stack((reference, (reference.astype(np.int32) // 2).astype(np.int16)))
        pair = make_pair(self.wav, native, reference)
        result = self.analyze(pair, block=997)
        same = result["comparisons"]["same_epoch"]
        psd = same["welch"]
        window = signal.windows.hann(4096, sym=False)
        for field, values in (("native_psd", native.astype(np.float64).mean(axis=1)),
                              ("reference_psd", reference.astype(np.float64))):
            _, expected = signal.welch(values, fs=RATE, window=window, nperseg=4096,
                                       noverlap=2048, nfft=4096, detrend=False,
                                       return_onesided=True, scaling="density", average="mean")
            np.testing.assert_allclose(psd[field], expected, rtol=2e-12, atol=1e-8)
        self.assertEqual(psd["segments"], 7)
        self.assertEqual(psd["trailing_frames_without_full_segment"], 619)
        self.assertEqual(same["envelope"]["windows"], (count + 440) // 441)
        other = self.analyze(pair, block=8000)
        np.testing.assert_allclose(other["comparisons"]["same_epoch"]["welch"]["native_psd"],
                                   psd["native_psd"], rtol=2e-12, atol=1e-8)

    def test_silence_has_finite_statistics_and_undefined_correlations(self):
        reference = np.zeros(8192, dtype=np.int16)
        pair = make_pair(self.wav, np.column_stack((reference, reference)), reference)
        result = self.analyze(pair)
        self.assertIsNone(result["native"]["full_signal"]["support"]["first_nonzero_frame"])
        self.assertEqual(result["native"]["full_signal"]["support"]["trailing_zero_frames"], 8192)
        same = result["comparisons"]["same_epoch"]
        self.assertEqual(same["raw"]["rmse"], 0)
        self.assertIsNone(same["raw"]["correlation"])
        self.assertIsNone(same["envelope"]["best_native_delay_bins"])
        self.assertIsNone(same["welch"]["native_dominant_frequency_hz"])
        json.dumps(result, allow_nan=False)

    def test_reads_are_bounded_and_never_request_the_full_recording(self):
        reference = np.arange(23000, dtype=np.int16)
        pair = make_pair(self.wav, np.column_stack((reference, reference)), reference)
        original = wave.Wave_read.readframes
        requests = []
        def checked(stream, frames):
            requests.append(frames)
            return original(stream, frames)
        with mock.patch.object(wave.Wave_read, "readframes", checked):
            self.analyze(pair, block=701)
        self.assertTrue(requests)
        self.assertTrue(all(0 < n <= 701 for n in requests))

    def test_hash_format_pcm_identity_and_frame_mismatches_are_rejected(self):
        reference = np.arange(9000, dtype=np.int16)
        pair = make_pair(self.wav, np.column_stack((reference, reference)), reference)
        for field, value, expected in (("sha256", "xyz", "SHA-256"),
                                      ("sha256", "0" * 64, "SHA-256"),
                                      ("pcm_sha256", "0" * 64, "PCM"),
                                      ("frames", 8999, "frame"),
                                      ("channels", 1, "channel"),
                                      ("sample_rate", 22050, "sample"),
                                      ("file", "../elsewhere.wav", "filename")):
            with self.subTest(field=field, value=value):
                altered = json.loads(json.dumps(pair))
                altered["native_wav"][field] = value
                with self.assertRaisesRegex(ValueError, expected):
                    self.analyze(altered)

    def test_truncated_pcm_is_rejected_even_with_updated_file_hash(self):
        reference = np.arange(9000, dtype=np.int16)
        pair = make_pair(self.wav, np.column_stack((reference, reference)), reference)
        path = self.wav / pair["native_wav"]["file"]
        path.write_bytes(path.read_bytes()[:-4])
        pair["native_wav"]["sha256"] = sha(path)
        pair["native_wav"]["bytes"] = path.stat().st_size
        with self.assertRaisesRegex(ValueError, "Truncated"):
            self.analyze(pair)

    def test_full_manifest_completeness_and_no_statistical_acceptance(self):
        reference = np.zeros(5000, dtype=np.int16)
        native = np.full((5000, 2), 32767, dtype=np.int16)
        pairs = [make_pair(self.wav, native, reference, i) for i in range(1, 19)]
        pairs.append(make_pair(self.wav, native, reference, 1, transitions=True))
        manifest = make_manifest(self.wav.parent, pairs)
        out = self.root / "analysis"
        result = ANALYSIS.analyze_manifest(manifest, out, block_frames=1009)
        self.assertTrue(result["complete"])
        self.assertTrue(result["source_stable"])
        self.assertEqual(result["pair_manifest_sha256"], sha(manifest))
        self.assertEqual(len(result["pairs"]), 19)
        self.assertFalse(result["fidelity_approved"])
        self.assertFalse(result["ROM_EXACT"])
        self.assertEqual(result["owner_listening_approval"], "PENDING")
        self.assertTrue((out / "metrics.json").is_file())
        encoded = (out / "metrics.json").read_text()
        self.assertNotIn(str(self.root), encoded)
        self.assertIn("numpy", result["dependencies"])
        self.assertEqual(len(result["source_sha256"]["analyze_complete_music_pairs.py"]), 64)
        with self.assertRaisesRegex(ValueError, "new"):
            ANALYSIS.analyze_manifest(manifest, out)

    def test_diagnostic_subset_is_not_a_complete_collection(self):
        reference = np.zeros(6000, dtype=np.int16)
        pair = make_pair(self.wav, np.column_stack((reference, reference)), reference)
        manifest = make_manifest(self.wav.parent, [pair], diagnostic=True)
        result = ANALYSIS.analyze_manifest(manifest, self.root / "diagnostic", block_frames=1024)
        self.assertTrue(result["analysis_complete"])
        self.assertFalse(result["complete"])

    def test_compatible_exporter_versions_are_explicit(self):
        reference = np.zeros(6000, dtype=np.int16)
        pair = make_pair(self.wav, np.column_stack((reference, reference)), reference)
        path = make_manifest(self.wav.parent, [pair], diagnostic=True)
        manifest = json.loads(path.read_text())
        for version in ("1.1.0", "1.2.0"):
            with self.subTest(version=version):
                manifest["tool_version"] = version
                path.write_text(json.dumps(manifest) + "\n")
                result = ANALYSIS.analyze_manifest(path, self.root / version)
                self.assertTrue(result["analysis_complete"])
                self.assertEqual(result["input_exporter_version"], version)
        manifest["tool_version"] = "2.0.0"
        with self.assertRaisesRegex(ValueError, "exporter"):
            ANALYSIS.validate_manifest(manifest)

    def test_wav_change_after_pair_is_detected_and_report_stays_incomplete(self):
        reference = np.zeros(6000, dtype=np.int16)
        pair = make_pair(self.wav, np.column_stack((reference, reference)), reference)
        manifest = make_manifest(self.wav.parent, [pair], diagnostic=True)
        original = ANALYSIS.analyze_pair
        def changed(*args, **kwargs):
            result = original(*args, **kwargs)
            with (self.wav / pair["native_wav"]["file"]).open("ab") as stream:
                stream.write(b"changed")
            return result
        out = self.root / "wav-changed"
        with mock.patch.object(ANALYSIS, "analyze_pair", changed):
            with self.assertRaisesRegex(ValueError, "changed"):
                ANALYSIS.analyze_manifest(manifest, out)
        report = json.loads((out / "metrics.json").read_text())
        self.assertFalse(report["complete"])
        self.assertFalse(report["analysis_complete"])
        self.assertFalse(report["source_stable"])

    def test_source_validation_uses_the_identity_actually_recorded(self):
        reference = np.zeros(6000, dtype=np.int16)
        pair = make_pair(self.wav, np.column_stack((reference, reference)), reference)
        manifest = make_manifest(self.wav.parent, [pair], diagnostic=True)
        source = self.root / "source-identity-fixture.py"
        source.write_text("before\n")
        original = ANALYSIS.runtime_identity
        def changed():
            hashes, dependencies, paths = original()
            hashes[source.name] = sha(source)
            paths[source.name] = source
            source.write_text("after\n")
            return hashes, dependencies, paths
        with mock.patch.object(ANALYSIS, "runtime_identity", changed):
            with self.assertRaisesRegex(ValueError, "source or dependency changed"):
                ANALYSIS.analyze_manifest(manifest, self.root / "source-changed")

    def test_manifest_change_during_analysis_is_detected(self):
        reference = np.zeros(6000, dtype=np.int16)
        pair = make_pair(self.wav, np.column_stack((reference, reference)), reference)
        manifest = make_manifest(self.wav.parent, [pair], diagnostic=True)
        original = ANALYSIS.analyze_pair
        def changed(*args, **kwargs):
            result = original(*args, **kwargs)
            manifest.write_text(manifest.read_text() + " ")
            return result
        with mock.patch.object(ANALYSIS, "analyze_pair", changed):
            with self.assertRaisesRegex(ValueError, "changed"):
                ANALYSIS.analyze_manifest(manifest, self.root / "changed")


if __name__ == "__main__":
    unittest.main()
