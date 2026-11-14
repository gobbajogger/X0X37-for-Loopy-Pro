# Synth One direct-event scheduler source prototype

This cut adds the upstream direct-note event and block scheduling behaviour to the genuine six-note pre-FX graph. It keeps six fixed poly states and a separate seventh mono state, one shared PRNG/LFO context, borrowed immutable waveform/tuning data and caller-owned DSP/control state. It changes no firmware source or shared graph/control module.

The reference is AudioKit Synth One `6466a3715c96b7ecf1dd255a218cbd571408a314` with AudioKit `03fecf80a66be88f6ac0356cd532c72dbe09922d`. Existing graph input is project #934 `a0c8698f724e28ec6275307525933c195817b3c5`. `DEPENDENCIES.json` pins every compiled external dependency, the shared graph header, source metadata and the already prepared public waveform payload. The input tables retain their original float32 bits and overshoots. No supplied presets or rendered audio are included. `init` represents constructing a fresh logical instance; it does not promise to reproduce the upstream `init()` method called on an already live kernel.

## Supported boundary

Calls are serialized. Arp must be exactly zero. The caller establishes the original `sequencer.reset(true)` empty-pending-list precondition; `init` and the subset `restore` explicitly perform that precondition step after parameter restoration. `EmptySequencer` asserts that domain at each frame. This adapter does not implement the original initializer's 1024 pending zero notes, arpeggiator or sequencer.

`Scheduler::start` reproduces kernel `startNote`, including ignored explicit frequency, pitch-deduplicated held recency, current transpose and tuning lookup. `on` reproduces `turnOnKey`: its explicit-frequency overload uses that frequency, while its two-argument overload resolves current transpose/tuning. `stop` edits the held array before `turnOffKey`; `off` acts directly on voices. The fixed array preserves head-first iteration order and supports all 128 distinct pitches. It models no channel, key owner or press count.

The input domain is pitch/velocity 0 through 127, finite explicit frequency from zero through Nyquist, and finite resolved parameter inputs (clamped to the existing pinned physical ranges). Arp-on is rejected. Tuning supplies 128 finite positive frequencies no higher than Nyquist; the source lookup clamps the final transposed index. Supported sample rates are 44.1, 48 and 96 kHz. The borrowed waveform and tuning spans must remain readable and immutable throughout the instance lifetime. The actual source rate snapping, dependent-parameter routing, tuning generation and AU wrappers remain outside this API.

Invalid admitted input is rejected before scheduler state changes. Output spans must be distinct and must not overlap the instance, tuning or any table payload. `process` accepts at most 65,536 frames including offset; the caller supplies enough writable output memory. Input pointers must point to readable spans of the stated size. `init` rejects parameter input that overlaps its object; `restore` permits exact aliasing with its parameter outputs and rejects partial object overlap. Tables and tuning cannot alias instance storage. The API does not provide concurrent access or arbitrary pointer validation.

## Preserved source behaviour

- Poly allocation first finds the same root pitch, then a free root sentinel from the current cursor, then advances that cursor once to steal. Cursor survives musical resets. Retrigger/steal retain per-slot oscillator, filter and envelope history.
- `process` clears release voices below 0.01 only at block entry and only in the selected mono/poly bank. A voice crossing below the threshold mid-block stays eligible through that block. Equal-to-threshold remains occupied.
- Each frame advances compact parameter outputs, mono-frequency smoothing, raw `isMono` change reset, shared LFOs, the empty-sequencer precondition and eligible notes in ascending poly-slot order. Sparse holes never consume noise draws. Eligible notes still consume shared PRNG noise when noise volume is zero.
- Poly note-off performs the original extra two ADSR computes immediately. Zero `monoIsLegato` forces that release step on mono retrigger/fallback; nonzero does not. Mono fallback also occurs for non-head or absent valid note-offs. Its omission of the noise-amplitude update is retained.
- Fractional positive `isMono` changes reset both note banks even when both old/new values are positive. Mono smoothing runs before that reset. Current transpose/tuning apply to fallback.
- Note `clear` changes only the original metadata/gate/amp fields. Musical `reset`, `reset_dsp`, release-based `stop_all`, fresh initialization and parameter/note restoration remain separate operations. `restore` resets parameter smoothing accumulators and marks note DSP for lazy initialization while retaining source note metadata not assigned by `initializeNoteStates`.

