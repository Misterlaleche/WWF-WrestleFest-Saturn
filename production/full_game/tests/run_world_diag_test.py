"""Exercise optional world-commit fault publication and normal compilation."""
from pathlib import Path
import hashlib
import subprocess
import tempfile

root = Path(__file__).resolve().parents[1]
include = root / 'vendor/tag_r3/src/runtime/include'
source = root / 'vendor/tag_r3/src/runtime/src/wf_r18_world.c'
test = root / 'tests/world_diag_test.c'
text = source.read_text()
start = text.index('\n#else\nwf_status_t wf_r18_world_commit(') + len('\n#else\n')
end = text.rindex('\n#endif')
release_body = text[start:end].rstrip() + '\n'
assert hashlib.sha256(release_body.encode()).hexdigest() == (
    '22dd7ece9776e667b1ec4fa7970dc107501b5fc95b3fb8316063fb49c69f3248'
), 'Normal world_commit body differs from the approved original'
with tempfile.TemporaryDirectory(prefix='wf-world-diag-') as tmp:
    exe = Path(tmp) / 'world_diag_test'
    subprocess.run(['cc', '-Wall', '-Wextra', '-Werror', '-DWF_R18_WORLD_DIAG',
                    '-I', str(include), str(test), str(source), '-o', str(exe)], check=True)
    subprocess.run([str(exe)], check=True)
    subprocess.run(['cc', '-Wall', '-Wextra', '-Werror', '-I', str(include),
                    '-c', str(source), '-o', str(Path(tmp) / 'world_plain.o')], check=True)
print('PASS diagnostic test and plain build')
