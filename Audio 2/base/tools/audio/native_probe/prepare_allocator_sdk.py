"""Select Yaul's real allocator fat object and keep its native SH-2 relocations.

GCC LTO resolves calls inside the SDK before GNU ld --wrap can intercept them.
Only this SDK object must use its existing native code; game LTO is unchanged.
The installed SDK archive is read-only and never modified.
"""
import argparse
from pathlib import Path
import subprocess


def allocator_object(archive):
    data = archive.read_bytes()
    if not data.startswith(b'!<arch>\n'):
        raise ValueError('expected a normal SDK ar archive')
    offset = 8
    found = []
    while offset + 60 <= len(data):
        header = data[offset:offset + 60]
        size = int(header[48:58])
        blob = data[offset + 60:offset + 60 + size]
        if blob.startswith(b'\x7fELF') and b'___mm_init\0' in blob and b'_private_pool\0' in blob:
            found.append(blob)
        offset += 60 + size + (size & 1)
    if len(found) != 1:
        raise ValueError(f'expected one SDK allocator object, found {len(found)}')
    return found[0]


def prepare(archive, objcopy, output):
    output.parent.mkdir(parents=True, exist_ok=True)
    original = output.with_suffix('.fat.o')
    original.write_bytes(allocator_object(archive))
    subprocess.run([str(objcopy), '--remove-section=.gnu.lto_*', str(original), str(output)], check=True)
    if b'.gnu.lto_' in output.read_bytes():
        raise ValueError('SDK allocator still contains LTO sections')


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--archive', type=Path, required=True)
    parser.add_argument('--objcopy', type=Path, required=True)
    parser.add_argument('--out', type=Path, required=True)
    args = parser.parse_args()
    prepare(args.archive, args.objcopy, args.out)
