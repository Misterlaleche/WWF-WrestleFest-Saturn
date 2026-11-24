#!/usr/bin/env python3
import hashlib
import json
from pathlib import Path
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[1]
REF = ROOT / 'tools' / 'audio_reference'
ROM = ROOT / 'build' / 'p1-audio' / 'reference'


def main():
    with tempfile.TemporaryDirectory(prefix='wf-render-rate-test-') as temporary:
        tmp = Path(temporary)
        exe = tmp / 'capture_board_replay'
        subprocess.run(['cc','-O2','-std=c99','-I'+str(REF),
            str(REF/'capture_board_replay.c'),str(REF/'z80.c'),str(REF/'opm.c'),'-lm','-o',str(exe)],check=True)
        schedule = tmp / 'schedule.csv'
        schedule.write_text('0,2\n')
        report = tmp / 'report.json'
        result = subprocess.run([str(exe),str(ROM/'SOUND.ROM'),str(ROM/'SAMPLES.ROM'),str(schedule),'1',
            str(tmp/'trace.csv'),str(tmp/'audio.pcm'),str(report),str(tmp/'deliveries.csv'),'full','176400'])
        assert result.returncode == 0, ('renderer rejected explicit output rate', result.returncode)
        data = json.loads(report.read_text())
        assert data['output_rate'] == 176400, data
        assert data['frames'] == 176400, data
        assert (tmp/'audio.pcm').stat().st_size == 176400 * 2
        assert hashlib.sha256((tmp/'audio.pcm').read_bytes()).hexdigest() == \
            '23e3605314a686a73b0acc37a67b72edaaa3518a51e2683ba85a0da639f43648'
        # The optional rate must not alter the historical default capture used
        # by earlier reference fixtures.
        default_report = tmp / 'default-report.json'
        default_pcm = tmp / 'default.pcm'
        default = subprocess.run([str(exe),str(ROM/'SOUND.ROM'),str(ROM/'SAMPLES.ROM'),str(schedule),'1',
            str(tmp/'default-trace.csv'),str(default_pcm),str(default_report),
            str(tmp/'default-deliveries.csv'),'full'])
        assert default.returncode == 0
        default_data = json.loads(default_report.read_text())
        assert default_data['output_rate'] == 44100 and default_data['frames'] == 44100
        assert hashlib.sha256(default_pcm.read_bytes()).hexdigest() == \
            'faf1eac0decd12e9ff00e95d50686556a67a978e94496086d037f41ccd685d49'
    print('PASS renderer explicit 176400 Hz capture and unchanged 44100 Hz default')


if __name__ == '__main__':
    main()
