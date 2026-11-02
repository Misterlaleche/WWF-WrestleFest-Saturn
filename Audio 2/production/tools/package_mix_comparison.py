#!/usr/bin/env python3
"""Package measured private mix comparisons without changing audio or gates.

Optional supplementary WAVs must have an explicit identity and scope. They are
never silently treated as another arcade/native pair or as accepted firmware.
"""
import argparse
import hashlib
import json
from pathlib import Path, PurePosixPath
import zipfile

from package_comparisons import sha, wav_info


def safe_member(name):
    path = PurePosixPath(name)
    if (path.is_absolute() or '..' in path.parts or
            not name.startswith('AUDIO2-COMPARATIVAS/')):
        raise ValueError('Unsafe or unexpected archive member')
    return name


def main():
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument('--report', type=Path, required=True)
    p.add_argument('--out', type=Path, required=True)
    p.add_argument('--supplement', type=Path,
                   help='Private JSON with files[{path,archive_path,sha256,scope}]')
    a = p.parse_args()
    if a.out.exists():
        raise ValueError('Refuse to replace an earlier delivery')
    report = json.loads(a.report.read_text())
    if (report.get('kind') != 'OBSERVED_NATIVE_ADMISSION_FULL_BOARD_MIX_COMPARISON' or
            not report.get('complete') or not report.get('source_stable')):
        raise ValueError('Need a complete, source-stable original-board mix report')
    if any(report.get(key) is not False for key in
           ('normalization', 'resampling', 'time_stretch', 'waveform_fitted_alignment')):
        raise ValueError('Audio processing must remain explicitly disabled')
    root = 'AUDIO2-COMPARATIVAS'
    members, catalogue = {}, []

    def add(source, member, expected, scope):
        source = Path(source)
        member = safe_member(member)
        if member in members:
            raise ValueError('Duplicate archive member')
        info = wav_info(source)
        if info['sha256'] != expected:
            raise ValueError('Supplement or mix audio identity changed')
        members[member] = source
        catalogue.append(dict(info, path=member, scope=scope))
        return info

    primary = []
    for key in ('reference', 'native'):
        source = Path(report[key + '_wav'])
        primary.append(add(source, f'{root}/MEZCLAS/{source.name}',
                           report[key + '_wav_sha256'], report['scope']))
    if (primary[0]['frames'] != primary[1]['frames'] or
            primary[0]['frames'] != report['seconds'] * 44100):
        raise ValueError('Complete mix duration mismatch')
    for row in report['excerpts']:
        source = Path(row['wav'])
        info = add(source, f'{root}/MEZCLAS/{source.name}', row['sha256'],
                   'Fixed excerpt of the complete mix; identical levels and time base')
        if info['frames'] != report['excerpt_seconds'] * 44100:
            raise ValueError('Mix excerpt duration mismatch')
    supplement = None
    if a.supplement:
        supplement = json.loads(a.supplement.read_text())
        for row in supplement['files']:
            if not row.get('scope'):
                raise ValueError('Supplementary audio needs an explicit scope')
            add(row['path'], row['archive_path'], row['sha256'], row['scope'])
    manifest = {
        'purpose': 'Private listening; protected audio, do not publish to GitHub',
        'source_report_sha256': sha(a.report),
        'source_supplement_sha256': sha(a.supplement) if a.supplement else None,
        'firmware_sha256': report['native_firmware_sha256'],
        'build': report['native_runtime_gates_preserved']['build'],
        'native_runtime_gates': report['native_runtime_gates_preserved'],
        'scope': report['scope'],
        'alignment': {
            'method': 'Observed API admissions; full native lead and latency retained',
            'native_music_lead_frames_not_removed': report['native_music_lead_not_removed_samples'],
            'DAC_alignment': 'Not independently calibrated',
        },
        'files': catalogue,
        'normalization': False, 'gain': 1, 'resampling': False, 'time_stretch': False,
        'production_ready': False, 'owner_listening_approval': 'PENDING',
        'hardware_status': 'HARDWARE PENDING',
    }
    readme = '''# Audio 2 — mezclas y observaciones privadas

La pareja MEZCLA reproduce 64 comandos originales observados en TAG durante
138 segundos. La referencia ejecuta el Z80 original, YM y OKI; el WAV Saturn
procede de Mednafen 1.32.1. El manifiesto conserva todos los fallos medidos.

Es un replay aislado de las admisiones de la API. No se presenta como captura
de una placa arcade ni como ejecución simultánea de gráficos y combate.
El lead musical nativo de 4096 Fs (92,88 ms) permanece en esta comparación.
El fragmento 50–80 segundos pertenece a los dos archivos completos.

Si se incluyen WAV adicionales de combate real o experimentos PCM, cada uno
tiene un alcance propio en MANIFIESTO.json. No se convierten en una pareja
arcade/Saturn exacta por estar en el mismo ZIP. Una variante experimental no
sustituye el firmware seleccionado.

Todos los WAV son PCM16/44100 Hz sin normalización, cambio de ganancia,
remuestreo o estiramiento temporal. La aprobación auditiva sigue pendiente.
HARDWARE PENDING. No publicar este audio ni recursos ROM en GitHub.
'''
    if supplement is not None and supplement.get('notes'):
        readme += '\n## Observaciones adicionales\n\n' + supplement['notes'] + '\n'
    extras = {
        f'{root}/LEEME.md': readme.encode(),
        f'{root}/MANIFIESTO.json': (json.dumps(manifest, indent=2, ensure_ascii=False) + '\n').encode(),
        f'{root}/MEZCLAS/INFORME-ORIGINAL.json': a.report.read_bytes(),
    }
    if supplement is not None:
        extras[f'{root}/SUPLEMENTO.json'] = a.supplement.read_bytes()
    sums = [f'{sha(source)}  {name}\n' for name, source in sorted(members.items())]
    sums += [f'{hashlib.sha256(data).hexdigest()}  {name}\n' for name, data in sorted(extras.items())]
    extras[f'{root}/SHA256SUMS'] = ''.join(sums).encode()
    a.out.parent.mkdir(parents=True, exist_ok=True)
    with zipfile.ZipFile(a.out, 'x', compression=zipfile.ZIP_DEFLATED) as archive:
        for name, source in sorted(members.items()):
            archive.writestr(zipfile.ZipInfo(name, (2026, 11, 1, 0, 0, 0)),
                             source.read_bytes(), compress_type=zipfile.ZIP_DEFLATED)
        for name, data in sorted(extras.items()):
            archive.writestr(zipfile.ZipInfo(name, (2026, 11, 1, 0, 0, 0)),
                             data, compress_type=zipfile.ZIP_DEFLATED)
    with zipfile.ZipFile(a.out) as archive:
        if archive.testzip() is not None:
            raise ValueError('ZIP CRC failure')
        for line in sums:
            checksum, name = line.rstrip('\n').split('  ', 1)
            if hashlib.sha256(archive.read(name)).hexdigest() != checksum:
                raise ValueError('Packaged member differs')
    receipt = {'path': str(a.out.resolve()), 'sha256': sha(a.out),
               'bytes': a.out.stat().st_size, 'wav_files': len(catalogue),
               'all_members_verified_after_packaging': True, 'production_ready': False}
    a.out.with_suffix('.json').write_text(json.dumps(receipt, indent=2) + '\n')
    print(json.dumps(receipt, indent=2))


if __name__ == '__main__':
    main()
