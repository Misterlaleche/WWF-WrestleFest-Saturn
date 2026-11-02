#!/usr/bin/env python3
"""Package reviewed complete WAV pairs privately, without changing any samples.

Six ZIPs contain three original commands each. A seventh holds the original
transition replay and optional active-fade diagnostics. Only DELIVERY.json
and source tools belong in Git; ZIPs and their protected WAVs never do.
"""
import argparse
import hashlib
import io
import json
from pathlib import Path
import shutil
import wave
import zipfile

VERSION = '1.0.1'
PACKAGE = Path(__file__).resolve().parents[1]
ROOT = 'AUDIO2-COMPARATIVAS'


def digest(path):
    with Path(path).open('rb') as stream:
        return hashlib.file_digest(stream, 'sha256').hexdigest()


def encoded(value):
    return (json.dumps(value, indent=2, ensure_ascii=False, allow_nan=False) + '\n').encode()


def load_manifest(directory, full):
    path = directory / 'public-manifest.json'
    report = json.loads(path.read_text())
    if (report.get('complete_capture_export') is not True
            or report.get('source_stable') is not True
            or report.get('normalization') is not False or report.get('gain') != 1
            or report.get('resampling') is not False or report.get('time_stretch') is not False
            or report.get('waveform_fitted_alignment') is not False):
        raise ValueError('Need a completed, immutable export with unaltered sample values')
    if report.get('source_sha256', {}).get('tools/prepare_music_audio_pairs.py') != digest(
            PACKAGE / 'tools/prepare_music_audio_pairs.py'):
        raise ValueError('Export must identify the currently reviewed pair exporter')
    pairs = report.get('pairs', [])
    if not pairs or any(p.get('runtime_status') != 'PASS'
                        or p.get('complete_recorded_window') is not True
                        or p.get('native_audit_revalidated') is not True
                        or type(p.get('seconds')) is not int or p['seconds'] <= 0
                        or type(p.get('frames')) is not int or p['frames'] != p['seconds'] * 44100
                        for p in pairs):
        raise ValueError('This delivery packages only cases with complete passing runtime evidence')
    if full:
        if (report.get('complete_18_music_collection') is not True
                or report.get('diagnostic_only') is not False
                or report.get('native_runtime_passed') is not True
                or len(pairs) != 19
                or sorted(p['command'] for p in pairs if p['kind'] == 'music') != list(range(1, 19))
                or sum(p['kind'] == 'transitions' for p in pairs) != 1
                or any(p['seconds'] != 608 or p.get('requested_events') != [
                    {'sample': 0, 'word': p['command']}, {'sample': 600 * 44100, 'word': 31}]
                    for p in pairs if p['kind'] == 'music')):
            raise ValueError('Need all 18 complete long pairs and the original transition case')
        transition = next(p for p in pairs if p['kind'] == 'transitions')
        if (transition['seconds'] != 32 or transition.get('command') != 1
                or transition.get('requested_events') != [{'sample': i * 3 * 44100, 'word': word}
                    for i, word in enumerate((1, 6, 18, 31, 3, 31, 2, 1, 31))]):
            raise ValueError('Need the complete declared original nine-command transition')
    elif (report.get('diagnostic_only') is not True or len(pairs) != 1
          or pairs[0].get('kind') != 'music' or pairs[0].get('command') not in (5, 8, 9)
          or pairs[0].get('seconds') != 10 or pairs[0].get('requested_events') != [
              {'sample': 0, 'word': pairs[0].get('command')}, {'sample': 44100, 'word': 31}]):
        raise ValueError('Extra manifests must be one active-fade diagnostic for 05, 08 or 09')
    return path, report


def wav_member(directory, pair, key, member):
    info = pair[key]
    name = info['file']
    if Path(name).name != name or name in ('', '.', '..'):
        raise ValueError('A WAV name must be a single relative filename')
    if (type(pair.get('seconds')) is not int or pair['seconds'] <= 0
            or type(pair.get('frames')) is not int or pair['frames'] != pair['seconds'] * 44100):
        raise ValueError('Pair duration must equal the complete sample-frame count')
    source = directory / 'wav' / name
    if source.is_symlink() or not source.is_file() or source.resolve().parent != (directory / 'wav').resolve():
        raise ValueError('WAV must be an ordinary file inside its export directory')
    if digest(source) != info['sha256'] or source.stat().st_size != info['bytes']:
        raise ValueError('WAV no longer matches its reviewed manifest')
    with wave.open(str(source)) as stream:
        actual = (stream.getnframes(), stream.getnchannels(), stream.getsampwidth(),
                  stream.getframerate(), stream.getcomptype())
        expected = (pair['frames'], 2 if key == 'native_wav' else 1, 2, 44100, 'NONE')
        if (actual != expected or info.get('source_pcm_bytes_match') is not True
                or any(info.get(k) != value for k, value in {
                    'frames': pair['frames'], 'seconds': pair['seconds'], 'channels': expected[1],
                    'bits': 16, 'sample_rate': 44100}.items())):
            raise ValueError('WAV format, complete duration or byte provenance mismatch')
        pcm = hashlib.sha256()
        count = 0
        while chunk := stream.readframes(65536):
            pcm.update(chunk)
            count += len(chunk)
        if count != pair['frames'] * expected[1] * 2 or pcm.hexdigest() != info.get('pcm_sha256'):
            raise ValueError('Actual PCM digest or byte count differs from its manifest')
    return {'source': source, 'member': member, 'sha256': info['sha256'], 'bytes': info['bytes']}


