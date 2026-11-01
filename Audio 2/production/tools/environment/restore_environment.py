#!/usr/bin/env python3
"""Create an isolated Audio 2 tool environment from pinned public inputs.

The user must supply the exact SH-2 SDK archive. No BIOS or game ROM is
downloaded or copied. This script never installs system packages.
"""
import argparse
import hashlib
import json
import os
from pathlib import Path
import shutil
import stat
import subprocess
import tarfile
import zipfile

PACKAGE = Path(__file__).resolve().parent
SDK_SHA256 = '8a30d927a869c22f711e0fc1f37fb22106da7561e4d54968077cff77bb90487d'


def digest(path):
    with Path(path).open('rb') as source:
        return hashlib.file_digest(source, 'sha256').hexdigest()


def verify(path, expected):
    actual = digest(path)
    if actual != expected:
        raise ValueError(f'SHA-256 mismatch for {path}: {actual}')


def obtain(out, cache, offline, filename, url, sha256):
    target = out / 'downloads' / filename
    if not target.exists() and cache and (cache / filename).is_file():
        shutil.copy2(cache / filename, target)
    if not target.exists():
        if offline:
            raise FileNotFoundError(f'Missing offline input: {filename}')
        temporary = target.with_name(target.name + '.part')
        subprocess.run(['curl', '--fail', '--location', '--retry', '2',
                        '--max-time', '120', '--output', str(temporary), url], check=True)
        verify(temporary, sha256)
        temporary.replace(target)
    verify(target, sha256)
    return target


