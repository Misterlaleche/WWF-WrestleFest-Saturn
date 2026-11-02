#!/usr/bin/env python3
"""Stream an explicitly selected PRIVATE validation archive, then audit CRC/SHA.

No directory is copied or searched implicitly. A local JSON plan lists every
source file, archive member, kind and expected SHA256. Original ROM/BIOS files,
audio/video, object files and source code are forbidden. Executables and disc
images contain protected data and must never be committed or publicly uploaded.
"""
import argparse
import gzip
import hashlib
import json
from pathlib import Path, PurePosixPath
import re
import shutil
import sys
import zipfile
import zlib

PACKAGE = Path(__file__).resolve().parents[1]
ARCHIVE = 'AUDIO2-VALIDACION-PRIVADA.zip'
RESERVED = {'MANIFEST.json', 'SHA256SUMS', 'README-VALIDACION.txt'}
TEXT_SUFFIXES = {'.json', '.map', '.log', '.txt', '.cfg', '.csv', '.su', '.ld', '.specs', '.md'}
FORBIDDEN_SUFFIXES = {'.rom', '.pak', '.dat', '.wav', '.mov', '.mp3', '.mp4', '.flac',
                      '.pcm', '.o', '.a', '.c', '.h', '.s', '.inc', '.py', '.zip', '.7z'}
RAM_BYTES = {'SCSP-RAM-saturn-order.bin': 524288,
             'SCSP-RAM-state-order.bin': 524288,
             'WorkRAMH-saturn-order.bin': 1048576,
             'WorkRAMH-host-order.bin': 1048576,
             'WorkRAML-saturn-order.bin': 1048576,
             'WorkRAML-host-order.bin': 1048576}
LAYOUT_BYTES = {'game-layout.bin': 28, 'sfx-layout.bin': 44}
CHUNK = 1024 * 1024


def digest_stream(stream):
    digest = hashlib.sha256()
    size = 0
    while block := stream.read(CHUNK):
        digest.update(block)
        size += len(block)
    return size, digest.hexdigest()


def digest(path):
    with Path(path).open('rb') as stream:
        return digest_stream(stream)


def safe_member(value):
    path = PurePosixPath(value)
    if not value or '\\' in value or path.is_absolute() or '..' in path.parts or \
            str(path) != value or any(part.startswith('.') for part in path.parts):
        raise ValueError('Unsafe archive member: ' + value)
    return value


def private_output(value):
    original = Path(value)
    out = original.resolve()
    protected = {PACKAGE}
    if PACKAGE.name == 'production' and PACKAGE.parent.name == 'Audio 2':
        protected.add(PACKAGE.parent.parent)
    for parent in PACKAGE.parents:
        if (parent / '.git').exists():
            protected.add(parent)
            break
    if original.is_symlink() or out.exists() or any(out == p or p in out.parents for p in protected):
        raise ValueError('Use a fresh private output outside the source package/repository')
    return out


