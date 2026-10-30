WP1980 standalone Mednafen capture
=================================

This launcher runs the unmodified standalone Mednafen 1.32.1 executable.
It does not run a libretro frontend or load the Beetle core. The only code
imported from the older capture utility is its read-only ISO9660/ELF identity
validation; that source is frozen with each capture for provenance.

The launcher verifies the supplied ELF-derived binary against A.BIN in the
disc, freezes the CUE/track/ELF, and records source hashes before and after.
Use a new output directory each time. Do not overwrite the original input
paths while a capture is running, even though the process reads its frozen
private copy. No source repository or native executable is modified.

Verified environment
--------------------
The old wp1979/tooling/host/usr/games/mednafen actually reports 1.29.0. The
launcher therefore uses the already-supplied 1.32.1 binary at:
/workspace/scratch/8e35fd0cc989/wp02_r02/deps/mednafen132/usr/games/mednafen

Its existing runtime libraries are under deps/sysroot. These are read-only
dependencies; the tool performs no installation. Version, executable hash,
loaded library versions, settings and the emulator's own startup log are
retained. The real US/EU BIOS is supplied by an explicit path, read-only;
it is not copied into the capture or deliverable.

Portable paths
--------------
--mednafen, --sdk, --bios and --actors-layout accept explicit paths.
WP1980_MEDNAFEN_RUNTIME_ROOT selects the prefix containing lib/ and usr/lib/;
WP1980_HOST_TOOLS_ROOT selects the prefix containing usr/bin/Xvfb and xdotool.
Both default to the known supplied dependencies. Set either prefix to / for
the corresponding dependencies installed normally on a Linux system.

The standalone launcher also requires Python 3, Pillow for the imported disc
validation module, ffmpeg and ffprobe. Xvfb/xdotool are needed only for actual
keyboard/controller tests. No X11 display is needed for --offscreen. --no-movie omits qtrecord and
encoding entirely for counter-only runs; its report explicitly records no movie.

Normal timing and video
-----------------------
The default requests native -ss.dbg_cem full and refuses success unless the
emulator logs CPU Cache Emulation Mode: Full. This override is supported by
the supplied 1.32.1 executable. It is not an overclock. Default CD software
would otherwise use data-only cache emulation; the report always names the
mode actually logged. No separate Beetle interleave setting is claimed.

Region is explicitly NTSC North America. --cartridge 4MiB selects extram4;
--cartridge None selects none. These are ordinary hardware configurations,
checked against the native emulator log. Frame skipping is disabled,
nothrottle is 0 and fast-forward speed is 1. No fast-forward key is sent.

Video/audio come from Mednafen's own -qtrecord writer: native.mov is CSCD
lossless video plus 44.1 kHz stereo PCM. The dummy host audio driver still
generates and records emulated sound. The recorder's selected geometry is
retained; the tested source MOV uses 704x480. No screen compositor capture,
generated frame, video playback implementation or audio replacement is used.
Aspect correction is disabled for raw pixel recording, horizontal overscan
is retained, NTSC scanlines 0..239 are selected, and recorder doubling
thresholds are 0. Those choices affect recording/display, not the CPU clock.

native.mp4 is encoded from the retained original MOV using fps passthrough,
the source encoder time base and a 44100-track timescale. There is no -r,
setpts, atempo or other speed change. Smoke verification compared every
video PTS: all 2389 MOV/MP4 timestamps were identical. The last-packet
duration metadata differed by 25 ticks (about 0.567 ms), because the muxers
infer the last duration; no displayed frame was shifted or removed.

Do not use ffprobe r_frame_rate 60000/1001 as measured game cadence. Use
the original MOV PTS/time_base and native update/VBlank counter deltas.
Host wall time is recorded separately and is never a native speed result.

Read-only observer
------------------
observer.py opens only the launched child's /proc memory with O_RDONLY.
launch_identity.py publishes that child's own PID namespace identity before
exec, atomically, because the environment's /proc uses host PIDs while Popen
returns namespace PIDs. The observer verifies parent PID, namespace PID,
start ticks and executable hash. It never scans unrelated process memory.

Two consecutive 4096-byte instruction blocks must match the exact ELF-derived
program in HWRAM. Bytes are reconstructed from Mednafen's native uint16
storage. Watches come from the exact ELF; all nine 268-byte actors and RNG
use the locally compiled SH2 ABI file. The final HWRAM dump is a read-only
live memory observation, not an emulator save state.

Samples are asynchronous. Counter guards detect a boundary changing during
a read, but an unchanged guard does not prove an atomic actor snapshot.
Clock values skipped by polling are reported. Every recorded video frame
still comes from Mednafen independently of the observer sampling rate.
samples.jsonl.gz stores full changed observations; telemetry.csv contains
only scalar counters, avoiding duplicate large actor records.

Exit and stopping
-----------------
The default run continues until the native original result presentation
reaches finished_presentation 2, then records --post-fields more VBlanks.
--stop-game-frame is a bounded smoke-test stop. --run-fields is measured
from the first observed native started flag and is useful for controls.

