# Shared six-note Synth One DSP graph

This host experiment embeds six genuine Synth One note processors around one
shared context, PRNG, table descriptor set and two LFOs. It preserves the pinned
per-note/LFO compute arithmetic while replacing processor allocations with
caller-owned state. It is not a complete Synth One engine or firmware integration.

The parent experiment's source closure is pinned to project commit
`0258bfd846fdd00d5df8d4e7309b2c5888e218a3`. `DEPENDENCIES.json` records relative
paths, sizes, Git blob identities and SHA-256 hashes for the existing oracle,
interface, thirteen C modules, required headers, notices and public input manifest.
The preparation helper independently pins that dependency manifest's SHA-256 and
rejects changed bytes before a compiler runs. It never accepts an arbitrary
caller-supplied Soundpipe tree.

## Run

Requirements: Linux/POSIX host, Python 3, GNU-compatible C/C++ toolchain and linker
support for `--wrap`/`--gc-sections`, and little-endian IEEE-754 float32. The tests
use `mmap`, `mprotect` and `sysconf`; platform independence is not claimed.

Provide the original public Synth One source root at commit
`6466a3715c96b7ecf1dd255a218cbd571408a314`, containing
`AudioKitSynthOne/DSP/BandlimitedWavetables/`. Only its 52 waveform JSON files,
index and frequency file are consumed. No preset bank is required.

```sh
sh experiments/synthone-port-feasibility-20261114/shared-graph/run.sh /path/to/SynthOneSource o2
```

The first argument is required and can be relative to the caller or contain spaces.
The optional variant is `o2` (default), `os` or `sanitize`. The sanitizer variant
uses ASan, UBSan, explicit float-cast-overflow checks and leak detection. The
runner verifies inputs first, creates a fresh build directory, and links an
explicit sixteen-object list. Existing or unrelated object files are excluded.

Focused binding regressions run without rebuilding the C++ suite:

```sh
python3 experiments/synthone-port-feasibility-20261114/shared-graph/tests/test_bindings.py /path/to/SynthOneSource
```

They exercise normal Python, `python -O` and `PYTHONOPTIMIZE=1`, including changed
or missing files, reordered input, self-consistent manifest substitution and
preservation of previous output after rejection.

Local float32 payloads, reports, objects and executables remain under ignored
`build/` directories. Do not publish these generated binaries or use owner preset
banks as waveform input. The helper performs no download or SDK/toolchain setup.

## Authentic table domain

All waveform source files are checked against the parent experiment's pinned
public-source manifest by byte count, SHA-256 and Git blob identity. The index
orders thirteen bands, each triangle/square/PWM/saw, with 4096 samples per table.
Values are converted to source float32 representation and receive positive-zero
guards without clipping or scaling. Sine is generated separately by the original
`sp_gen_sine` algorithm.

Authentic morph rows exceed unity: 58,480 JSON values are outside `[-1,1]`; 58,476
remain outside after float32 conversion. Their extrema are
`±1.2820513248443604`. Morph admission therefore uses `[-2,2]`. Sine retains a
separate `[-1,1]` limit to protect the source FM phase-conversion domain. A uniform
normalized-table restriction would reject valid public morph data.

## Ownership and scheduling

`Graph` must remain at its final address; copying and moving are disabled. It owns
descriptors and processor state while borrowing immutable sample arrays until
teardown or successful re-init. Init validates before changing state. Reset
restores all six notes, LFOs and the shared PRNG, retaining table bindings and
setting active count to zero. Teardown is idempotent and never frees caller memory.
The Soundpipe mutable-pointer ABI is confined to binding; general Soundpipe const
correctness is not claimed. The test protects payloads read-only during processing.

Each frame processes the shared LFO graph once, then source notes `[0,count)` in
ascending order with direct additive accumulation. Explicit index controls do not
change count. Count zero advances LFOs while preserving note state, output and
PRNG; excluded suffix notes freeze. Mono frequency is a separate explicit control.

This prefix policy is intentionally narrower than the full upstream scheduler.
It omits allocation, stage/root-note eligibility, release removal, a separate mono
note, mono-switch reset, parameter ramps, sequencer and FX. Selected offstage notes
still run the source per-note function. No automatic allocator behavior or complete
app audio match is inferred.

## Proof and limits

The allocating oracle uses one context/PRNG, shared tables, two LFOs and six
original note initializers. Comparisons cover bitwise additive PCM and every
declared DSP/LFO field, shared PRNG, parameters, table metadata and bindings after
each frame, including inactive notes. Allocation wrappers fail immediately during
static graph actions and count the allocating oracle as a positive control.

Cases cover counts zero through six, asymmetric noise and order sensitivity,
held/released/restarted notes, inactive state, clear/reset/rebind, mono/fractional
controls, modulation/filter changes, table/band boundaries, Nyquist/FM15, invalid
inputs preserving state and independent graphs sharing payload.

Retained GCC 14.2 source-stage runs at `-O2`, `-Os` and with sanitizers each passed
15 cases, 42,864 bitwise PCM frames and 200,352 source voice calls. They compared
42,909 complete six-note/shared-state snapshots with zero intercepted heap calls
in static actions. The portable runner is also exercised separately against the
pinned dependencies; its input/dependency rejection is checked under optimized
Python. These are host results, not target timing or installed-app proof.

Caller graph size is 7,272 bytes on a 64-bit host. External tables plus sine remain
868,564 bytes and do not fit current application flash. Table storage, global FX
memory/topology, full event/mono scheduling, smoothing, tuning/rate snapping, arp,
persistence, target link/math/resources/CPU and whole-instrument acceptance remain
open. No engine replacement or firmware/device/release qualification follows.

License notices and original module author headers are retained in the parent
experiment's `notices/`, `audiokit-4.9.2/` and probe sources. No additional licensed
dependency or duplicate module copy is introduced here.
