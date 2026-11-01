#!/usr/bin/env python3
"""Freeze a complete source package before a multi-build experiment.

Hash both ends of the copy so a concurrent edit cannot silently produce a
mixed source profile. Generated output must remain outside the source package.
"""
import hashlib
from pathlib import Path
import shutil


IGNORED_DIRECTORIES = {'.git', '__pycache__', 'build'}


def source_identity(package):
    package = Path(package).resolve()
    result = {}
    for path in sorted(package.rglob('*')):
        relative = path.relative_to(package)
        if any(part in IGNORED_DIRECTORIES for part in relative.parts):
            continue
        if path.suffix == '.pyc':
            continue
        if path.is_symlink():
            raise ValueError('Source snapshot requires regular files: ' + str(relative))
        if path.is_file():
            result[str(relative)] = hashlib.sha256(path.read_bytes()).hexdigest()
    return result


def snapshot_package(package, destination):
    package, destination = Path(package).resolve(), Path(destination).resolve()
    if destination == package or package in destination.parents:
        raise ValueError('Source snapshot must be outside the source package')
    before = source_identity(package)
    if not before:
        raise ValueError('Source package is empty')
    shutil.copytree(package, destination,
                    ignore=shutil.ignore_patterns(*IGNORED_DIRECTORIES, '*.pyc'))
    copied = source_identity(destination)
    after = source_identity(package)
    if before != after or before != copied:
        raise RuntimeError('Source changed while making snapshot; no experiment was started')
    return copied
