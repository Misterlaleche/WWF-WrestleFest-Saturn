#!/usr/bin/env python3
"""Exercise the real binary asset reader's boot observer and read boundaries."""
from pathlib import Path
import os
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[1]
HARNESS = r'''
#include "wf_jo_assets.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

/* Baseline can link before the observer exists: absence must fail behavior,
 * not compilation. All file access below is the production asset reader. */
extern void wf_jo_assets_set_loading_observer(
    void (*observer)(const char *, size_t, size_t)) __attribute__((weak));
static unsigned calls, started, remove_before_open;
static size_t previous, last_total;
static void observe(const char *name, size_t done, size_t total)
{
    assert(name && *name && done <= total);
    if (!done) { started = 1; previous = 0; }
    else { assert(started && done > previous && done - previous <= 32768u); }
    previous = done; last_total = total; ++calls;
    if (remove_before_open && !done) assert(unlink(name) == 0);
}
int main(int argc, char **argv)
{
    assert(argc == 2);
    unsigned char bytes[100005]; memset(bytes, 0xa5, sizeof bytes);
    if (wf_jo_assets_set_loading_observer)
        wf_jo_assets_set_loading_observer(observe);
    assert(wf_jo_assets_file_read(argv[1], 13, bytes + 1, 100003) == 0);
    assert(started && calls > 2 && "Boot load must report before and during I/O");
    assert(previous == 100003 && last_total == 100003);
    assert(bytes[0] == 0xa5 && bytes[100004] == 0xa5);
    for (unsigned i = 0; i < 100003; ++i)
        assert(bytes[i+1] == (unsigned char)((i+13)*29u+7u));
    wf_jo_assets_set_loading_observer(NULL); calls = 0;
    assert(wf_jo_assets_file_read(argv[1], 9, bytes, 51) == 0 && !calls);
    wf_jo_assets_set_loading_observer(observe);
    remove_before_open = 1; started = 0;
    assert(wf_jo_assets_file_read(argv[1], 0, bytes, 128) == WF_JO_ASSET_IO);
    assert(started && calls == 1 && previous == 0 && last_total == 128);
    /* The physical failure occurred in file_size, which has its own open.
     * Removing this new file from the observer proves that it too reports
     * the filename before entering the blocking operation. */
    FILE *f = fopen(argv[1], "wb"); assert(f);
    assert(fwrite("HEAD", 1, 4, f) == 4 && fclose(f) == 0);
    uint32_t size = 0; calls = started = 0;
    assert(wf_jo_assets_file_size(argv[1], &size) == WF_JO_ASSET_IO);
    assert(started && calls == 1 && previous == 0 && last_total == 0);
    puts("PASS: size/read observers precede blocking open; chunked read is exact; observer detaches; failures retain last position");
}
'''

def main():
    with tempfile.TemporaryDirectory(prefix="wf-boot-load-") as folder:
        work = Path(folder)
        (work / "probe.c").write_text(HARNESS)
        (work / "asset.bin").write_bytes(bytes((i * 29 + 7) & 255 for i in range(100100)))
        vendor = [ROOT / "port/zlib" / (name + ".c") for name in
                  ("inflate", "inffast", "inftrees", "adler32", "zutil")]
        command = [os.environ.get("CC", "cc"), "-std=c11", "-O2", "-g",
                   "-DWF_JO_ASSETS_HOST", "-DZ_SOLO", "-DNO_GZIP",
                   "-fsanitize=address,undefined", "-fno-omit-frame-pointer",
                   "-I" + str(ROOT / "port"), "-I" + str(ROOT / "port/zlib"),
                   str(work / "probe.c"), str(ROOT / "port/wf_jo_assets.c"),
                   *map(str, vendor), "-o", str(work / "probe")]
        subprocess.run(command, check=True)
        subprocess.run([str(work / "probe"), str(work / "asset.bin")], check=True,
                       env=dict(os.environ, ASAN_OPTIONS="detect_leaks=0"))

if __name__ == "__main__":
    main()
