#!/usr/bin/env python3
"""Compile/run the production bounded backup transaction with host BIOS I/O."""
import subprocess
import tempfile
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
with tempfile.TemporaryDirectory(prefix='wf-save-io-') as directory:
    target = Path(directory) / 'save-test'
    subprocess.run(['cc', '-std=c11', '-O2', '-Wall', '-Wextra', '-Werror',
                    '-fsanitize=address,undefined', '-fno-omit-frame-pointer',
                    '-I' + str(ROOT / 'port'), '-I' + str(ROOT / 'core/include'),
                    str(ROOT / 'tests/test_save_adapter.c'), str(ROOT / 'port/wf_jo_save.c'),
                    str(ROOT / 'core/src/wf_r18_save.c'), '-o', str(target)], check=True)
    subprocess.run([str(target)], check=True)
