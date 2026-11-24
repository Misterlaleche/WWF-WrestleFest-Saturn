# Prepared WAV header reference

`original_prepare.c` contains the original SDK WAV parser and preparation functions before cached-header preparation. Only function names were changed for side-by-side execution. The original BSD notice is retained. `provenance.json` identifies the source and fixture bytes; the test verifies the fixture hash before compiling it.

The test resets the shared music state between calls and replaces filesystem operations at the I/O boundary. It compares ordinary and cached preparation against the frozen original on all 18 current WAVs, corrupt headers, short reads and failures. This is a host semantic oracle, not a CD timing or hardware model.

From the repository root, with the verified `cd` assets present:

```
python3 tests/test_music_headers_api.py --source . --assets cd --output build/music-headers-api
python3 tests/test_music_headers_generation.py --source . --assets cd --output build/music-headers-generation
python3 tests/test_music_headers_order.py --source . --assets cd --output build/music-headers-order
```

The optional API `--red` control runs the original sector-read path and requires it to fail the zero-read target. The normal order test also compiles private invalid-ID variants; it never changes the repository adapter.