With X11, the launcher exits using ordinary F12. With --offscreen, it sends
SIGINT to Mednafen's normal handler; the verified smoke exited with code 0,
closed a valid MOV and retained all video/audio. The report states which
exit method was actually used. Error cleanup is not accepted as a successful
capture. No pause, debugger, breakpoint, frame stepping or save state is used.

Ordinary controls
-----------------
Default physical mapping is recorded explicitly:
W/S/A/D = Saturn Up/Down/Left/Right; keypad 1/2/3 = Saturn A/B/C; Return=Start.
The launcher sends only normal X11 keydown/keyup events. Action intervals
are relative to the first observed started 1 with game_frame>=1; actual event
times and native clock values are recorded. They are not RAM input writes.

--scenario movement-reset covers four directions, A/B/C, then A+B+C+Start.
--scenario reset-first sends that chord as the first physical input.
Use --run-fields 2300 for those acceptance scenarios. --press button:first:last
adds an ordinary inclusive interval. The report records physical pad state,
active-low gameplay input and reset/init counters for later assertions.

The supplied Xvfb has a compiled /usr/bin/xkbcomp helper path, but that
helper is supplied under its own read-only prefix. On this specific system
only, libxvfb_xkb_path.so redirects Xvfb's exported helper-directory pointer.
It checks the exact Xvfb executable and initial path, then removes its own
LD_PRELOAD before child helpers. It is never passed to Mednafen. Its source,
hash and scope are recorded in an X11 capture. Normally installed Xvfb with
/usr/bin/xkbcomp needs no adapter. Local display socket creation may require
the environment's ordinary execution approval; no permissions are changed.

Examples from the workspace root
--------------------------------
python wp1980/tooling/mednafen/run_mednafen.py DISC.cue OUTPUT --elf PROGRAM.elf --cartridge 4MiB --offscreen --post-fields 240 --wall-timeout 1200
python wp1980/tooling/mednafen/run_mednafen.py DISC.cue OUTPUT --elf PROGRAM.elf --cartridge None --offscreen --post-fields 240 --wall-timeout 1200
python wp1980/tooling/mednafen/run_mednafen.py DISC.cue OUTPUT --elf PROGRAM.elf --cartridge 4MiB --scenario movement-reset --run-fields 2300
python wp1980/tooling/mednafen/run_mednafen.py DISC.cue OUTPUT --elf PROGRAM.elf --cartridge 4MiB --scenario reset-first --run-fields 2300

The symbolic DISC/OUTPUT/PROGRAM names are placeholders, not session files.

Validation and scope
--------------------
python wp1980/tooling/mednafen/test_observer.py
The observer tests use only a synthetic child process owned by the test.
They cover byte order, odd offsets, nine actors, RNG, exact ELF watch bounds,
signature rejection, clock gaps, dumps and identity preservation across exec.

report.json PASS means the requested capture, identities, logged hardware
settings, observation and recording checks passed. It is not a claim of
physical Saturn validation, uniform frame rate or complete arcade pixel
equivalence. Check the frozen ELF/disc hashes and the capture's scenario
before treating it as final acceptance evidence.

For complete recordings, run:
python wp1980/tooling/mednafen/verify_media.py CAPTURE_DIRECTORY
python wp1980/tooling/mednafen/verify_audio.py CAPTURE_DIRECTORY --start 45.5 --duration 120
python wp1980/tooling/mednafen/analyze_execution.py CAPTURE_DIRECTORY --reference EARLIER_NATIVE_REPORT.json

The media check compares every source/delivery video PTS exactly, stores
SHA256 hashes and derives the recorded field rate from the first/last
timestamps. The release recording has 11584 identical source/delivery PTS
and measures 59.8261259219 fields/s. Always use the recording being analyzed
for its own rate. The audio check measures signal non-silence in an explicit
combat interval, excluding the BIOS startup sound; it is not a listening
quality judgment. Filter statistics can include a partial boundary packet.

analyze_execution.py uses native completed-draw counters, reports first and
worst observed windows, and excludes ambiguous or missing endpoints instead
of interpolating them. It compares all nine terminal actors and RNG if a
reference is supplied. For a --no-movie run, use --pts-check pointing to a
verified same-configuration primary video-pts-check.json. The report labels
that external calibration reuse explicitly.

For controller evidence, add the current executable's validated ABI watches
--watch frame_ready:rt+7644:1 --watch frame_stage:rt+7646:1 to the capture and
run analyze_controls.py CAPTURE_DIRECTORY afterward. Those offsets were
compiled from the release SH2 headers; recheck them for a different runtime.
The analyzer qualifies walking displacement only in consecutive observed
updates with stable inputs, character and free-walking state. It retains
missing button evidence as INCONCLUSIVE. For example, a short physical key
pulse entirely inside a synchronous CD load can be released before native
controller sampling resumes; a capture PASS alone does not prove its receipt.
test_analyze_controls.py checks these evidence qualifications on synthetic
fixtures; it does not emulate or modify the game.

Large captures should use a fresh /tmp directory in this environment to avoid
live workspace synchronization producing duplicate large partial files. Keep
at least 2GiB available and run one large movie at a time. Retain the original
MOV until its hash/PTS/MP4 are verified; a recording with any write error is
not accepted, even if gameplay continued and all native counters completed.
