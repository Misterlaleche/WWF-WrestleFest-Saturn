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


def main():
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument('--matrix', type=Path, required=True)
    p.add_argument('--kind', choices=('music', 'sfx'), required=True)
    p.add_argument('--out', type=Path, required=True)
    p.add_argument('--reference-note', default='', help='Explicit additional provenance limit, retained in the ZIP and catalogue manifest')
    a = p.parse_args()
    if a.out.exists():
        raise ValueError('Refuse to replace an earlier listening delivery')
    matrix_path = a.matrix.resolve()
    matrix = json.loads(matrix_path.read_text())
    expected = list(range(1, 19)) if a.kind == 'music' else list(range(0x20, 0x82))
    entries = {int(row['command'], 0) if isinstance(row['command'], str) else row['command']: row
               for row in matrix['commands']}
    if sorted(entries) != expected or not matrix.get('all_pairs_available'):
        raise ValueError('The requested complete command catalogue is not available')
    folder = 'MUSICAS' if a.kind == 'music' else 'EFECTOS'
    root = 'AUDIO2-COMPARATIVAS'
    members, rows = {}, []
    for command in expected:
        label = f'{command:02d}' if a.kind == 'music' else f'{command:02X}'
        comparison_path = matrix_path.parent / 'pairs' / ('COMPARACION-' + label + '.json')
        comparison = json.loads(comparison_path.read_text())
        if int(comparison['command'], 0) != command:
            raise ValueError('Command identity mismatch')
        row = {'command_rom': f'0x{command:02X}', 'file_label': label,
               'reference_scope': comparison['reference_kind'],
               'saturn_scope': comparison['saturn_kind'],
               'alignment': comparison['alignment'], 'processing': comparison['processing'],
               'metrics': comparison['metrics'], 'native_diagnostics': comparison['diagnostics'],
               'identities': comparison['identities'],
               'runtime_gates_passed': entries[command]['native_gates']['passed'],
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
        if a.kind == 'music' and row['files']['SATURN-AUDIO2']['frames'] < 30 * 44100:
            raise ValueError('Music listening excerpt is shorter than 30 seconds')
        members[f'{root}/{folder}/{comparison_path.name}'] = comparison_path
        rows.append(row)
    manifest = {
        'schema': 1, 'purpose': 'Private comparative listening; protected audio, do not commit to GitHub',
        'kind': a.kind, 'commands': rows, 'source_matrix_sha256': sha(matrix_path),
        'source_stable': matrix.get('source_stable', False),
        'matrix_runtime_gates_passed': matrix.get('all_runtime_gates_passed', False),
        'all_individual_runtime_gates_passed': all(row['runtime_gates_passed'] for row in rows),
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
