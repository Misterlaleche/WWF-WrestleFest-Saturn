#!/usr/bin/env python3
from pathlib import Path
root=Path(__file__).resolve().parents[1]
text=(root/'makefile').read_text()
start=text.index('WF_COLD_FRONTEND_OBJS :=')
end=text.index('$(WF_COLD_FRONTEND_OBJS): CCFLAGS',start)
cold=text[start:end]
assert 'port/zlib/adler32.o' not in cold, 'SH-2 Adler-32 bootstrap checksum must keep the standard -O2 build'
print('PASS: Adler-32 is excluded from the -Os cold-object list')
