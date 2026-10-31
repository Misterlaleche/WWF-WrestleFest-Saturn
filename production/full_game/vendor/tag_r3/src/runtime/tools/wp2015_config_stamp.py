#!/usr/bin/env python3
"""Invalidate native objects only when the actual build configuration changes."""
import sys
from pathlib import Path

request, stamp = map(Path, sys.argv[1:])
data = request.read_bytes()
if not stamp.exists() or stamp.read_bytes() != data:
    request.replace(stamp)
else:
    request.unlink()
