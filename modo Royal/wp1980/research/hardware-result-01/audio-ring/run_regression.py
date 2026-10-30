#!/usr/bin/env python3
"""Compile the real fast SFX path; keep RED/GREEN evidence in separate files."""
import argparse
import hashlib
import json
from pathlib import Path
import subprocess


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--source', type=Path, required=True)
    parser.add_argument('--label', required=True)
    args = parser.parse_args()
    source = args.source.resolve()
    here = Path(__file__).resolve().parent
    executable = here / ('fast-ring-' + args.label)
    command = ['cc', '-std=c11', '-O2', '-Wall', '-Wextra', '-Werror',
               '-I', str(source.parent),
               '-DWF_SFX_SOURCE=' + json.dumps(str(source)),
               str(here / 'fast_ring_regression.c'), '-o', str(executable)]
    compile_result = subprocess.run(command, capture_output=True, text=True)
    report = {'source': str(source),
              'source_sha256': hashlib.sha256(source.read_bytes()).hexdigest(),
              'scope': 'Production FAST_68000 fill/work/advance, relocated sound RAM; deterministic contiguous decoder replaces assembly only.',
              'physical_hardware_validation': False,
              'compile_command': command,
              'compile_exit': compile_result.returncode,
              'compile_stdout': compile_result.stdout,
              'compile_stderr': compile_result.stderr}
    if compile_result.returncode == 0:
        run = subprocess.run([str(executable)], capture_output=True, text=True)
        report.update(run_command=[str(executable)], run_exit=run.returncode,
                      stdout=run.stdout, stderr=run.stderr)
        print(run.stdout, end='')
        print(run.stderr, end='')
        code = run.returncode
    else:
        print(compile_result.stdout, end='')
        print(compile_result.stderr, end='')
        code = compile_result.returncode
    (here / (args.label + '.json')).write_text(json.dumps(report, indent=2) + '\n')
    raise SystemExit(code)


if __name__ == '__main__':
    main()