def check_kind(source, member, kind):
    """A small allowlist prevents an overbroad plan from shipping raw inputs."""
    if source.suffix.lower() in FORBIDDEN_SUFFIXES or \
            PurePosixPath(member).suffix.lower() in FORBIDDEN_SUFFIXES:
        raise ValueError('Forbidden raw input, source, media or object: ' + str(source))
    if kind == 'elf':
        if source.suffix.lower() != '.elf' or PurePosixPath(member).suffix.lower() != '.elf':
            raise ValueError('Forbidden non-ELF executable')
        with source.open('rb') as stream:
            if stream.read(4) != b'\x7fELF':
                raise ValueError('ELF identity missing: ' + str(source))
    elif kind == 'disc':
        if source.suffix.lower() != '.iso' or PurePosixPath(member).suffix.lower() != '.iso':
            raise ValueError('Forbidden non-ISO disc')
        with source.open('rb') as stream:
            stream.seek(16 * 2048 + 1)
            if stream.read(5) != b'CD001':
                raise ValueError('ISO9660 identity missing: ' + str(source))
    elif kind == 'cue':
        if source.suffix.lower() != '.cue' or PurePosixPath(member).suffix.lower() != '.cue':
            raise ValueError('Forbidden non-CUE descriptor')
        text = source.read_text()
        if not re.fullmatch(r'FILE "[^"/\\]+\.iso" BINARY\s+TRACK 01 MODE1/2048\s+INDEX 01 00:00:00\s*', text):
            raise ValueError('Unexpected or external CUE reference: ' + str(source))
    elif kind == 'state':
        if not re.fullmatch(r'.+\.mc[0-9]', source.name, re.I):
            raise ValueError('Forbidden non-native F5 state')
        with gzip.open(source, 'rb') as stream:
            if stream.read(8) != b'MDFNSVST':
                raise ValueError('Native Mednafen state identity missing')
    elif kind in ('ram', 'layout'):
        expected = (RAM_BYTES if kind == 'ram' else LAYOUT_BYTES).get(source.name)
        if expected is None or source.stat().st_size != expected or PurePosixPath(member).name != source.name:
            raise ValueError('Forbidden binary outside named captured RAM/layout contract')
    elif kind == 'text':
        if source.suffix.lower() not in TEXT_SUFFIXES or PurePosixPath(member).suffix.lower() not in TEXT_SUFFIXES:
            raise ValueError('Forbidden non-metadata file: ' + str(source))
        with source.open('rb') as stream:
            prefix = stream.read(CHUNK)
        if b'\0' in prefix:
            raise ValueError('Forbidden binary metadata: ' + str(source))
        prefix.decode('utf-8')
    else:
        raise ValueError('Unknown artifact kind: ' + str(kind))


def preflight(plan):
    if plan.get('schema') != 1 or not plan.get('files'):
        raise ValueError('Require schema1 and explicit file selection')
    rows = []
    names = set(RESERVED)
    for item in plan['files']:
        source = Path(item['source'])
        member = safe_member(item['member'])
        if member in names:
            raise ValueError('Duplicate/reserved member: ' + member)
        names.add(member)
        if source.is_symlink() or not source.is_file():
            raise ValueError('Require a regular existing source: ' + str(source))
        source = source.resolve()
        check_kind(source, member, item['kind'])
        expected = item['sha256']
        size, actual = digest(source)
        if not re.fullmatch(r'[0-9a-f]{64}', expected) or actual != expected:
            raise ValueError('SHA256 mismatch before packaging: ' + str(source))
        rows.append({'source': str(source), 'member': member, 'kind': item['kind'],
                     'bytes': size, 'sha256': actual})
    # A disc descriptor must have its exact ISO beside it in the archive.
    for row in rows:
        if row['kind'] == 'cue':
            target = re.search(r'FILE "([^"]+)"', Path(row['source']).read_text()).group(1)
            target_member = str(PurePosixPath(row['member']).parent / target)
            if target_member not in names:
                raise ValueError('CUE has no packaged ISO: ' + target_member)
    return sorted(rows, key=lambda row: row['member'])


def zip_info(name):
    item = zipfile.ZipInfo(name, (1980, 1, 1, 0, 0, 0))
    item.compress_type = zipfile.ZIP_DEFLATED
    item.external_attr = 0o100600 << 16
    return item


