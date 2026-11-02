#!/usr/bin/env python3
"""Package existing verified WAV containers for private listening, never for Git.

This performs no audio synthesis, resampling, gain change or acceptance upgrade.
Per-command evidence and the matrix's failed/stable flags remain explicit.
"""
import argparse
import csv
import hashlib
import io
import json
from pathlib import Path
import struct
import wave
import zipfile
import numpy as np


def sha(path):
    with Path(path).open('rb') as stream:
        return hashlib.file_digest(stream, 'sha256').hexdigest()


def wav_info(path):
    raw = path.read_bytes()
    if raw[:4] != b'RIFF' or raw[8:12] != b'WAVE' or struct.unpack_from('<I', raw, 4)[0] + 8 != len(raw):
        raise ValueError('Incomplete or non-RIFF native WAV: ' + str(path))
    with wave.open(str(path), 'rb') as stream:
        if stream.getcomptype() != 'NONE' or stream.getsampwidth() != 2 or stream.getframerate() != 44100:
            raise ValueError('Expected uncompressed PCM16 at 44100 Hz: ' + str(path))
        frames, channels = stream.getnframes(), stream.getnchannels()
        if len(stream.readframes(frames)) != frames * channels * 2:
            raise ValueError('Truncated WAV samples: ' + str(path))
    return {'sample_rate': 44100, 'bits': 16, 'channels': channels,
            'frames': frames, 'seconds': frames / 44100, 'sha256': sha(path), 'bytes': len(raw)}


def digital_support(path):
    """Exact nonzero PCM support, without a fitted threshold or gain change."""
    with wave.open(str(path), 'rb') as stream:
        channels, frames = stream.getnchannels(), stream.getnframes()
        values = np.frombuffer(stream.readframes(frames), dtype='<i2').reshape(-1, channels)
    indices = np.flatnonzero(np.any(values != 0, axis=1))
    if not len(indices):
        return {'all_exact_zero': True, 'window_frames': frames,
                'first_nonzero_frame': None, 'last_nonzero_frame_exclusive': None,
                'nonzero_span_seconds': None, 'touches_end_of_window': False}
    first, end = int(indices[0]), int(indices[-1]) + 1
    return {'all_exact_zero': False, 'window_frames': frames,
            'first_nonzero_frame': first, 'last_nonzero_frame_exclusive': end,
            'first_nonzero_seconds': first / 44100,
            'last_nonzero_seconds_exclusive': end / 44100,
            'nonzero_span_seconds': (end - first) / 44100,
            'trailing_exact_silence_seconds': (frames - end) / 44100,
            'touches_end_of_window': end == frames}


