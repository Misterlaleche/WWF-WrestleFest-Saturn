#!/usr/bin/env python3
"""Publish this launched child's /proc identity, then exec the unchanged binary.

Usage: python launch_identity.py IDENTITY.json COMMAND ARGS...
No process list scanning, emulator hooks, memory writes or privilege changes.
The launcher should wait for /proc/proc_pid/exe to identify the expected binary
before attaching its read-only observer, because this record precedes exec.
"""

import json
import os
from pathlib import Path
import sys
import time


def main():
    if len(sys.argv) < 3:
        raise SystemExit(__doc__)
    destination = Path(sys.argv[1])
    command = sys.argv[2:]
    if command[0] == '--':
        command = command[1:]
    if not command:
        raise SystemExit('Missing executable command')
    stat = Path('/proc/self/stat').read_text()
    fields = stat.rpartition(')')[2].split()
    status = {}
    for line in Path('/proc/self/status').read_text().splitlines():
        key, separator, value = line.partition(':')
        if separator:
            status[key] = value.strip()
    identity = dict(namespace_pid=os.getpid(), proc_pid=int(stat.split(' ', 1)[0]),
                    proc_parent_pid=int(status['PPid']),
                    namespace_ids=[int(value) for value in status.get('NSpid', str(os.getpid())).split()],
                    process_start_ticks=int(fields[19]), command=command,
                    host_monotonic_ns=time.monotonic_ns(),
                    scope='Own child identity before exec; PID and start ticks persist across exec')
    temporary = destination.with_name(destination.name + '.tmp.' + str(os.getpid()))
    with temporary.open('x') as output:
        json.dump(identity, output, indent=2)
        output.write('\n')
    try:
        # Publish a complete record atomically, retaining exclusive-create
        # semantics if a caller accidentally supplied an existing identity.
        os.link(temporary, destination)
    finally:
        temporary.unlink()
    os.execvpe(command[0], command, os.environ)


if __name__ == '__main__':
    main()