def unpack_sdk(archive_path, out):
    with zipfile.ZipFile(archive_path) as archive:
        for item in archive.infolist():
            target = out / item.filename
            if not target.resolve().is_relative_to(out):
                raise ValueError('SDK archive path escapes output directory')
            mode = item.external_attr >> 16
            if item.is_dir():
                target.mkdir(parents=True, exist_ok=True)
                continue
            target.parent.mkdir(parents=True, exist_ok=True)
            if stat.S_ISLNK(mode):
                link = archive.read(item).decode()
                if not (target.parent / link).resolve().is_relative_to(out):
                    raise ValueError('SDK symlink escapes output directory')
                if target.exists() or target.is_symlink():
                    target.unlink()
                target.symlink_to(link)
            else:
                target.write_bytes(archive.read(item))
                target.chmod(mode & 0o777 or 0o644)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--out', required=True, type=Path)
    parser.add_argument('--sdk-archive', required=True, type=Path)
    parser.add_argument('--cache', type=Path, help='Optional directory containing cached .deb/source downloads')
    parser.add_argument('--offline', action='store_true', help='Refuse all network access')
    parser.add_argument('--build', action='store_true', help='Also build unmodified standalone Mednafen 1.32.1')
    args = parser.parse_args()
    out = args.out.resolve()
    if any(character.isspace() for character in str(out)):
        raise ValueError('The Yaul SDK requires an output path without whitespace')
    archive_path = args.sdk_archive.resolve()
    cache = args.cache.resolve() if args.cache else None
    if out == PACKAGE or PACKAGE.is_relative_to(out):
        raise ValueError('--out must be a separate environment directory')
    for program in ('curl', 'dpkg-deb', 'gcc', 'g++', 'make', 'nm'):
        if shutil.which(program) is None:
            raise RuntimeError(f'Missing host prerequisite: {program}')
    verify(archive_path, SDK_SHA256)
    for name in ('downloads', 'host', 'bin', 'src', 'logs', 'verification', 'mednafen-home'):
        (out / name).mkdir(parents=True, exist_ok=True)
    shutil.copytree(PACKAGE / 'templates', out, dirs_exist_ok=True)
    shutil.copytree(PACKAGE / 'src', out / 'src', dirs_exist_ok=True)
    shutil.copytree(PACKAGE / 'licenses', out / 'licenses', dirs_exist_ok=True)
    shutil.copytree(PACKAGE / 'manifests', out / 'manifests', dirs_exist_ok=True)
    unpack_sdk(archive_path, out)
    packages = json.loads((PACKAGE / 'manifests/packages.json').read_text())
    for package in packages:
        path = obtain(out, cache, args.offline, Path(package['Filename']).name,
                      'https://archive.ubuntu.com/ubuntu/' + package['Filename'], package['SHA256'])
        subprocess.run(['dpkg-deb', '-x', str(path), str(out / 'host')], check=True)
    sources = json.loads((PACKAGE / 'manifests/sources.json').read_text())
    for source in sources:
        path = obtain(out, cache, args.offline, source['filename'], source['url'], source['sha256'])
        if source['filename'] == 'mednafen-1.32.1.tar.xz':
            with tarfile.open(path) as archive:
                archive.extractall(out / 'src/upstream', filter='data')
        elif source['filename'] == 'gcc-13.3.0-lb1sf68.S':
            shutil.copy2(path, out / 'src/gcc-lb1sf68.S')
    subprocess.run(['gcc', '-shared', '-fPIC', '-O2', '-Wall', '-Wextra', '-Werror',
                    str(out / 'src/xvfb-relocate.c'), '-o',
                    str(out / 'bin/xvfb-relocate.so'), '-ldl'], check=True)
    for name in ('gcc', 'as', 'ld', 'ar', 'gcc-ar', 'gcc-nm', 'gcc-ranlib', 'nm',
                 'objcopy', 'objdump', 'readelf', 'ranlib', 'strip'):
        program = 'm68k-linux-gnu-' + name
        if not (out / 'host/usr/bin' / program).exists():
            continue
        wrapper = out / 'sh2eb-elf/bin' / program
        wrapper.write_text(
            '#!/usr/bin/env bash\nset -euo pipefail\n'
            'WF_M68K_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"\n'
            'export LD_LIBRARY_PATH="$WF_M68K_ROOT/host/usr/lib/x86_64-linux-gnu${LD_LIBRARY_PATH:+:$LD_LIBRARY_PATH}"\n'
            'exec "$WF_M68K_ROOT/host/usr/bin/' + program + '" "$@"\n')
        wrapper.chmod(0o755)
    environment = dict(os.environ)
    environment['PATH'] = str(out / 'host/usr/bin') + ':' + environment.get('PATH', '')
    environment['LD_LIBRARY_PATH'] = str(out / 'host/usr/lib/x86_64-linux-gnu') + ':' + str(out / 'host/lib/x86_64-linux-gnu')
    versions = {}
    for name, command in {
        'sh2_gcc': [str(out / 'sh2eb-elf/bin/sh2eb-elf-gcc'), '--version'],
        'm68k_gcc': [str(out / 'host/usr/bin/m68k-linux-gnu-gcc'), '--version'],
        'xorrisofs': [str(out / 'host/usr/bin/xorrisofs'), '--version'],
    }.items():
        result = subprocess.run(command, env=environment, text=True, capture_output=True, check=True)
        (out / 'logs' / (name + '-version.log')).write_text(result.stdout + result.stderr)
        versions[name] = result.stdout.splitlines()[0] if result.stdout else result.stderr.splitlines()[0]
    probe = out / 'verification/compiler-probe.c'
    probe.write_text('unsigned probe(unsigned a,unsigned b){return a*3u+b;}\n')
    for name, compiler, flags in (
        ('sh2', out / 'sh2eb-elf/bin/sh2eb-elf-gcc', ['-m2', '-mb']),
        ('m68k', out / 'host/usr/bin/m68k-linux-gnu-gcc', ['-m68000', '-msoft-float', '-fno-pic', '-fno-pie'])):
        subprocess.run([str(compiler), *flags, '-ffreestanding', '-O2', '-Wall', '-Wextra', '-Werror',
                        '-c', str(probe), '-o', str(out / 'verification' / (name + '-probe.o'))],
                       env=environment, check=True)
    report = {'sdk_archive_sha256': SDK_SHA256, 'public_packages_verified': len(packages),
              'public_sources': sources, 'versions': versions,
              'cross_compilation_probes': 'passed', 'mednafen_built': False,
              'bios_and_rom_downloaded': False}
    if args.build:
        subprocess.run(['bash', str(out / 'build-mednafen.sh')], check=True)
        report['mednafen_built'] = True
        report['mednafen_sha256'] = digest(out / 'mednafen/bin/mednafen')
    (out / 'logs/restore-report.json').write_text(json.dumps(report, indent=2) + '\n')
    print(json.dumps(report, indent=2))
    if not args.build:
        print('Next: bash', out / 'build-mednafen.sh')


if __name__ == '__main__':
    main()