def main():
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument('--matrix', type=Path, required=True)
    p.add_argument('--kind', choices=('music', 'sfx'), required=True)
    p.add_argument('--out', type=Path, required=True)
    p.add_argument('--reference-note', default='', help='Explicit additional provenance limit, retained in the ZIP and catalogue manifest')
    p.add_argument('--comparison-matrix', type=Path, help='Optional separately audited new references for exactly the same native captures')
    a = p.parse_args()
    if a.out.exists():
        raise ValueError('Refuse to replace an earlier listening delivery')
    matrix_path = a.matrix.resolve()
    matrix = json.loads(matrix_path.read_text())
    pairs = matrix_path.parent / 'pairs'
    replacement = None
    if a.comparison_matrix:
        replacement = json.loads(a.comparison_matrix.read_text())
        if (not replacement.get('complete') or not replacement.get('compare_source_stable') or
                replacement.get('native_matrix_sha256') != sha(matrix_path) or
                replacement.get('firmware_sha256') != matrix.get('firmware_sha256')):
            raise ValueError('Replacement reference matrix does not identify these exact native captures')
        pairs = a.comparison_matrix.resolve().parent / 'pairs'
    expected = list(range(1, 19)) if a.kind == 'music' else list(range(0x20, 0x82))
    # Long matrices also contain a transition case beginning with command01.
    # It is a separate test, never a replacement for the command01 music case.
    catalogue = [row for row in matrix['commands']
                 if a.kind != 'music' or row.get('kind', 'music') == 'music']
    entries = {}
    for row in catalogue:
        command = int(row['command'], 0) if isinstance(row['command'], str) else row['command']
        if command in entries:
            raise ValueError('Duplicate catalogue command; do not select a later attempt silently')
        entries[command] = row
    if sorted(entries) != expected or not matrix.get('all_pairs_available'):
        raise ValueError('The requested complete command catalogue is not available')
    folder = 'MUSICAS' if a.kind == 'music' else 'EFECTOS'
    root = 'AUDIO2-COMPARATIVAS'
    members, rows = {}, []
    for command in expected:
        label = f'{command:02d}' if a.kind == 'music' else f'{command:02X}'
        comparison_path = pairs / ('COMPARACION-' + label + '.json')
        comparison = json.loads(comparison_path.read_text())
        if int(comparison['command'], 0) != command:
            raise ValueError('Command identity mismatch')
        expected_files = entries[command]['build']['files']
        for identity, filename in [('source_executable', 'AUDIO2.ELF'),
                                   ('source_iso', 'AUDIO2.iso')]:
            if comparison['identities'][identity] != expected_files[filename]['sha256']:
                raise ValueError('Listening comparison belongs to another native build')
        if replacement:
            original_comparison = json.loads((matrix_path.parent / 'pairs' / comparison_path.name).read_text())
            for identity in ('saturn_wav', 'source_capture_wav', 'source_executable', 'source_iso'):
                if comparison['identities'][identity] != original_comparison['identities'][identity]:
                    raise ValueError('Replacement reference changed a native capture identity')
        row = {'command_rom': f'0x{command:02X}', 'file_label': label,
               'reference_scope': comparison['reference_kind'],
               'saturn_scope': comparison['saturn_kind'],
               'alignment': comparison['alignment'], 'processing': comparison['processing'],
               'metrics': comparison['metrics'], 'native_diagnostics': comparison['diagnostics'],
               'identities': comparison['identities'],
               'runtime_gates_passed': entries[command]['native_gates']['passed'],
               'capture_attempts': entries[command].get('capture_attempts'),
               'independent_native_audit_sha256': entries[command].get('independent_native_audit_sha256'),
               'firmware_sha256': matrix['firmware_sha256'],
               'owner_listening_approval': 'PENDING', 'hardware_status': 'HARDWARE PENDING',
               'files': {}}
        if comparison['processing'].get('normalization') or comparison['processing'].get('gain') != 1:
            raise ValueError('Do not silently package level-adjusted comparisons')
        for prefix, key in [('ARCADE-REFERENCIA', 'reference_wav'), ('SATURN-AUDIO2', 'saturn_wav')]:
            source = comparison_path.parent / f'{prefix}-{label}.wav'
            info = wav_info(source)
            if info['sha256'] != comparison['identities'][key]:
                raise ValueError('Audio bytes no longer match the finalized comparison manifest')
            member = f'{root}/{folder}/{source.name}'
            members[member] = source
            row['files'][prefix] = dict(info, path=member)
        if row['files']['ARCADE-REFERENCIA']['frames'] != row['files']['SATURN-AUDIO2']['frames']:
            raise ValueError('Pair has different frame counts')
        support = {prefix: digital_support(comparison_path.parent / f'{prefix}-{label}.wav')
                   for prefix in ('ARCADE-REFERENCIA', 'SATURN-AUDIO2')}
        left, right = support['ARCADE-REFERENCIA'], support['SATURN-AUDIO2']
        for field in ('first_nonzero_frame', 'last_nonzero_frame_exclusive'):
            support[field + '_saturn_minus_reference_ms'] = (
                (right[field] - left[field]) * 1000 / 44100
                if right[field] is not None and left[field] is not None else None)
        row['additional_activity_measurement'] = dict(
            method='Exact nonzero raw PCM; no threshold, alignment fit or signal change',
            limitation='Window bounds and PCM quantization apply; not a composition duration or a calibrated DAC latency',
            numpy_version=np.__version__, **support)
        if a.kind == 'music' and row['files']['SATURN-AUDIO2']['frames'] < 30 * 44100:
            raise ValueError('Music listening excerpt is shorter than 30 seconds')
        members[f'{root}/{folder}/{comparison_path.name}'] = comparison_path
        rows.append(row)
    manifest = {
        'schema': 1, 'purpose': 'Private comparative listening; protected audio, do not commit to GitHub',
        'kind': a.kind, 'commands': rows, 'source_matrix_sha256': sha(matrix_path),
        'replacement_reference_matrix_sha256': sha(a.comparison_matrix) if replacement else None,
        'reference_report_sha256': (replacement or matrix).get('reference_report_sha256'),
        'source_stable': matrix.get('source_stable', False),
        'matrix_runtime_gates_passed': matrix.get('all_runtime_gates_passed', False),
        'all_individual_runtime_gates_passed': all(row['runtime_gates_passed'] for row in rows),
        'capture_attempt_policy': matrix.get('capture_attempt_policy'),
        'original_campaign_runtime_gates_passed': matrix.get('original_campaign_runtime_gates_passed'),
        'emulator': matrix['emulator'], 'emulator_sha256': matrix.get('emulator_sha256'),
        'production_ready': False, 'owner_listening_approval': 'PENDING',
        'hardware_status': 'HARDWARE PENDING',
        'additional_reference_provenance_limit': a.reference_note or None,
        'classification': ('Music file numbers are decimal 01..18; command_rom contains original hexadecimal01..12.'
                           if a.kind == 'music' else
                           'All98 effects/voice commands are included under their original hexadecimal IDs. Semantic voice/character labels are unverified; no invented classification.'),
    }
    description = '''# Audio 2 — comparativas privadas

WAV PCM16 sin pérdidas a44.100Hz. Escuchar cada ARCADE-REFERENCIA junto a su
SATURN-AUDIO2 del mismo número. Los niveles se conservan: sin normalización,
cambio de ganancia, remuestreo ni estiramiento temporal de los WAV entregados.
La referencia es un render de software de datos originales, con su alcance
exacto en MANIFIESTO.json. Saturn procede de Mednafen1.32.1 real.

Esta entrega permite escuchar el candidato. No certifica fidelidad absoluta,
estabilidad de combate, bucles completos ni aprobación del propietario.
HARDWARE PENDING. Consultar los fallos y la estabilidad de fuentes del manifiesto.
No publicar estos WAV ni otros recursos derivados de la ROM en GitHub.
'''
    if a.reference_note:
        description += '\n## Limitación adicional de la referencia\n\n' + a.reference_note + '\n'
    extra = {
        f'{root}/LEEME.md': description.encode(),
        f'{root}/MANIFIESTO.json': (json.dumps(manifest, indent=2, ensure_ascii=False) + '\n').encode(),
    }
    if a.kind == 'sfx':
        extra[f'{root}/VOCES/INDICE.md'] = ('''# Voces incluidas en el catálogo exhaustivo

Los98 comandos20–81, incluidas sus voces, están en../EFECTOS/. Se conservan
los IDs originales porque aún no se ha demostrado qué personaje/productor
corresponde a cada voz. Este índice no inventa etiquetas ni copia audio bajo
una atribución incierta. Los comandos silenciosos conservan su silencio.
''').encode()
    table = io.StringIO()
    writer = csv.writer(table)
    writer.writerow(['comando_ROM', 'archivo', 'segundos', 'SHA256', 'prueba_individual', 'aprobacion_auditiva'])
    for row in rows:
        for data in row['files'].values():
            writer.writerow([row['command_rom'], data['path'], f"{data['seconds']:.9f}", data['sha256'], row['runtime_gates_passed'], 'PENDING'])
    extra[f'{root}/CATALOGO.csv'] = table.getvalue().encode('utf-8')
    sums = [f'{sha(path)}  {name}\n' for name, path in sorted(members.items())]
    sums += [f'{hashlib.sha256(data).hexdigest()}  {name}\n' for name, data in sorted(extra.items())]
    extra[f'{root}/SHA256SUMS'] = ''.join(sums).encode()
    a.out.parent.mkdir(parents=True, exist_ok=True)
    with zipfile.ZipFile(a.out, 'x', compression=zipfile.ZIP_DEFLATED, compresslevel=6) as archive:
        for name, source in sorted(members.items()):
            archive.writestr(zipfile.ZipInfo(name, (2026, 11, 1, 0, 0, 0)), source.read_bytes(), compress_type=zipfile.ZIP_DEFLATED)
        for name, data in sorted(extra.items()):
            archive.writestr(zipfile.ZipInfo(name, (2026, 11, 1, 0, 0, 0)), data, compress_type=zipfile.ZIP_DEFLATED)
    with zipfile.ZipFile(a.out) as archive:
        if archive.testzip() is not None:
            raise ValueError('ZIP integrity failure')
        for line in sums:
            checksum, name = line.rstrip('\n').split('  ', 1)
            if hashlib.sha256(archive.read(name)).hexdigest() != checksum:
                raise ValueError('ZIP member hash differs after packaging')
    receipt = {'path': str(a.out.resolve()), 'sha256': sha(a.out), 'bytes': a.out.stat().st_size,
               'pairs': len(rows), 'wav_files': len(rows) * 2,
               'all_members_verified_after_packaging': True, 'production_ready': False}
    a.out.with_suffix('.json').write_text(json.dumps(receipt, indent=2) + '\n')
    print(json.dumps(receipt, indent=2))


if __name__ == '__main__':
    main()
