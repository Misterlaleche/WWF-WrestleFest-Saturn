#!/usr/bin/env python3
"""Rebuild an ISO2048 image by replacing files in a locked root layout.

The template may be ISO2048 or a single MODE1/2352 data track.  File extents
are never moved or expanded: a replacement must fit in the sectors already
allocated to that root file.  The complete old extent is zeroed, the new bytes
are written, and the ISO9660 directory record is updated in both byte orders.

This is intentionally a layout-preserving tool for reproducible Saturn test
images when the original mkisofs implementation is unavailable.  It does not
create new ISO files, directories or extents.
"""
from __future__ import annotations

import argparse
import hashlib
import json
import os
from pathlib import Path
import tempfile

import sys

TOOLS = Path(__file__).resolve().parent
if str(TOOLS) not in sys.path:
    sys.path.insert(0, str(TOOLS))
from extract_disc_assets import (
    AssetError, DiscImage, SECTOR, parse_record, require, safe_name,
)


def digest(path: Path) -> str:
    value = hashlib.sha256()
    with path.open('rb') as source:
        for block in iter(lambda: source.read(1 << 20), b''):
            value.update(block)
    return value.hexdigest()


def _root_record_offsets(iso_path: Path):
    """Return {primary root name: (record byte offset, parsed entry)}."""
    with iso_path.open('rb') as source:
        source.seek(16 * SECTOR)
        primary = source.read(SECTOR)
        require(len(primary) == SECTOR and primary[:7] == b'\x01CD001\x01',
                'Primary volume descriptor is missing after template decode')
        root = parse_record(primary[156:190])
        require(root.identifier == b'\0' and root.flags == 2 and root.size % SECTOR == 0,
                'Invalid root directory in decoded template')
        result = {}
        for sector_index in range(root.size // SECTOR):
            base = (root.extent + sector_index) * SECTOR
            source.seek(base)
            block = source.read(SECTOR)
            require(len(block) == SECTOR, 'Truncated root directory')
            at = 0
            while at < SECTOR:
                length = block[at]
                if not length:
                    require(not any(block[at:]), 'Nonzero root directory padding')
                    break
                require(length >= 34 and at + length <= SECTOR,
                        'Root directory record crosses a sector')
                entry = parse_record(block[at:at + length])
                if entry.identifier not in (b'\0', b'\1'):
                    try:
                        identifier = entry.identifier.decode('ascii')
                    except UnicodeDecodeError as error:
                        raise AssetError('Non-ASCII root filename is unsupported') from error
                    name, separator, version = identifier.rpartition(';')
                    if separator:
                        require(version == '1', 'Only ISO file version ;1 is supported')
                    else:
                        name = identifier
                    safe_name(name)
                    require(name not in result, f'Duplicate root filename: {name}')
                    result[name] = (base + at, entry)
                at += length
        return result


def _decode_template(template: Path, output) -> tuple[str, int, int]:
    with DiscImage(template, 'auto') as image:
        # Parse first so the complete ISO structure is validated before output.
        image.root_entries()
        for lba in range(0, image.image_blocks, 256):
            count = min(256, image.image_blocks - lba)
            data = image.blocks(lba, count)
            require(output.write(data) == len(data), 'Short write while decoding template')
        return image.format, image.image_blocks, image.volume_blocks


def repack(template, output_iso, replacements):
    template = Path(template)
    output_iso = Path(output_iso)
    require(template.resolve() != output_iso.resolve(), 'Template and output must be different files')
    require(isinstance(replacements, dict) and replacements, 'At least one replacement is required')

    checked = {}
    for name, source in replacements.items():
        safe_name(name)
        source = Path(source)
        require(source.is_file() and not source.is_symlink(),
                f'Replacement must be a regular file: {source}')
        require(source.stat().st_size <= 0xffffffff, f'Replacement exceeds ISO9660 size field: {name}')
        checked[name] = source

    output_iso.parent.mkdir(parents=True, exist_ok=True)
    temporary = None
    try:
        with tempfile.NamedTemporaryFile(dir=output_iso.parent, prefix='.wf-repack-',
                                         suffix='.iso', delete=False) as target:
            temporary = Path(target.name)
            template_format, image_blocks, volume_blocks = _decode_template(template, target)
            target.flush()
            os.fsync(target.fileno())

        records = _root_record_offsets(temporary)
        missing = sorted(set(checked) - set(records))
        require(not missing, 'Replacement root file is missing: ' + ', '.join(missing))

        report_files = []
        with temporary.open('r+b') as image:
            for name in sorted(checked):
                record_offset, entry = records[name]
                require(not entry.flags & 2, f'Replacement target is a directory: {name}')
                source = checked[name]
                size = source.stat().st_size
                old_blocks = (entry.size + SECTOR - 1) // SECTOR
                new_blocks = (size + SECTOR - 1) // SECTOR
                require(new_blocks <= old_blocks,
                        f'Replacement exceeds allocated extent for {name}: {new_blocks}>{old_blocks} sectors')

                extent_offset = entry.extent * SECTOR
                image.seek(extent_offset)
                remaining = old_blocks * SECTOR
                zero = b'\0' * (1 << 20)
                while remaining:
                    block = zero[:min(remaining, len(zero))]
                    require(image.write(block) == len(block), f'Short zero-fill for {name}')
                    remaining -= len(block)
                image.seek(extent_offset)
                with source.open('rb') as payload:
                    for block in iter(lambda: payload.read(1 << 20), b''):
                        require(image.write(block) == len(block), f'Short payload write for {name}')

                image.seek(record_offset + 10)
                require(image.write(size.to_bytes(4, 'little') + size.to_bytes(4, 'big')) == 8,
                        f'Short directory update for {name}')
                report_files.append(dict(name=name, old_bytes=entry.size, bytes=size,
                                         extent=entry.extent, sectors=old_blocks,
                                         sha256=digest(source)))
            image.flush()
            os.fsync(image.fileno())

        # Re-open through the strict parser and verify every replacement byte.
        with DiscImage(temporary, 'iso2048') as image:
            entries = image.root_entries()
            for row in report_files:
                entry = entries[row['name']]
                require(entry.size == row['bytes'], f'Directory size verification failed for {row["name"]}')
                value = hashlib.sha256()
                count = 0
                for block in image.stream(entry):
                    value.update(block)
                    count += len(block)
                require(count == row['bytes'] and value.hexdigest() == row['sha256'],
                        f'Payload verification failed for {row["name"]}')

        os.replace(temporary, output_iso)
        temporary = None
        return dict(template=str(template), template_format=template_format,
                    output=str(output_iso), output_sha256=digest(output_iso),
                    image_blocks=image_blocks, volume_blocks=volume_blocks,
                    files=len(report_files), replacements=report_files)
    finally:
        if temporary is not None:
            temporary.unlink(missing_ok=True)


def parse_replacements(values):
    result = {}
    for value in values:
        name, separator, path = value.partition('=')
        if not separator or not name or not path:
            raise argparse.ArgumentTypeError('--replace requires NAME=PATH')
        if name in result:
            raise argparse.ArgumentTypeError(f'duplicate replacement: {name}')
        result[name] = Path(path)
    return result


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--template', required=True, type=Path)
    parser.add_argument('--output-iso', required=True, type=Path)
    parser.add_argument('--replace', action='append', default=[], metavar='NAME=PATH')
    parser.add_argument('--report', type=Path)
    args = parser.parse_args()
    try:
        replacements = parse_replacements(args.replace)
        report = repack(args.template, args.output_iso, replacements)
        if args.report:
            args.report.parent.mkdir(parents=True, exist_ok=True)
            args.report.write_text(json.dumps(report, indent=2) + '\n', encoding='utf-8')
        print(f"Repacked {report['files']} files into {args.output_iso} from {report['template_format']}")
    except (AssetError, OSError, ValueError, argparse.ArgumentTypeError) as error:
        parser.exit(1, f'Repack failed: {error}\n')


if __name__ == '__main__':
    main()
