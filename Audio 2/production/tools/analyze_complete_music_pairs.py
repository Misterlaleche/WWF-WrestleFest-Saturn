#!/usr/bin/env python3
"""Measure existing v1.1.0/v1.2.0 exported WAV pairs without generating audio.

PCM16/44100 input remains untouched. Channel statistics cover every frame.
Arithmetic mono is used only for analysis. Comparisons use the recorded epoch
and, separately, the documented native 4096-frame lead; neither is waveform-fit.
Envelope lag is observational. Welch uses all complete periodic Hann 4096
windows, hop 2048, no detrending, one-sided density and arithmetic averaging.
Statistics never confer fidelity, ROM_EXACT or owner/listening approval.
"""
from __future__ import annotations

import argparse
import hashlib
import importlib
import json
import math
from pathlib import Path
import platform
import re
import sys
import wave

import numpy as np

VERSION = "1.0.0"
EXPORTER_VERSIONS = ("1.1.0", "1.2.0")
PACKAGE = Path(__file__).resolve().parents[1]
RATE = 44100
ENVELOPE_FRAMES = 441
FFT_FRAMES = 4096
FFT_HOP = 2048
NATIVE_LEAD = 4096
MAX_LAG_BINS = 25
HASH_PATTERN = re.compile(r"[0-9a-f]{64}\Z")


