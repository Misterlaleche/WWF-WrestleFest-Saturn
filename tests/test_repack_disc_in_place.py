#!/usr/bin/env python3
import importlib.util
import tempfile
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
TOOL = ROOT / 'tools' / 'repack_disc_in_place.py'
SECTOR = 2048
RAW = 2352
SYNC = b'\0' + b'\xff' * 10 + b'\0'


def both(value, width):
    return value.to_bytes(width, 'little') + value.to_bytes(width, 'big')


def record(identifier, extent, size, flags=0):
    name = identifier if isinstance(identifier, bytes) else identifier.encode('ascii')
    length = 33 + len(name) + (not len(name) % 2)
    out = bytearray(length)
    out[0] = length
    out[1] = 0
    out[2:10] = both(extent, 4)
    out[10:18] = both(size, 4)
    out[18:25] = bytes((126, 1, 1, 0, 0, 0, 0))
    out[25] = flags
    out[26] = out[27] = 0
    out[28:32] = both(1, 2)
    out[32] = len(name)
    out[33:33 + len(name)] = name
    return bytes(out)


def minimal_iso():
    sectors = [bytearray(SECTOR) for _ in range(32)]
    root_extent = 20
    root_size = SECTOR
    pvd = sectors[16]
    pvd[0:7] = b'\x01CD001\x01'
    pvd[80:88] = both(len(sectors), 4)
    pvd[120:124] = both(1, 2)
    pvd[124:128] = both(1, 2)
    pvd[128:132] = both(SECTOR, 2)
    root = record(b'\0', root_extent, root_size, 2)
    pvd[156:156 + len(root)] = root
    pvd[881] = 1
    sectors[17][0:7] = b'\xffCD001\x01'
    directory = b''.join((
        record(b'\0', root_extent, root_size, 2),
        record(b'\1', root_extent, root_size, 2),
        record('0.BIN;1', 21, 10),
        record('MU01.WAV;1', 22, 12),
    ))
    sectors[root_extent][:len(directory)] = directory
    sectors[21][:10] = b'0123456789'
    sectors[22][:12] = b'hello world!'
    return b''.join(sectors)


def raw_wrap(iso):
    result = bytearray()
    for lba in range(len(iso) // SECTOR):
        address = lba + 150
        minute, second, frame = address // 4500, (address // 75) % 60, address % 75
        bcd = lambda value: ((value // 10) << 4) | value % 10
        sector = bytearray(RAW)
        sector[:12] = SYNC
        sector[12:16] = bytes((bcd(minute), bcd(second), bcd(frame), 1))
        sector[16:16 + SECTOR] = iso[lba * SECTOR:(lba + 1) * SECTOR]
        result += sector
    return bytes(result)


def load_tool():
    spec = importlib.util.spec_from_file_location('repack_disc_in_place', TOOL)
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module


def exercise(template_bytes, expected_format):
    tool = load_tool()
    with tempfile.TemporaryDirectory(prefix='wf-repack-test-') as folder:
        folder = Path(folder)
        template = folder / ('template.bin' if expected_format == 'mode1-2352' else 'template.iso')
        output = folder / 'output.iso'
        program = folder / 'game.raw'
        music = folder / 'MU01.WAV'
        template.write_bytes(template_bytes)
        program.write_bytes(b'new-code')
        music.write_bytes(b'new music!!!')
        report = tool.repack(template, output, {'0.BIN': program, 'MU01.WAV': music})
        assert report['template_format'] == expected_format, report
        assert report['files'] == 2, report
        assert report['volume_blocks'] == 32, report
        assert output.stat().st_size == 32 * SECTOR
        with tool.DiscImage(output, 'iso2048') as image:
            entries = image.root_entries()
            assert entries['0.BIN'].size == 8
            assert b''.join(image.stream(entries['0.BIN'])) == b'new-code'
            assert entries['MU01.WAV'].size == 12
            assert b''.join(image.stream(entries['MU01.WAV'])) == b'new music!!!'
        raw = output.read_bytes()
        assert raw[21 * SECTOR + 8:(21 + 1) * SECTOR] == b'\0' * (SECTOR - 8)


def main():
    iso = minimal_iso()
    exercise(iso, 'iso2048')
    exercise(raw_wrap(iso), 'mode1-2352')
    tool = load_tool()
    with tempfile.TemporaryDirectory(prefix='wf-repack-negative-') as folder:
        folder = Path(folder)
        template = folder / 'template.iso'; template.write_bytes(iso)
        huge = folder / 'huge.bin'; huge.write_bytes(b'x' * (SECTOR + 1))
        try:
            tool.repack(template, folder / 'out.iso', {'0.BIN': huge})
        except ValueError as error:
            assert 'allocated extent' in str(error)
        else:
            raise AssertionError('extent expansion was accepted')
    print('repack_disc_in_place: PASS')


if __name__ == '__main__':
    main()
