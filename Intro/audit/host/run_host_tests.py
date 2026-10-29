#!/usr/bin/env python3
"""Run the delivered R04 behavioral tests without editing runtime sources."""
import hashlib
import json
import os
from pathlib import Path
import shlex
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[2]
SOURCE = ROOT / "source"
SUPPORT = ROOT / "audit" / "host"
PCM = ROOT / "audit" / "audio" / "TITLE02.PCM"
RIC = SOURCE / "cd" / "TITLE02.RIC"


def main():
    compiler = shlex.split(os.environ.get("CC", "cc"))
    common = compiler + ["-std=c11", "-O2", "-Wall", "-Wextra", "-Werror",
                         "-I" + str(SOURCE)]
    cases = [
        ("lossless_codec", [SOURCE / "tests/test_wp0_audio.c", SOURCE / "wp0_rice.c"],
         [], [str(RIC), str(PCM)]),
        ("audio_ring", [SOURCE / "tests/test_wp0_player.c", SOURCE / "wp0_rice.c",
                        SOURCE / "wp0_audio_player.c"], [], [str(RIC), str(PCM)]),
        ("audio_observation", [SUPPORT / "test_audio_observation.c",
                               SOURCE / "wp0_audio_player.c", SOURCE / "wp0_rice.c"], [], []),
        ("framebuffer_controller", [SUPPORT / "test_framebuffer.c", SOURCE / "wp0_framebuffer.c", SUPPORT / "wp0_timer_host.c"],
         ["-I" + str(SUPPORT / "host_include")], []),
        ("sequence_cache", [SOURCE / "tests/test_sequence_cache.c", SOURCE / "wp0_sequence_cache.c"], [], [str(SOURCE / "cd/SEQ.DAT")]),
    ]
    results = []
    with tempfile.TemporaryDirectory(prefix="wp0_r04_host_") as temporary:
        for name, inputs, extra, arguments in cases:
            binary = Path(temporary) / name
            command = common + extra + [str(p) for p in inputs] + ["-o", str(binary)]
            compile_result = subprocess.run(command, check=True, capture_output=True, text=True)
            result = subprocess.run([str(binary), *arguments], check=True, capture_output=True, text=True)
            results.append({"name": name, "result": "PASS", "output": result.stdout.strip(),
                            "compiler_diagnostics": compile_result.stderr.strip()})
    hashed = [*SOURCE.glob("*.c"), *SOURCE.glob("*.h"), PCM, RIC,
              SUPPORT / "test_framebuffer.c", SUPPORT / "test_audio_observation.c",
              SUPPORT / "host_include/yaul.h"]
    print(json.dumps({"scope": "Delivered R04 sources; host behavioral tests, not physical GPU timing",
                      "tests": results,
                      "sha256": {str(p.relative_to(ROOT)): hashlib.sha256(p.read_bytes()).hexdigest()
                                 for p in sorted(hashed)}}, indent=2))


if __name__ == "__main__":
    main()