The static initializer is checked against actual unchanged `S1NoteState::init` and its source setup field assignments. Test heap allocations are zero-filled solely to make unused initial allocation bytes deterministic, following the existing graph oracle convention. Production objects are zero-initialized.

## Reference and checks

`oracle/generate.py` verifies exact upstream byte lengths, Git blob IDs and SHA-256 before extracting eleven actual method bodies or process-prefix bodies. Its manifest records original/adapted body hashes and every exact-count Objective-C/container/output adaptation. The source process retains the full original parameter-port loop, mono port and LFO arithmetic independently from the prototype's compact port tick. The runner regenerates and compares the oracle before compiling.

Focused tests compare exact pre-FX PCM buffers, guards and canonical field bytes at block entry, after every frame and after direct events. The snapshots include every declared field of all note DSP objects, all selected port states/targets, mono/LFO state, held lists, cursor, PRNG/context and table descriptor metadata. Pointer ownership and immutable payload identity are explicit; object addresses/padding are excluded. Separate original `S1NoteState::init` calls check fresh and lazy-restored static note initialization. Event/render actions trap malloc/calloc/realloc/free. Tables and tuning are mapped read-only.

Run the changed cut with explicit input paths:

```sh
python3 run_tests.py --experiment-root /path/to/verified/experiment \
  --controls-root /path/to/verified/control-module \
  --master-root /path/to/pinned/master-source \
  --tables /path/to/verified/public-morphs.f32 --variant o2
```

### Portable control-package binding

The public consumer pins the actual four compiler files under the sibling `parameter-ports/` package. Its public `parameter_ports.hpp` is 1,836 bytes with SHA-256 `b533cc60b93da7df64aca34be3f382ff0191ffd2c4f6db6275bffcd06c665f61`; public `generated/parameter_map.hpp` is 445 bytes with SHA-256 `c7bc2f49818d4f0c202b6b1c79ea7552f8a70476ff388889636908f7421ca11e`. Their only differences from the original producer inputs are documentation comments. Both C++ implementation files and all declarations/arithmetic remain unchanged. The staging step copies these authenticated public bytes without rewriting includes or comments.

Use `--smoke` to compile this exact portable consumer boundary and execute only three short direct-event/control cases, one per supported sample rate. The generated smoke entry point reuses the unchanged fixture's canonical state/PCM and no-heap helpers, while its original 48-case entry point is renamed and never called. This checks the portable package consumption, not a new full scheduler qualification. Original 48-case/compiler/sanitizer evidence is retained under its original source/input freeze and is not reported as rerun against the new public header pins. The public runner's default full-fixture mode remains available for future changed-input work.

Use `os` or `sanitize` for the other host variants. Inputs are hashed and their verified bytes copied into a fresh compiler closure before compilation. Unlisted caller-tree headers cannot shadow the pinned headers. Ambient `CPATH`, `C_INCLUDE_PATH`, `CPLUS_INCLUDE_PATH` and `OBJC_INCLUDE_PATH` are cleared for compiler/linker calls. The exact local code is also staged and recorded in `COMPILED-SOURCES.json`; each run uses a fresh object directory and an explicit link closure. An optional `--build-root` keeps a review run in its own directory. Unchanged upstream C/DSP warnings remain visible and are not made fatal. Prototype, adapter, control and test translation units use strict warning checks.

`mutation_checks.py --build /path/to/completed/o2-run --tables /path/to/verified/public-morphs.f32` compiles synthetic mistakes against that run's exact objects. It requires each mutation to compile/link and then fail the runtime assertions, so compiler errors do not count as detected behaviour regressions.

## Limits and next stage

This is a pre-FX direct-event subset. All four global-delay reset calls in the reference are explicit no-ops in this oracle. Reset results qualify retained note/shared/control state only; they say nothing about delay, reverb, phaser, compressor or other global tails. No full-engine acoustic equivalence, full AU/event slicing, MIDI normalization, sustain wrapper, AEArray thread timing, arp sequencing, device or integrated CPU/resource acceptance is claimed.

The common prototype state also carries independent original parameter-port storage for the reference adapter. That is test scaffolding, not an accepted target memory layout. Before production integration, separate those oracle-only fields, combine the scheduler with the real graph/control/storage ownership, qualify the target table-read path, and measure exact combined native flash/RAM and worst-case workload. Preset interpretation and all global FX still need their own genuine source stages. The firmware candidate remains unchanged by this experiment.

MIT notices are retained in `notices/` and the pinned dependencies.
