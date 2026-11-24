#!/usr/bin/env python3
"""Check target code for constant-width coin CCR and small register helpers.

This is a code-generation check, not a Saturn timing benchmark. A generic
right-shift helper in this translation unit indicates that CCR width is still
selected at runtime; all original arithmetic calls use fixed8/16-bit widths.
The coin-selector left shift remains valid and must not be removed by this
check. Runtime/ROM tests establish behavior separately.
"""
import argparse
import hashlib
import json
from pathlib import Path
import re
import subprocess

ROOT = Path(__file__).resolve().parents[1]


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--source', type=Path, default=ROOT / 'core/src/wf_r18_frontend_coin.c')
    parser.add_argument('--compiler', type=Path, default=ROOT / 'sdk/Compiler/LINUX/bin/sh-none-elf-gcc-8.2.0')
    parser.add_argument('--build-dir', type=Path, default=ROOT / 'build/coin-codegen')
    parser.add_argument('--output', type=Path)
    parser.add_argument('--inline-helpers', action='store_true',
                        help='also reject out-of-line local validation and MOVE.B/W helpers')
    args = parser.parse_args()
    args.build_dir.mkdir(parents=True, exist_ok=True)
    assembly = args.build_dir / 'coin.s'
    command = [str(args.compiler), '-m2', '-std=gnu99', '-O2', '-fno-builtin',
               '-fno-peephole2', '-Wall', '-Wextra', '-Werror',
               '-I' + str(ROOT / 'core/include'),
               '-I' + str(ROOT / 'sdk/Compiler/WINDOWS/sh-elf/include'),
               '-S', str(args.source), '-o', str(assembly)]
    subprocess.run(command, check=True)
    generated = assembly.read_text()
    helpers = re.findall(r'^\s*\.long\s+(___\w*sh\w*)\s*$', generated, re.M)
    right = [name for name in helpers if 'lshrsi3' in name]
    shadow = re.findall(r'^\s*\.long\s+(_wf_r17_m68k_move_[bw])\s*$', generated, re.M)
    validation = bool(re.search(r'^_valid:\s*$', generated, re.M))
    failed = bool(right or (args.inline_helpers and (shadow or validation)))
    result = {'status': 'FAIL' if failed else 'PASS',
              'source': str(args.source),
              'source_sha256': hashlib.sha256(args.source.read_bytes()).hexdigest(),
              'assembly': str(assembly),
              'assembly_sha256': hashlib.sha256(assembly.read_bytes()).hexdigest(),
              'command': command, 'shift_helper_table_entries': helpers,
              'generic_ccr_right_shift_entries': len(right),
              'shadow_move_table_entries': shadow,
              'out_of_line_validation': validation,
              'inline_helpers_required': args.inline_helpers,
              'scope': 'Standalone SH-2 generated code with production optimization flags; no execution-time claim.'}
    if args.output:
        args.output.parent.mkdir(parents=True, exist_ok=True)
        args.output.write_text(json.dumps(result, indent=2) + '\n')
    print(f"{result['status']}: coin CCR generic right-shift entries={len(right)}; "
          f"shadow MOVE references={len(shadow)}; out-of-line valid={validation}; generated helpers={helpers}")
    raise SystemExit(failed)


if __name__ == '__main__':
    main()
