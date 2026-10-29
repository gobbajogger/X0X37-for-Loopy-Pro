# Chicago CPU optimization

## Scope and method

Work starts at `40935bd153d86764913c5a317fbc4f10ddf2b600` on
`codex/chicago-opt`. Reduce Chicago render cost on AC79 while preserving every
sample against `build/synth684-golden-9d1cf9fb`. Small flash increases are
acceptable. Keep Miami-specific source unchanged. Commit locally only; do not
push, install, or flash. Private inputs and plugin/firmware binaries stay out
of Git.

For each candidate: inspect the affected callers and failure behavior, compare
all 15 golden renders, run focused arithmetic regressions, measure Callgrind
instructions and linked Elite flash/RAM, then record the result before committing.
Reject changes that alter samples. The player should hear identical audio with
more CPU headroom.

## Baseline

- Starting Chicago count supplied with the task: **5,320,215** (sum of the
  per-block instruction counts for 11 Chicago scenarios at `40935bd1`), versus
  **5,434,906** at `9a90cd58`.
- Local host baseline: REAL inputs, manifest
  `9c7ac9b0a71e775fb7009e2d4035fce827b2c56c4e8e692a7a8910c9c6c881bc`;
  **15/15 renders identical**. Command:
  `PATH=/opt/homebrew/bin:$PATH python3 tools/synth684_harness/run.py --golden build/synth684-golden-9d1cf9fb --out build/chicago-opt-mac-before`.
- Environment: Apple silicon macOS. System Python is 3.9; use Homebrew Python.
  Local baseline log: `build/chicago-opt-mac-before.log`.
- Initial Docker context points to a broken Colima VM. The Mac `size` utility
  does not support the harness's GNU `size -A` parser, so its zero object-size
  totals are **not measurements**.

## Change 1: bounded oscillator phase-step conversion

Replace the audio-rate general bit-level `miami_fx_fcvtzs` call with a native
truncating cast, guarding the positive saturation endpoint first. Accepted
immutable controls keep all waveform intermediates finite; the existing clamp
bounds the converted step to `[-2^31, 2^31]`. The positive endpoint becomes
`INT32_MAX`, and all other values have a defined C conversion identical to the
reference. Arithmetic order, SYNC, phase updates, gain ramps and checked API
failure behavior stay the same. The purpose is to reduce work in every active
oscillator pair and leave more CPU headroom without changing the sound;
instruction-count qualification is still pending.

- Chicago instructions before: **5,320,215**, supplied baseline; after:
  **NOT RUN**. Valgrind is absent locally and in the available Linux VM.
  Fetching its official Ubuntu package failed with DNS resolution failure.
  Do not infer a measured instruction reduction from the native conversion.
- Real Elite build, pinned JieLi Clang 4.0.1, `-mcpu=r3 -mfprev1`, `-Os`, soft
  FMA, FP contraction off: **PASS** before and after. An isolated copy of the
  existing Linux disk runs under QEMU TCG with no network; the existing Colima
  disk and configuration are not modified. Local source/input directories are
  shared into that VM; compiler and SDK shares are read-only.
- Elite flash attribution from `elite_sizes.py`: Chicago **72,468 → 72,468 B**;
  Miami **21,171 → 21,171 B**; shared **9,659 → 9,679 B (+20 B)**. Whole app
  image: **413,488 → 413,504 B (+16 B)** after alignment.
- Whole app RAM `.data + .bss`: **97,432 → 97,432 B (0 B)**; pool:
  **328,880 → 328,880 B (0 B)**, leaving **15,184 B** free. No state fields or
  dynamic allocations were added.
- `sh tests/host/run_oscillator_phase_step.sh`: **2,026,782 exact comparisons
  PASS**, both optimized and ASan/UBSan/float-cast-overflow builds. Includes
  signed zero, subnormals, normal/exponent boundaries, both endpoints and
  randomized legal inputs.
- Golden harness after: **REAL, 15/15 identical**, including all 11 Chicago
  scenarios and four Miami regressions. Log: `build/chicago-opt-phase.log`.
- `original_sound_cell_test.sh`: **PASS**, normal/sanitized and native/soft
  FMA variants. Independent read-only review found no arithmetic-domain,
  boundary or state-write blocker.
- Local target logs: `build/chicago-opt-{before,phase}-elite.log`; size receipts:
  `build/chicago-opt-{before,phase}-elite-sizes.json`. Generated packages stay
  under ignored `build/`; none are installed, flashed or staged.

## Change 2: binary32 oscillator gain validation

Use unsigned binary32 ordering to validate each current oscillator gain instead
of separate finite/range floating checks. Accept negative zero explicitly.
Keep the same public checked API and validate every gain before rendering or
writing state. The purpose is to reduce repeated audio-rate validation work,
with identical sound and invalid-input behavior. Only shared `oscillator_motion.c` changes;
no Miami-specific source is edited.

- Chicago instruction counts before/after this change: **NOT RUN / NOT RUN**;
  same missing Valgrind dependency as change 1. The only measured total supplied
  for this work remains the starting **5,320,215**.
