#!/usr/bin/env python3
"""Verify the untouched upstream source snapshot without requiring Git."""
import hashlib
import json
from pathlib import Path

PRODUCTION = Path(__file__).resolve().parents[1]
ROOT = PRODUCTION.parents[1]

def main():
    manifest = json.loads((PRODUCTION / "source-lock.json").read_text())
    failures = []
    for item in manifest["files"]:
        path = ROOT / item["path"]
        if not path.is_file() or path.is_symlink():
            failures.append({"path": item["path"], "reason": "missing/non-regular"})
            continue
        data = path.read_bytes()
        if len(data) != item["bytes"] or hashlib.sha256(data).hexdigest() != item["sha256"]:
            failures.append({"path": item["path"], "reason": "content differs"})
    print(json.dumps({"pass": not failures, "files_checked": len(manifest["files"]),
                      "upstream_commit": manifest["commit"], "failures": failures}, indent=2))
    return 1 if failures else 0

if __name__ == "__main__":
    raise SystemExit(main())