def sha(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as stream:
        for block in iter(lambda: stream.read(1 << 20), b""):
            digest.update(block)
    return digest.hexdigest()


def fingerprint(path: Path) -> tuple:
    info = path.stat()
    return (info.st_dev, info.st_ino, info.st_size, info.st_mtime_ns, info.st_ctime_ns)


def integer(value, name: str, minimum: int = 0) -> int:
    if type(value) is not int or value < minimum:
        raise ValueError("Invalid " + name)
    return value


def require_sha(value, name: str) -> str:
    if not isinstance(value, str) or not HASH_PATTERN.fullmatch(value):
        raise ValueError("Invalid SHA-256 for " + name)
    return value


def wav_path(directory: Path, metadata: dict) -> Path:
    name = metadata.get("file")
    if (not isinstance(name, str) or not name or name in (".", "..")
            or "/" in name or "\\" in name or Path(name).name != name):
        raise ValueError("WAV filename must be a basename")
    path = (directory / name).resolve()
    if path.parent != directory.resolve():
        raise ValueError("WAV filename escapes its input directory")
    return path


def checked_wav(directory: Path, metadata: dict, frames: int, channels: int) -> tuple:
    if not isinstance(metadata, dict):
        raise ValueError("Missing WAV metadata")
    path = wav_path(directory, metadata)
    require_sha(metadata.get("sha256"), "WAV file")
    require_sha(metadata.get("pcm_sha256"), "WAV PCM")
    if integer(metadata.get("frames"), "WAV frames", 1) != frames:
        raise ValueError("WAV frame count differs from pair frame count")
    if integer(metadata.get("channels"), "WAV channels", 1) != channels:
        raise ValueError("Unexpected WAV channel count")
    if metadata.get("sample_rate") != RATE:
        raise ValueError("Unexpected WAV sample rate")
    if metadata.get("bits") != 16:
        raise ValueError("Expected PCM16 WAV")
    size = integer(metadata.get("bytes"), "WAV byte count", 1)
    stamp = fingerprint(path)
    if stamp[2] != size:
        raise ValueError("WAV byte count differs from manifest")
    if sha(path) != metadata["sha256"]:
        raise ValueError("File SHA-256 mismatch for " + metadata["file"])
    with wave.open(str(path), "rb") as stream:
        if stream.getcomptype() != "NONE" or stream.getsampwidth() != 2:
            raise ValueError("Expected uncompressed PCM16 WAV")
        if stream.getframerate() != RATE or stream.getnchannels() != channels:
            raise ValueError("WAV header sample rate/channel count differs")
        if stream.getnframes() != frames:
            raise ValueError("WAV header frame count differs")
    seconds = metadata.get("seconds")
    if (not isinstance(seconds, (int, float)) or isinstance(seconds, bool)
            or not math.isfinite(seconds) or not math.isclose(seconds * RATE, frames, abs_tol=1e-6)):
        raise ValueError("WAV duration differs from frame count")
    return path, stamp


def support(first, last, frames: int, nonzero_frames: int) -> dict:
    return {"first_nonzero_frame": first, "last_nonzero_frame": last,
            "first_nonzero_seconds": None if first is None else first / RATE,
            "last_nonzero_seconds": None if last is None else last / RATE,
            "leading_zero_frames": frames if first is None else first,
            "trailing_zero_frames": frames if last is None else frames - last - 1,
            "nonzero_frames": nonzero_frames}


class ChannelStatistics:
    """Exact integer moments and digital support over all original channels."""
    def __init__(self, channels: int):
        self.frames = 0
        self.sums = [0] * channels
        self.squares = [0] * channels
        self.minimum = [32767] * channels
        self.maximum = [-32768] * channels
        self.positive = [0] * channels
        self.negative = [0] * channels
        self.first = [None] * channels
        self.last = [None] * channels
        self.nonzero = [0] * channels
        self.any_nonzero = 0
        self.first_any = self.last_any = None

    def add(self, pcm: np.ndarray) -> None:
        values = pcm.astype(np.int64)
        active = np.flatnonzero(np.any(values != 0, axis=1))
        if active.size:
            if self.first_any is None:
                self.first_any = self.frames + int(active[0])
            self.last_any = self.frames + int(active[-1])
            self.any_nonzero += int(active.size)
        for channel in range(values.shape[1]):
            x = values[:, channel]
            self.sums[channel] += int(x.sum(dtype=np.int64))
            self.squares[channel] += int((x * x).sum(dtype=np.int64))
            self.minimum[channel] = min(self.minimum[channel], int(x.min()))
            self.maximum[channel] = max(self.maximum[channel], int(x.max()))
            self.positive[channel] += int(np.count_nonzero(x == 32767))
            self.negative[channel] += int(np.count_nonzero(x == -32768))
            active = np.flatnonzero(x)
            if active.size:
                if self.first[channel] is None:
                    self.first[channel] = self.frames + int(active[0])
                self.last[channel] = self.frames + int(active[-1])
                self.nonzero[channel] += int(active.size)
        self.frames += len(values)

    def finish(self) -> dict:
        channels = []
        for index in range(len(self.sums)):
            channels.append({"channel_index": index,
                "mean": self.sums[index] / self.frames,
                "rms": math.sqrt(self.squares[index] / self.frames),
                "minimum": self.minimum[index], "maximum": self.maximum[index],
                "peak_abs": max(abs(self.minimum[index]), abs(self.maximum[index])),
                "positive_rail_samples": self.positive[index],
                "negative_rail_samples": self.negative[index],
                "clip_rail_samples": self.positive[index] + self.negative[index],
                "support": support(self.first[index], self.last[index], self.frames, self.nonzero[index])})
        return {"frames": self.frames, "seconds": self.frames / RATE, "units": "raw PCM16 units",
                "clip_definition": "Samples exactly at -32768 or 32767; rail hits do not by themselves prove clipping distortion.",
                "channels": channels,
                "support": support(self.first_any, self.last_any, self.frames, self.any_nonzero)}


class RawComparison:
    """Use doubled mono integers to retain exact accumulated PCM moments."""
    def __init__(self):
        self.count = self.sx = self.sy = self.xx = self.yy = self.xy = self.error2 = self.max_error2 = 0

    def add(self, native2: np.ndarray, reference2: np.ndarray) -> None:
        x, y = native2.astype(np.int64), reference2.astype(np.int64)
        delta = x - y
        self.count += len(x)
        self.sx += int(x.sum(dtype=np.int64))
        self.sy += int(y.sum(dtype=np.int64))
        self.xx += int((x * x).sum(dtype=np.int64))
        self.yy += int((y * y).sum(dtype=np.int64))
        self.xy += int((x * y).sum(dtype=np.int64))
        self.error2 += int((delta * delta).sum(dtype=np.int64))
        self.max_error2 = max(self.max_error2, int(np.max(np.abs(delta))))

    def finish(self) -> dict:
        n = self.count
        vx, vy = n * self.xx - self.sx ** 2, n * self.yy - self.sy ** 2
        corr = None if not vx or not vy else (n * self.xy - self.sx * self.sy) / math.sqrt(vx * vy)
        return {"frames": n, "units": "raw PCM16 analysis-mono units",
                "native_rms": math.sqrt(self.xx / (4 * n)),
                "reference_rms": math.sqrt(self.yy / (4 * n)),
                "rmse": math.sqrt(self.error2 / (4 * n)),
                "mean_error": (self.sx - self.sy) / (2 * n),
                "maximum_absolute_error": self.max_error2 / 2,
                "correlation": None if corr is None else max(-1.0, min(1.0, corr)),
                "correlation_definition": "Pearson correlation, centered; null for zero variance."}


class Envelope:
    def __init__(self):
        self.pending = np.empty(0, dtype=np.float64)
        self.parts = []
        self.frames = 0

    def add(self, values: np.ndarray) -> None:
        self.frames += len(values)
        data = np.concatenate((self.pending, values))
        used = len(data) // ENVELOPE_FRAMES * ENVELOPE_FRAMES
        if used:
            blocks = data[:used].reshape(-1, ENVELOPE_FRAMES)
            self.parts.append(np.sqrt(np.mean(blocks * blocks, axis=1)))
        self.pending = data[used:].copy()

    def finish(self) -> np.ndarray:
        if len(self.pending):
            self.parts.append(np.array([math.sqrt(float(np.mean(self.pending * self.pending)))]))
        return np.concatenate(self.parts) if self.parts else np.empty(0, dtype=np.float64)


def correlation(x: np.ndarray, y: np.ndarray):
    if len(x) < 2:
        return None
    a, b = x - float(np.mean(x)), y - float(np.mean(y))
    aa, bb = float(np.sum(a * a)), float(np.sum(b * b))
    if not aa or not bb:
        return None
    return max(-1.0, min(1.0, float(np.sum(a * b)) / math.sqrt(aa * bb)))


def compare_envelopes(native: np.ndarray, reference: np.ndarray, frames: int) -> dict:
    maximum = min(MAX_LAG_BINS, max(0, len(native) - 2))
    lags = []
    for lag in range(-maximum, maximum + 1):
        if lag > 0:
            x, y = native[lag:], reference[:-lag]
        elif lag < 0:
            x, y = native[:lag], reference[-lag:]
        else:
            x, y = native, reference
        lags.append({"native_delay_bins": lag, "native_delay_ms": lag * 10,
                     "overlap_bins": len(x), "correlation": correlation(x, y)})
    defined = [item for item in lags if item["correlation"] is not None]
    best = max(defined, key=lambda item: (item["correlation"], -abs(item["native_delay_bins"]),
                                        -item["native_delay_bins"])) if defined else None
    return {"window_frames": ENVELOPE_FRAMES, "window_ms": 10, "windows": len(native),
            "final_window_frames": frames % ENVELOPE_FRAMES or ENVELOPE_FRAMES,
            "units": "raw PCM16 RMS units", "rmse": float(np.sqrt(np.mean((native - reference) ** 2))),
            "zero_lag_correlation": correlation(native, reference),
            "search_limit_ms": 250, "resolution_ms": 10,
            "lag_sign": "Positive means native is later: compare native_envelope[k+lag] with reference_envelope[k].",
            "best_native_delay_bins": None if best is None else best["native_delay_bins"],
            "best_native_delay_ms": None if best is None else best["native_delay_ms"],
            "best_correlation": None if best is None else best["correlation"],
            "lag_applied_to_output": False, "lag_observations": lags}


class Welch:
    def __init__(self):
        self.window = 0.5 - 0.5 * np.cos(2 * np.pi * np.arange(FFT_FRAMES) / FFT_FRAMES)
        self.scale = 1.0 / (RATE * float(np.sum(self.window * self.window)))
        self.pending = np.empty(0, dtype=np.float64)
        self.power = np.zeros(FFT_FRAMES // 2 + 1, dtype=np.float64)
        self.segments = self.frames = 0

    def add(self, values: np.ndarray) -> None:
        self.frames += len(values)
        data = np.concatenate((self.pending, values))
        count = 0 if len(data) < FFT_FRAMES else 1 + (len(data) - FFT_FRAMES) // FFT_HOP
        if count:
            windows = np.lib.stride_tricks.sliding_window_view(data, FFT_FRAMES)[::FFT_HOP][:count]
            transformed = np.fft.rfft(windows * self.window, axis=1)
            self.power += np.sum(transformed.real ** 2 + transformed.imag ** 2, axis=0)
            self.segments += count
        self.pending = data[count * FFT_HOP:].copy()

    def finish(self):
        if not self.segments:
            return None
        result = self.power * (self.scale / self.segments)
        result[1:-1] *= 2
        return result


def compare_spectra(native: Welch, reference: Welch) -> dict:
    x, y = native.finish(), reference.finish()
    result = {"available": x is not None, "window": "periodic Hann", "window_frames": FFT_FRAMES,
              "hop_frames": FFT_HOP, "fft_frames": FFT_FRAMES, "detrend": False,
              "one_sided": True, "scaling": "density", "average": "arithmetic mean",
              "units": "raw PCM16 analysis-mono units squared per Hz", "segments": native.segments,
              "frequency_step_hz": RATE / FFT_FRAMES, "frequency_bin_count": FFT_FRAMES // 2 + 1,
              "trailing_frames_without_full_segment": native.frames if not native.segments else
                  native.frames - ((native.segments - 1) * FFT_HOP + FFT_FRAMES)}
    if x is None:
        result["reason"] = "Fewer than 4096 shared frames; no padding or shortened window is substituted."
        return result
    energy_x, energy_y = float(np.sum(x * x)), float(np.sum(y * y))
    result.update({"native_psd": x.tolist(), "reference_psd": y.tolist(),
                   "native_integrated_power": float(np.sum(x) * RATE / FFT_FRAMES),
                   "reference_integrated_power": float(np.sum(y) * RATE / FFT_FRAMES),
                   "psd_rmse": float(np.sqrt(np.mean((x - y) ** 2))),
                   "psd_correlation": correlation(x, y),
                   "psd_cosine_similarity": None if not energy_x or not energy_y else
                       max(-1.0, min(1.0, float(np.sum(x * y)) / math.sqrt(energy_x * energy_y))),
                   "native_dominant_frequency_hz": None if not np.any(x) else int(np.argmax(x)) * RATE / FFT_FRAMES,
                   "reference_dominant_frequency_hz": None if not np.any(y) else int(np.argmax(y)) * RATE / FFT_FRAMES})
    return result


def compare_windows(native_path: Path, reference_path: Path, frames: int, native_start: int,
                    reference_start: int, block_frames: int, channel_statistics=None) -> tuple:
    raw = RawComparison()
    env_x, env_y, psd_x, psd_y = Envelope(), Envelope(), Welch(), Welch()
    digest_x, digest_y = hashlib.sha256(), hashlib.sha256()
    with wave.open(str(native_path), "rb") as native, wave.open(str(reference_path), "rb") as reference:
        native.setpos(native_start)
        reference.setpos(reference_start)
        remaining = frames
        while remaining:
            count = min(remaining, block_frames)
            bx, by = native.readframes(count), reference.readframes(count)
            if len(bx) != count * 4 or len(by) != count * 2:
                raise ValueError("Truncated PCM data in comparison window")
            digest_x.update(bx)
            digest_y.update(by)
            x = np.frombuffer(bx, dtype="<i2").reshape(-1, 2)
            y = np.frombuffer(by, dtype="<i2").reshape(-1, 1)
            if channel_statistics is not None:
                channel_statistics[0].add(x)
                channel_statistics[1].add(y)
            # Twice arithmetic mono, without gain normalization or rounding.
            x2 = x.astype(np.int64).sum(axis=1)
            y2 = y[:, 0].astype(np.int64) * 2
            raw.add(x2, y2)
            mono_x, mono_y = x2.astype(np.float64) * 0.5, y2.astype(np.float64) * 0.5
            env_x.add(mono_x)
            env_y.add(mono_y)
            psd_x.add(mono_x)
            psd_y.add(mono_y)
            remaining -= count
    result = {"available": True, "native_start_frame": native_start,
              "reference_start_frame": reference_start, "frames_compared": frames,
              "shared_seconds": frames / RATE, "equal_frame_counts": True,
              "raw": raw.finish(), "envelope": compare_envelopes(env_x.finish(), env_y.finish(), frames),
              "welch": compare_spectra(psd_x, psd_y)}
    return result, digest_x.hexdigest(), digest_y.hexdigest()


def validate_pair_identity(pair: dict) -> int:
    if not isinstance(pair, dict):
        raise ValueError("Pair must be an object")
    command = integer(pair.get("command"), "command", 1)
    kind = pair.get("kind")
    expected_id = "transitions" if kind == "transitions" else f"command-{command:02X}"
    if command > 18 or kind not in ("music", "transitions") or pair.get("id") != expected_id:
        raise ValueError("Invalid music/transition pair identity")
    if kind == "transitions" and command != 1:
        raise ValueError("Invalid transition command identity")
    frames = integer(pair.get("frames"), "pair frames", 1)
    if frames > (1 << 31):
        raise ValueError("Unsupported PCM frame count")
    seconds = pair.get("seconds")
    if (not isinstance(seconds, (int, float)) or isinstance(seconds, bool)
            or not math.isfinite(seconds) or not math.isclose(seconds * RATE, frames, abs_tol=1e-6)):
        raise ValueError("Pair duration differs from frame count")
    window = pair.get("native_window", {})
    if (window.get("documented_native_music_lead_preserved_frames") != NATIVE_LEAD
            or window.get("native_music_lead_removed_frames") != 0
            or window.get("waveform_fitted_alignment") is not False):
        raise ValueError("Expected documented native 4096-frame lead preserved without waveform fitting")
    if pair.get("complete_recorded_window") is not True:
        raise ValueError("Incomplete recorded pair window")
    return frames


def analyze_pair(pair: dict, wav_directory: Path, block_frames: int = 65536) -> dict:
    integer(block_frames, "block frame count", 1)
    if block_frames > 1 << 20:
        raise ValueError("Block frame count exceeds the bounded-memory limit")
    frames = validate_pair_identity(pair)
    native_path, before_x = checked_wav(wav_directory, pair["native_wav"], frames, 2)
    reference_path, before_y = checked_wav(wav_directory, pair["original_host_wav"], frames, 1)
    sx, sy = ChannelStatistics(2), ChannelStatistics(1)
    same, pcm_x, pcm_y = compare_windows(native_path, reference_path, frames, 0, 0, block_frames, (sx, sy))
    if pcm_x != pair["native_wav"]["pcm_sha256"] or pcm_y != pair["original_host_wav"]["pcm_sha256"]:
        raise ValueError("PCM SHA-256 mismatch in full recorded window")
    same["alignment"] = "Recorded common API epoch; no sample shift."
    fixed = {"available": False, "native_start_frame": NATIVE_LEAD, "reference_start_frame": 0,
             "frames_compared": 0, "reason": "Recorded window has no frames after the documented native lead."}
    if frames > NATIVE_LEAD:
        fixed, _, _ = compare_windows(native_path, reference_path, frames - NATIVE_LEAD,
                                       NATIVE_LEAD, 0, block_frames)
    fixed["alignment"] = "Analytical comparison of native[4096:N] with reference[0:N-4096]; documented offset, not fitted."
    fixed["waveform_fitted_alignment"] = False
    if fingerprint(native_path) != before_x or fingerprint(reference_path) != before_y:
        raise ValueError("Input WAV changed during pair analysis")
    public_fields = ("file", "sha256", "pcm_sha256", "frames", "sample_rate", "bits", "channels", "seconds", "bytes")
    return {"id": pair["id"], "kind": pair["kind"], "command": pair["command"],
            "seconds": frames / RATE, "frames": frames, "input_runtime_status": pair.get("runtime_status"),
            "native": {**{k: pair["native_wav"][k] for k in public_fields}, "verified": True, "full_signal": sx.finish()},
            "original_host": {**{k: pair["original_host_wav"][k] for k in public_fields}, "verified": True, "full_signal": sy.finish()},
            "comparisons": {"same_epoch": same, "known_native_lead_4096": fixed}}


def runtime_identity() -> tuple:
    files = {"analyze_complete_music_pairs.py": Path(__file__).resolve()}
    test = PACKAGE / "tests/test_analyze_complete_music_pairs.py"
    if test.is_file():
        files["test_analyze_complete_music_pairs.py"] = test
    dependency_paths = {"python-executable": Path(sys.executable),
                        "stdlib/wave.py": Path(wave.__file__), "numpy/__init__.py": Path(np.__file__)}
    for name in ("numpy.fft._pocketfft", "numpy.fft._pocketfft_umath", "numpy._core._multiarray_umath"):
        module = importlib.import_module(name)
        dependency_paths[name] = Path(module.__file__)
    source_hashes = {name: sha(path) for name, path in files.items()}
    dependencies = {"python": {"version": platform.python_version(), "implementation": platform.python_implementation()},
                    "numpy": {"version": np.__version__},
                    "file_sha256": {name: sha(path) for name, path in dependency_paths.items()}}
    return source_hashes, dependencies, {**files, **dependency_paths}


def repository_root() -> Path:
    for path in (PACKAGE, *PACKAGE.parents):
        if (path / ".git").exists():
            return path
    return PACKAGE


def validate_manifest(manifest: dict) -> bool:
    if (not isinstance(manifest, dict) or manifest.get("schema_version") != 1
            or manifest.get("tool_version") not in EXPORTER_VERSIONS):
        raise ValueError("Expected exporter v1.1.0/v1.2.0 public manifest schema 1")
    if manifest.get("complete_capture_export") is not True or manifest.get("source_stable") is not True:
        raise ValueError("Input export is incomplete or its sources were not stable")
    for name in ("normalization", "resampling", "time_stretch", "waveform_fitted_alignment"):
        if manifest.get(name) is not False:
            raise ValueError("Unexpected input audio transformation: " + name)
    if manifest.get("gain") != 1:
        raise ValueError("Expected unchanged input gain")
    pairs = manifest.get("pairs")
    if not isinstance(pairs, list) or not 1 <= len(pairs) <= 19:
        raise ValueError("Manifest needs between 1 and 19 declared pairs")
    for pair in pairs:
        validate_pair_identity(pair)
    if len({pair["id"] for pair in pairs}) != len(pairs):
        raise ValueError("Duplicate pair identity")
    filenames = [pair.get(kind, {}).get("file") for pair in pairs for kind in ("native_wav", "original_host_wav")]
    if any(not isinstance(name, str) for name in filenames) or len(set(filenames)) != len(filenames):
        raise ValueError("Missing or duplicate WAV filename")
    music = [pair for pair in pairs if pair["kind"] == "music"]
    transitions = [pair for pair in pairs if pair["kind"] == "transitions"]
    full = len(music) == 18 and {pair["command"] for pair in music} == set(range(1, 19)) and len(transitions) == 1
    diagnostic = manifest.get("diagnostic_only")
    if type(diagnostic) is not bool:
        raise ValueError("Missing diagnostic/full collection mode")
    if not diagnostic and (not full or manifest.get("complete_18_music_collection") is not True):
        raise ValueError("Full collection requires 18 music pairs and transitions")
    return full and not diagnostic and manifest.get("complete_18_music_collection") is True


def analyze_manifest(manifest_path: Path, out: Path, block_frames: int = 65536,
                     wav_directory: Path | None = None, progress=None) -> dict:
    manifest_path, out = Path(manifest_path).resolve(), Path(out).resolve()
    wav_directory = (Path(wav_directory) if wav_directory is not None else manifest_path.parent / "wav").resolve()
    if out.exists() or not out.parent.is_dir():
        raise ValueError("Output must be a new directory with an existing parent")
    if any(out == parent or parent in out.parents for parent in
           (repository_root(), manifest_path.parent, wav_directory)):
        raise ValueError("Output must be outside the repository and immutable input directories")
    integer(block_frames, "block frame count", 1)
    if block_frames > 1 << 20:
        raise ValueError("Block frame count exceeds the bounded-memory limit")
    manifest_digest, manifest_stamp = sha(manifest_path), fingerprint(manifest_path)
    manifest = json.loads(manifest_path.read_text())
    full = validate_manifest(manifest)
    sources, dependencies, code_paths = runtime_identity()
    # These are the identities actually recorded below. Taking a second baseline
    # here could conceal a source change immediately after runtime_identity().
    code_hashes = {**sources, **dependencies["file_sha256"]}
    inputs = {}
    for pair in manifest["pairs"]:
        for kind in ("native_wav", "original_host_wav"):
            metadata = pair[kind]
            path = wav_path(wav_directory, metadata)
            inputs[path] = (require_sha(metadata.get("sha256"), "WAV file"), fingerprint(path))
    out.mkdir(exist_ok=False)
    report = {"schema_version": 1, "tool_version": VERSION, "kind": "EXISTING_COMPLETE_MUSIC_PAIR_METRICS",
              "pair_manifest_sha256": manifest_digest, "input_exporter_version": manifest["tool_version"],
              "source_sha256": sources, "dependencies": dependencies,
              "sample_rate": RATE, "analysis_block_frames": block_frames,
              "memory_strategy": "Sequential pairs; bounded PCM/FFT blocks; retain only 10 ms envelopes and aggregate spectra, never full PCM.",
              "arithmetic_mono_analysis_only": True, "arithmetic_mono_definition": "(native_left+native_right)/2; reference remains its original mono channel.",
              "normalization": False, "gain_applied": 1, "resampling": False, "audio_files_written": False,
              "waveform_fitted_alignment": False, "known_native_lead_frames": NATIVE_LEAD,
              "complete": False, "analysis_complete": False, "source_stable": False,
              "statistical_thresholds_used_for_acceptance": False,
              "ROM_EXACT": False, "fidelity_approved": False, "owner_listening_approval": "PENDING",
              "hardware_status": "HARDWARE PENDING", "input_manifest_native_runtime_passed": manifest.get("native_runtime_passed"),
              "scope": "Descriptive metrics only; no waveform/ROM fidelity, owner approval, production or hardware certification.",
              "pairs": []}
    try:
        for pair in manifest["pairs"]:
            measured = analyze_pair(pair, wav_directory, block_frames)
            report["pairs"].append(measured)
            if progress is not None:
                progress(pair["id"], len(report["pairs"]), len(manifest["pairs"]))
        if sha(manifest_path) != manifest_digest or fingerprint(manifest_path) != manifest_stamp:
            raise ValueError("Input manifest changed during analysis")
        for path, (identity, stamp) in inputs.items():
            if fingerprint(path) != stamp or sha(path) != identity:
                raise ValueError("Input WAV changed during complete analysis")
        if any(sha(path) != code_hashes[name] for name, path in code_paths.items()):
            raise ValueError("Analysis source or dependency changed during analysis")
        report.update(complete=full, analysis_complete=True, source_stable=True,
                      analyzed_music_pairs=sum(pair["kind"] == "music" for pair in report["pairs"]),
                      analyzed_transition_pairs=sum(pair["kind"] == "transitions" for pair in report["pairs"]))
    except Exception as error:
        report["failure"] = {"type": type(error).__name__,
                             "scope": "Validation or analysis did not complete; no input or statistical acceptance is asserted."}
        with (out / "metrics.json").open("x") as stream:
            stream.write(json.dumps(report, indent=2, allow_nan=False) + "\n")
        raise
    with (out / "metrics.json").open("x") as stream:
        stream.write(json.dumps(report, indent=2, allow_nan=False) + "\n")
    return report


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--manifest", type=Path, required=True, help="Exporter's public-manifest.json")
    parser.add_argument("--wav-directory", type=Path, help="Defaults to the manifest's wav subdirectory")
    parser.add_argument("--out", type=Path, required=True, help="New directory outside repository and input trees")
    parser.add_argument("--block-frames", type=int, default=65536)
    args = parser.parse_args()
    def progress(pair_id, done, total):
        print(json.dumps({"id": pair_id, "analyzed_pairs": done, "declared_pairs": total}), flush=True)
    try:
        result = analyze_manifest(args.manifest, args.out, args.block_frames, args.wav_directory, progress)
    except (ValueError, OSError, EOFError, wave.Error) as error:
        print("Analysis failed: " + str(error), file=sys.stderr)
        return 1
    print(json.dumps({"complete": result["complete"], "analysis_complete": result["analysis_complete"],
                      "source_stable": result["source_stable"], "pair_manifest_sha256": result["pair_manifest_sha256"],
                      "metrics_sha256": sha(args.out.resolve() / "metrics.json")}), flush=True)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
