# Compact Synth One parameter ports

This source experiment stores the pinned master's 46 selected parameter smoothers
as independent target/`yt1` pairs around one shared coefficient tuple. The existing
150 output floats remain caller-owned. The immutable 46-index/150-slot map occupies
196 read-only bytes; no unused unsmoothed target/state array or runtime list builder
is required.

The source projection is based on project commit
`a0c8698f724e28ec6275307525933c195817b3c5` beside the shared-graph experiment.
`DEPENDENCIES.json` pins seven existing sibling inputs by length, SHA-256 and Git
blob: actual AudioKit port/header dependencies, notices, the public-source manifest
and the earlier metadata flags. It also binds the six compiler-facing project files
and metadata generator. No module, license, upstream source or preset data is
duplicated here. The verifier independently fixes that manifest's hash, and the
isolated build driver fixes the verifier's hash.

## Source contract

The eligible master group comes from Synth One commit
`6466a3715c96b7ecf1dd255a218cbd571408a314`, with Soundpipe from AudioKit
`03fecf80a66be88f6ac0356cd532c72dbe09922d`.

- All 46 selected ports share rate and global half-time. Mono-frequency and LFO
  ports are separate. Future independent per-port rates, half-times, resets or
  scheduling require a new representation or fallback.
- Setting parameter 144 first changes its target, then applies the **current
  smoothed output** as the global half-time. The new target is not substituted,
  and half-time does not automatically track later output changes.
- Restore resets `yt1`, rate fields and the dirty sentinel while retaining old
  `c1/c2` until the next lazy compute. Fresh initialization uses the documented
  zero-filled reference fixture for otherwise unused pre-compute coefficient bits.
- Musical reset and panic do not reset parameter ports. Audio-unit rate allocation
  uses kernel init and value restore; changing context rate alone is insufficient.
- Coefficients preserve float division, source double `pow`, float storage and
  source subtraction/recurrence order. Fast math and contraction are disabled.

The API expects finite, already source-clamped and tempo/rate-resolved physical
targets. Its numeric admission `[-48000,48000]` covers current metadata ranges but
does not implement per-parameter clamps, tuning, taper, tempo snapping or the full
AudioKit setter graph. The direct uniform half-time API admits only finite
`[1e-6,0.99]`; admitted rates are 44100, 48000 and 96000.

`Ports` is noncopyable/nonmovable and borrows exactly 150 live, aligned float
outputs. One writer must serialize access and retain the buffer until teardown or
successful rebind. Initial values are consumed synchronously. Exact input/output
aliasing is supported; partial or module-storage overlap rejects before writes.
Views return values and cannot mutate individual coefficient state. Counts do not
prove actual allocation or lifetime, and concurrency is not qualified.

## Build and focused projection checks

Use a Linux/POSIX host with Python 3, a GCC-compatible C/C++ toolchain and linker
wrapping/section-GC support. Provide the original pinned Synth One source root
containing `AudioKitSynthOne/DSP/`; sixteen control-path files are byte-verified.

```sh
sh experiments/synthone-port-feasibility-20261114/parameter-ports/run.sh /path/to/SynthOneSource smoke
python3 -I experiments/synthone-port-feasibility-20261114/parameter-ports/tests/test_inputs.py /path/to/SynthOneSource
```

The input path is required, may contain spaces and may be relative to the caller.
The generator checks the committed structural maps against the pinned metadata.
It creates the test-only min/default/max header locally under ignored `build/`;
no generated value arrays are committed. No bank, waveform or audio input is used.
All verification uses explicit checks that remain enabled under optimized Python.
The build entry uses isolated Python. Untracked project files, unverified quoted
include neighbors and nonempty ambient compiler-path variables reject before
compilation. Verifier modules execute source bytes directly rather than Python's
cached-bytecode loader; the build driver hashes and compiles the same byte buffer.
Each invocation snapshots authenticated candidate, dependency and
master bytes into a fresh private stage, generates headers there and compiles only
that stage. Compiler dependency files must match each translation unit's exact
non-system input path/hash set. Object hashes are checked before linking and binary
hashes before execution. Host compiler/system headers and libraries remain trusted
toolchain inputs, outside this project-source closure.

`smoke` compiles and links the retained full test without executing it, then runs
only a new six-frame projection check for input binding, lazy state, parameter-144
lag, immediate unsmoothed output and rejected alias/half-time operations. The
binding test exercises normal Python, `-O` and `PYTHONOPTIMIZE=1`, including changed
or missing inputs and self-consistent manifest substitution.

For future changed-input qualification, `run.sh` also accepts `o2`, `os` and
`sanitize`. Those modes execute the full representation test. The mutation command
requires the source root, rebuilds its own authenticated baseline/objects and runs
that baseline successfully before accepting any runtime mutation failure:

```sh
python3 -I experiments/synthone-port-feasibility-20261114/parameter-ports/mutation_checks.py /path/to/SynthOneSource
```

It never reads an earlier `build/o2` object cache. A bounded binding preflight is
also available without running the full baseline or claiming mutation acceptance:

```sh
python3 -I experiments/synthone-port-feasibility-20261114/parameter-ports/mutation_checks.py --preflight /path/to/SynthOneSource
```

Fresh stages retain `out/COMPILE-CLOSURE.json` with the actual compiler input hashes
and linked object/binary identities. Smoke and preflight explicitly record that
the full test was not executed. The retained full oracle/mutation batches were not
replayed for the compiler-binding repair.
Generated metadata, compiler output, objects and executables remain local and
ignored. The runner performs no download or SDK/toolchain setup.

## Retained proof and open integration boundary

The accepted source-stage GCC 14.2 `-O2`, `-Os` and sanitizer runs each covered
12 cases, 6,436 ticks, 1,073,850 all-150 output comparisons and 329,314 complete
seven-field selected-state comparisons. They checked immediate initialization,
setter and restore visibility; physical min/default/max; rate and half-time
changes; aliases, lifetime, rejection atomicity and independent instances.
Compact actions made zero intercepted heap calls. Eleven mutants compiled and
linked, then failed runtime assertions. These retained suites are not replayed
just to validate this portable projection.

Measured 64-bit-host layout is 400 bytes of module state plus the existing
600-byte output array. Dense ports/targets plus outputs total 5400 bytes; adding
legacy pointers gives a 6600-byte subtotal. The workload observed 593 compact
versus 27,278 reference coefficient `pow` calls. These are component-layout and
operation counts, not target SRAM/flash/CPU savings or an engine speedup.

The first integration boundary remains source-compatible, serialized setter events
feeding the shared graph's actual `kernel.parameters` array. Tick this module once
before mono-frequency, LFO and note processing; preserve parameter-144 event timing
and keep musical resets distinct from parameter restore. Complete frontend setters,
event/mono scheduling, concurrency, engine/FX integration, target qualification and
whole-instrument acceptance remain open. License notices and original module
attribution remain in the sibling experiment files pinned by the dependency manifest.
