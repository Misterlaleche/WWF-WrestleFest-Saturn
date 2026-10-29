#!/usr/bin/env python3
"""Reproduce the final decoder ASan/UBSan review using only packaged inputs.

The original reproduce_guard_review.sh is preserved as historical evidence.
This portable runner generates the corpus and builds all executables in a
new temporary directory. It leaves the package and its manifest unchanged.
"""
import hashlib
import json
import os
from pathlib import Path
import shlex
import shutil
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[2]


def main():
    evidence = json.loads((ROOT/'audit/review_final/final_review_evidence.json').read_text())
    with tempfile.TemporaryDirectory(prefix='wp0_r04_guard_review_') as td:
        stage = Path(td)
        for rel in ('audit/review_final/make_guard_corpus.py',
                    'audit/audio/verify_decoder_equivalence.py',
                    'audit/audio/TITLE02.PCM', 'source/cd/TITLE02.RIC'):
            target = stage/rel
            target.parent.mkdir(parents=True, exist_ok=True)
            shutil.copyfile(ROOT/rel, target)
        subprocess.run(['python3', str(stage/'audit/review_final/make_guard_corpus.py')],
                       check=True, capture_output=True, text=True)
        corpus = stage/'audit/review_final/guard_corpus.bin'
        corpus_hash = hashlib.sha256(corpus.read_bytes()).hexdigest()
        assert corpus_hash == evidence['sha256']['audit/review_final/guard_corpus.bin']
        flags = shlex.split(os.environ.get('CC', 'cc')) + [
            '-std=c11', '-O1', '-g', '-fsanitize=address,undefined',
            '-fno-omit-frame-pointer', '-Wall', '-Wextra', '-Werror']
        subprocess.run(flags + ['-Dwp0_rice_decode_block=old_decode',
                               '-Dwp0_rice_open=old_open', '-Dwp0_rice_next=old_next',
                               '-c', str(ROOT/'audit/reference/r03_decoder/wp0_rice.c'),
                               '-o', str(stage/'old_rice.o')], check=True)
        binary = stage/'review_decoder_guards'
        subprocess.run(flags + [str(ROOT/'audit/review_final/review_decoder_guards.c'),
                                str(stage/'old_rice.o'), str(ROOT/'source/wp0_rice.c'),
                                '-o', str(binary)], check=True)
        env = os.environ.copy()
        env['ASAN_OPTIONS'] = 'detect_leaks=0:halt_on_error=1'
        env['UBSAN_OPTIONS'] = 'halt_on_error=1'
        run = subprocess.run([str(binary), str(corpus)], env=env,
                             check=True, capture_output=True, text=True)
        result = json.loads(run.stdout)
        assert result == evidence['independent_host_result']
        result['candidate_sha256'] = hashlib.sha256((ROOT/'source/wp0_rice.c').read_bytes()).hexdigest()
        result['corpus_sha256'] = corpus_hash
        result['asan_enabled'] = result['ubsan_enabled'] = True
        result['leak_sanitizer_enabled'] = False
        print(json.dumps(result, indent=2))


if __name__ == '__main__':
    main()
