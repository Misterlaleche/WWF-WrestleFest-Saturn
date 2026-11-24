"""Validate the immutable B12 fixture before compiling any reference object."""
import hashlib
import json
from pathlib import Path
MANIFEST_SHA256 = '27649f792b71b9fb7836045a9c597a7b6245e5e6b02541b463503191d9ec8b24'
def verify_reference(base: Path):
    manifest = base/'manifest.json'
    if MANIFEST_SHA256 and hashlib.sha256(manifest.read_bytes()).hexdigest() != MANIFEST_SHA256:
        raise AssertionError('B12 reference manifest identity changed')
    data = json.loads(manifest.read_text())
    for name, expected in data['files'].items():
        actual = hashlib.sha256((base/name).read_bytes()).hexdigest()
        if actual != expected:
            raise AssertionError(f'B12 reference changed: {name}: {actual} != {expected}')