- Real Elite build: **PASS**. Shared flash **9,679 → 9,679 B (0 B)**; Chicago
  **72,468 → 72,468 B**; Miami **21,171 → 21,171 B**. Whole app image
  **413,504 → 413,504 B**. RAM `.data + .bss` **97,432 → 97,432 B**; pool
  **328,880 → 328,880 B**. Cumulative app flash delta versus baseline: **+16 B**;
  cumulative RAM delta: **0 B**.
- Compiler assembly confirms inlined integer gain comparisons and a native
  `ftoi(...)(trunc)` for change 1. The remaining general converter call is in
  the unchanged pitch lookup. No claim is made about target FP exception flags.
- Golden harness: **REAL, 15/15 identical** at both `-O2` and `-Os` on the Mac.
  `-Os` used an ignored local CC wrapper replacing the harness's `-O2` flag.
  Logs: `build/chicago-opt-gain.log`, `build/chicago-opt-final-os.log`.
- `sh tests/host/run_oscillator_gain_validation.sh`: **1,049,154 predicate
  comparisons PASS** in normal and ASan/UBSan builds, plus per-slot malformed
  gain rejection without state/output writes. Covers every exponent,
  significand edges, signed zero, max-gain neighbors, infinities and NaNs.
  Independent review found no blocker.
- Local target log: `build/chicago-opt-gain-elite.log`; size receipt:
  `build/chicago-opt-gain-elite-sizes.json`; compiler assembly:
  `build/chicago-opt-gain-oscillator.s`.
- Final Linux `original_idle_test.sh`: **PASS**, **432,446** PCM/state checks,
  identical normal/sanitized logs, PCM digest `35ca5fe82aea117f`, all **152**
  input paths unchanged during the run. Receipt:
  `build/chicago-opt-final-idle/RESULT.json`.
- Both focused oscillator regressions also **PASS** on Linux GCC in normal
  and sanitized builds. Logs:
  `build/chicago-opt-final-linux-{phase,gain}.log`.

## Exact final artifacts

Final `oscillator_motion.c` SHA-256:
`90237efd3e69aad5cd3da729a3b0d6a06a6ef9ee4a849c5e20c33aef5e217eee`.
Final Elite ELF SHA-256:
`5f2d2b3cccbd8e25bb46260e369e75353a90c443f4d358ea235367063de455ef`.
Final app image SHA-256:
`ecee470c73426bc3a75a6d26454d5352c7191e85e25071edf82d67c8e6e919f0`.
Baseline Elite ELF SHA-256:
`99c0235a0de06df1eb1c9316afbf585192f336199ca3a18d154501e696b49898`.
Full local digest receipt: `build/chicago-opt-sha256.json`.

## Integration checkpoint

Primary owns implementation and local commits; a read-only helper reviews
arithmetic and affected callers. No remote or hardware stage is authorized.
Both changes are bit-exact and have real target flash/RAM proof. Final Linux
host regressions pass. Instruction-count qualification remains pending
Valgrind. Reproduce counts at baseline `40935bd1`, change 1 snapshot
`build/chicago-opt-phase-src`, and the final tree with:
`python3 tools/synth684_harness/run.py --elite --icount --golden build/synth684-golden-9d1cf9fb`.
Do not claim a measured CPU reduction until those counts exist.

Local Git staging/commit was attempted after checking `git status`. The sandbox
rejected creation of
`/Users/mac/Documents/x0x37/x0x37-for-loopy-pro/.git/worktrees/chicago-opt/index.lock`
with `Operation not permitted`. No files were staged and no commit was made.
Do not route around that restriction. Pending actions: obtain instruction
counts and create local commits when the Git metadata is writable. Never push
or install/flash these artifacts.

## Local continuation: oscillator checkpoint — READY TO COMMIT

The source hash still matches the recorded 15/15 golden and target proof.
A fresh `git status --short` followed by explicit staging was rejected at
`index.lock` by the sandbox on 2026-10-29. Nothing was staged. Commit this
finished oscillator checkpoint first, using only:

- `experiments/original_synth/oscillator_motion.c`
- `tests/host/oscillator_phase_step_test.c`
- `tests/host/oscillator_gain_validation_test.c`
- `tests/host/run_oscillator_phase_step.sh`
- `tests/host/run_oscillator_gain_validation.sh`
- `CHICAGO-OPT.md` (oscillator proof sections)

Colima Docker was attempted again: `docker info` cannot connect because the
sandbox denies access to `/Users/mac/.colima/default/docker.sock`. This is a
host permission failure, not a harness or audio failure. No Docker/Valgrind
instruction measurement is available from this attempt.

## Continuation 02-idle-core

Skip disabled voice dispatch and preserve exact clock and global MG updates so notes after silence keep identical timing and sound with less idle CPU work.

Proof: `build/chicago-opt-series/02-idle-core/RESULT.json` (source hashes match, 15/15 golden renders; normal and ASan/UBSan idle-then-note comparison for every length 1–2000 blocks PASS).

## Continuation 03-block-control

Remove repeated control dispatch while preserving sample-position envelope, portamento, MG and ARP operations, leaving more CPU headroom without changing sound.

Proof: `build/chicago-opt-series/03-block-control/RESULT.json` (source hashes match, 15/15 golden renders; normal and ASan/UBSan idle-then-note comparison for every length 1–2000 blocks PASS).