def verify(archive_path):
    """Verify every CRC, exact member set and all recorded SHA256 digests."""
    archive_path = Path(archive_path)
    with zipfile.ZipFile(archive_path) as archive:
        names = archive.namelist()
        if len(names) != len(set(names)):
            raise ValueError('Duplicate archive members')
        for name in names:
            safe_member(name)
        if failed := archive.testzip():
            raise ValueError('CRC mismatch: ' + failed)
        manifest = json.loads(archive.read('MANIFEST.json'))
        if manifest['software_status'] != 'NOT PRODUCTION READY' or manifest['hardware_status'] != 'HARDWARE PENDING':
            raise ValueError('Unsupported acceptance claim')
        expected = {row['member']: row for row in manifest['files'] + manifest['generated_files']}
        if set(names) != set(expected) | {'MANIFEST.json', 'SHA256SUMS'}:
            raise ValueError('Archive member set differs from manifest')
        checksums = {}
        for line in archive.read('SHA256SUMS').decode().splitlines():
            checksum, member = line.split('  ', 1)
            if member in checksums or not re.fullmatch(r'[0-9a-f]{64}', checksum):
                raise ValueError('Invalid SHA256SUMS')
            checksums[member] = checksum
        if set(checksums) != set(names) - {'SHA256SUMS'}:
            raise ValueError('Incomplete SHA256SUMS')
        verified = []
        for name in sorted(names):
            with archive.open(name) as stream:
                size, checksum = digest_stream(stream)
            if name in expected and (checksum != expected[name]['sha256'] or size != expected[name]['bytes']):
                raise ValueError('SHA256 mismatch against MANIFEST: ' + name)
            if name in checksums and checksum != checksums[name]:
                raise ValueError('SHA256 mismatch against SHA256SUMS: ' + name)
            verified.append({'member': name, 'bytes': size, 'sha256': checksum,
                             'crc32': f'{archive.getinfo(name).CRC:08x}'})
    return {'archive_crc_verified': True, 'all_member_hashes_verified': True,
            'artifact_count': len(manifest['files']), 'archive_member_count': len(verified),
            'members': verified}