def add_pair(members, directory, pair, folder, suffix):
    for key, prefix in (('original_host_wav', 'ARCADE-REFERENCIA'), ('native_wav', 'SATURN-AUDIO2')):
        members.append(wav_member(directory, pair, key, f'{ROOT}/{folder}/{prefix}-{suffix}.wav'))


def write_zip(path, members, metadata):
    """Deterministic lossless packaging, with independent decompressed hashes."""
    names = [m['member'] for m in members] + list(metadata)
    if len(names) != len(set(names)):
        raise ValueError('Duplicate archive member')
    expected = {m['member']: m['sha256'] for m in members}
    expected.update({name: hashlib.sha256(data).hexdigest() for name, data in metadata.items()})
    sum_name = f'{ROOT}/SHA256SUMS'
    sums = ''.join(f'{value}  {name}\n' for name, value in sorted(expected.items())).encode()
    metadata = dict(metadata, **{sum_name: sums})
    expected[sum_name] = hashlib.sha256(sums).hexdigest()
    sources = {m['member']: m['source'] for m in members}
    with zipfile.ZipFile(path, 'x', compression=zipfile.ZIP_DEFLATED, compresslevel=6) as archive:
        for name in sorted(expected):
            entry = zipfile.ZipInfo(name, date_time=(1980, 1, 1, 0, 0, 0))
            entry.compress_type = zipfile.ZIP_DEFLATED
            entry._compresslevel = 6
            entry.create_system = 3
            entry.external_attr = 0o100644 << 16
            stream = sources[name].open('rb') if name in sources else io.BytesIO(metadata[name])
            check = hashlib.sha256()
            with stream, archive.open(entry, 'w', force_zip64=True) as sink:
                while chunk := stream.read(1 << 20):
                    check.update(chunk)
                    sink.write(chunk)
            if check.hexdigest() != expected[name]:
                raise ValueError('Input changed while packaging: ' + name)
    with zipfile.ZipFile(path) as archive:
        if set(archive.namelist()) != set(expected):
            raise ValueError('Archive membership mismatch')
        for name, identity in expected.items():
            with archive.open(name) as stream:
                if hashlib.file_digest(stream, 'sha256').hexdigest() != identity:
                    raise ValueError('Decompressed archive differs from source: ' + name)
    return {'file': path.name, 'bytes': path.stat().st_size, 'sha256': digest(path),
            'wav_files': len(members), 'decompressed_members_verified': len(expected),
            'files': [{k: v for k, v in m.items() if k != 'source'} for m in members]}


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--pairs', type=Path, required=True)
    parser.add_argument('--active-fade-pairs', type=Path, action='append', default=[])
    parser.add_argument('--analysis-report', type=Path)
    parser.add_argument('--out', type=Path, required=True)
    args = parser.parse_args()
    directory, out = args.pairs.resolve(), args.out.resolve()
    source_dirs = [directory] + [p.resolve() for p in args.active_fade_pairs]
    if (out.exists() or not out.parent.is_dir() or out == PACKAGE or PACKAGE in out.parents
            or any(out == p or p in out.parents for p in source_dirs)):
        raise ValueError('Use a fresh private output outside all source/input directories')
    path, report = load_manifest(directory, True)
    input_hashes = {path: digest(path), Path(__file__).resolve(): digest(__file__)}
    groups = []
    for first in range(1, 19, 3):
        members = []
        for command in range(first, first + 3):
            pair = next(p for p in report['pairs'] if p['kind'] == 'music' and p['command'] == command)
            add_pair(members, directory, pair, 'MUSICAS', f'{command:02d}')
        groups.append((f'AUDIO2-MUSICAS-COMPLETAS-{first:02d}-{first+2:02d}.zip', members))
    transitions = next(p for p in report['pairs'] if p['kind'] == 'transitions')
    extras = []
    add_pair(extras, directory, transitions, 'MEZCLAS', 'TRANSICIONES')
    extra_metadata, active_commands = {}, set()
    for extra_dir in source_dirs[1:]:
        extra_path, extra = load_manifest(extra_dir, False)
        pair = extra['pairs'][0]
        command = pair['command']
        if command in active_commands or extra['firmware_sha256'] != report['firmware_sha256']:
            raise ValueError('Duplicate active fade or incompatible firmware')
        for source in ('src/reference_driver/z80-native.c', 'src/wf_music_seq.c', 'src/wf_music_module.c'):
            if extra['matrix_source_sha256'][source] != report['matrix_source_sha256'][source]:
                raise ValueError('Extra fade uses a different runtime')
        active_commands.add(command)
        add_pair(extras, extra_dir, pair, 'MUSICAS/FADES', f'{command:02d}-FADE-ACTIVO')
        extra_metadata[f'{ROOT}/MANIFIESTO-FADE-{command:02d}.json'] = extra_path.read_bytes()
        input_hashes[extra_path] = digest(extra_path)
    groups.append(('AUDIO2-TRANSICIONES-Y-FADES.zip', extras))
    catalogue = {'schema_version': 1, 'tool_version': VERSION, 'pair_manifest_sha256': digest(path),
                 'music_commands': 18, 'music_wav_files': 36, 'transition_pairs': 1,
                 'active_fade_commands': sorted(active_commands), 'archives': [],
                 'sample_rate': 44100, 'bits': 16, 'normalization': False,
                 'fidelity_approved': False, 'owner_listening_approval': 'PENDING',
                 'hardware_status': 'HARDWARE PENDING', 'upload_status': 'NOT_UPLOADED',
                 'source_tool_sha256': digest(__file__)}
    readme = '''WWF WRESTLEFEST — COMPARATIVAS COMPLETAS DE AUDIO 2

Los seis ZIP musicales contienen los18 comandos originales (01..12 hexadecimal).
El número del nombre WAV es decimal01..18; el manifiesto indica el comando ROM.
Cada pareja conserva la ventana larga completa:600s de ejecución y8s tras
solicitar1F. 05,08 y09 terminan naturalmente mucho antes; su silencio posterior
forma parte del registro de prueba. No se afirma que duren diez minutos.

ARCADE-REFERENCIA: modelo del controlador Z80 original, YM2151 y OKIM6295 con
datos originales. Es referencia de software; no grabación de una PCB física.
SATURN-AUDIO2: extracción byte por byte de soundrecord de Mednafen1.32.1
ejecutando el módulo real en Saturn emulada. No es un renderer de PC presentado
como SCSP. PCM16 sin pérdidas,44.100Hz; referencia mono y Saturn estéreo.

Las parejas parten de la marca observada de admisión del comando. Saturn
conserva su lead4096 muestras (92,88ms), documentado, y su nivel original.
No se normaliza, estira, ajusta por correlación ni inventa un fade. La alineación
DAC física no está calibrada. Las transiciones y fades activos tienen sus
comandos y tiempos efectivos en los manifiestos; los WAV no se han alterado.

Los bucles de partitura y finales se verifican por separado de un período PCM.
0B/0C/11 contienen ciclos de canal de longitudes diferentes. No se presenta
una repetición corta de la mezcla ni fidelidad acústica absoluta como probada.

ESTADO: capturas para revisión del propietario. La aprobación auditiva y la
Saturn NTSC-U/MODE siguen pendientes. El banco OKI activo nativo y los dos
avisos del ensayo mixto aislado siguen bloqueando la aceptación global.
Esta entrega no integra ROM Graphics Final; esa integración corresponde a Codex.

Ver MANIFIESTO.json para hashes, versiones, duración, admisiones y límites.
SHA256SUMS permite verificar cada archivo tras descomprimir. Estos WAV son
privados y no deben publicarse en GitHub.
'''.encode()
    common = {f'{ROOT}/LEEME.txt': readme, f'{ROOT}/MANIFIESTO.json': path.read_bytes()}
    if args.analysis_report:
        analysis_path = args.analysis_report.resolve()
        analysis = json.loads(analysis_path.read_text())
        if (analysis.get('pair_manifest_sha256') != digest(path)
                or analysis.get('complete') is not True or analysis.get('source_stable') is not True):
            raise ValueError('Analysis does not identify the completed immutable pair collection')
        common[f'{ROOT}/METRICAS.json'] = analysis_path.read_bytes()
        input_hashes[analysis_path] = digest(analysis_path)
    required = sum(m['bytes'] for _, members in groups for m in members)
    if shutil.disk_usage(out.parent).free < required + 32 * 1024**2:
        raise ValueError('Insufficient free disk for worst-case lossless archives')
    out.mkdir()
    for name, members in groups:
        metadata = {**common, **(extra_metadata if name == groups[-1][0] else {})}
        result = write_zip(out / name, members, metadata)
        catalogue['archives'].append(result)
        print(json.dumps({k: result[k] for k in ('file', 'bytes', 'sha256', 'wav_files')}), flush=True)
    if any(digest(p) != identity for p, identity in input_hashes.items()):
        raise ValueError('Source or manifest changed during packaging')
    catalogue['source_stable'] = True
    catalogue['complete'] = True
    (out / 'DELIVERY.json').write_bytes(encoded(catalogue))
    print(json.dumps({'complete': True, 'archives': len(groups),
                      'wav_files': sum(len(m) for _, m in groups),
                      'delivery_sha256': digest(out / 'DELIVERY.json')}))


if __name__ == '__main__':
    main()