def package(plan, out, reserve_bytes=2 * 1024**3, archive_name=ARCHIVE):
    out = private_output(out)
    if not re.fullmatch(r'[A-Za-z0-9][A-Za-z0-9._-]*\.zip', archive_name):
        raise ValueError('Archive name must be a plain ASCII .zip filename')
    rows = preflight(plan)
    existing = out.parent
    while not existing.exists():
        existing = existing.parent
    total = sum(row['bytes'] for row in rows)
    if shutil.disk_usage(existing).free < reserve_bytes + total + 1024**2:
        raise ValueError('Insufficient space while preserving requested free-space reserve')
    out.mkdir(parents=True, exist_ok=False)
    readme = '''AUDIO 2 — VALIDACIÓN PRIVADA

ESTADO: NOT PRODUCTION READY / HARDWARE PENDING.
Este archivo conserva artefactos y evidencia de un candidato con fallos conocidos.
Contiene ejecutables, discos y estados que incorporan datos protegidos. No subirlo
a GitHub ni distribuirlo públicamente. No contiene la ROM/BIOS original, las
fuentes completas del repositorio, objetos de compilación ni capturas WAV/MOV.

MANIFEST.json identifica cada archivo, su SHA256, tamaño y ruta de origen. Las
rutas absolutas dentro de los informes históricos se conservan como evidencia;
la columna source -> member permite ubicarlas después de extraer este ZIP.
Los hashes de las fuentes congeladas están en los informes originales. El código
y las herramientas se entregan en la rama audio-2-production-ready del repositorio
Misterlaleche/WWF-WrestleFest-Saturn-Private, sin duplicar ese árbol en este ZIP.

Verificar sin extraer: python3 tools/package_validation_artifacts.py --verify ZIP
Para reproducir una captura, usar exclusivamente Mednafen1.32.1, el CUE/ISO del
caso y el BIOS NTSC-U propio. Conservar juntos CUE e ISO. report.json registra el
comando exacto, configuración, cartucho, velocidad de presentación, versión y
hash del emulador; adaptar únicamente rutas y display. Un estado F5 no sustituye
una reproducción desde arranque. No intercambiar módulos ABI4 entre TAG, Royal
base y Royal con cartucho: cada perfil conserva su propio master, módulo y mapa.

Los WAV y las comparativas auditivas se entregan separadamente. Su ausencia aquí
impide volver a ejecutar un gate que exija esos WAV hasta proporcionar la entrega
de audio correspondiente. Los informes no convierten fallos en aprobados.
La aprobación auditiva del propietario y la Saturn física siguen pendientes.
'''.encode('utf-8')
    generated = [{'member': 'README-VALIDACION.txt', 'bytes': len(readme),
                  'sha256': hashlib.sha256(readme).hexdigest()}]
    manifest = {'schema': 1, 'kind': 'PRIVATE_VALIDATION_ARTIFACTS',
                'software_status': 'NOT PRODUCTION READY', 'hardware_status': 'HARDWARE PENDING',
                'original_rom_or_bios_included': False, 'audio_waveforms_included': False,
                'full_source_repository_included': False,
                'metadata': plan.get('metadata', {}), 'files': rows, 'generated_files': generated,
                'packager_sha256': digest(Path(__file__))[1],
                'archive_format': {'compression': 'ZIP_DEFLATED', 'compression_level': 6,
                    'member_timestamp': '1980-01-01T00:00:00', 'python': sys.version,
                    'zlib_runtime': zlib.ZLIB_RUNTIME_VERSION}}
    manifest_bytes = (json.dumps(manifest, indent=2, ensure_ascii=False) + '\n').encode()
    sums = {row['member']: row['sha256'] for row in rows + generated}
    sums['MANIFEST.json'] = hashlib.sha256(manifest_bytes).hexdigest()
    sums_bytes = ''.join(f'{sums[name]}  {name}\n' for name in sorted(sums)).encode()
    temporary = out / (archive_name + '.incomplete')
    with zipfile.ZipFile(temporary, 'x', compression=zipfile.ZIP_DEFLATED, compresslevel=6) as archive:
        for row in rows:
            checksum = hashlib.sha256()
            size = 0
            with Path(row['source']).open('rb') as source, archive.open(zip_info(row['member']), 'w', force_zip64=True) as target:
                while block := source.read(CHUNK):
                    target.write(block)
                    checksum.update(block)
                    size += len(block)
            if checksum.hexdigest() != row['sha256'] or size != row['bytes']:
                raise ValueError('SHA256 mismatch during packaging: ' + row['source'])
        for name, data in [('README-VALIDACION.txt', readme), ('MANIFEST.json', manifest_bytes), ('SHA256SUMS', sums_bytes)]:
            archive.writestr(zip_info(name), data)
    audit = verify(temporary)
    target = out / archive_name
    temporary.rename(target)
    size, checksum = digest(target)
    audit.update(zip=str(target), zip_bytes=size, zip_sha256=checksum,
                 software_status='NOT PRODUCTION READY', hardware_status='HARDWARE PENDING')
    (out / 'MANIFEST.json').write_bytes(manifest_bytes)
    (out / 'SHA256SUMS').write_bytes(sums_bytes)
    (out / 'ARCHIVE-AUDIT.json').write_text(json.dumps(audit, indent=2, ensure_ascii=False) + '\n')
    return audit


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--plan', type=Path)
    parser.add_argument('--out', type=Path)
    parser.add_argument('--verify', type=Path)
    parser.add_argument('--reserve-gib', type=float, default=2)
    parser.add_argument('--archive-name', default=ARCHIVE)
    args = parser.parse_args()
    if args.verify:
        if args.plan or args.out:
            parser.error('--verify is read-only and cannot accompany --plan/--out')
        result = verify(args.verify)
    else:
        if not args.plan or not args.out or args.reserve_gib < 0:
            parser.error('Provide --plan and fresh private --out; reserve must be nonnegative')
        result = package(json.loads(args.plan.read_text()), args.out,
                         int(args.reserve_gib * 1024**3), args.archive_name)
    print(json.dumps({key: value for key, value in result.items() if key != 'members'}, indent=2))
    return 0


if __name__ == '__main__':
    sys.exit(main())
