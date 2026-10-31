# Chicago CPU optimization

## Full-access result — 2026-10-30

All ranked steps are committed locally. The oscillator checkpoint is
`9621d6cc`; the target-safe final DSP commit is `b079b95b`.

- **Idle instructions per 256-frame block: 146,977.55 → 20,638.09
  (mean of eleven Chicago cases), 85.96% fewer.** Actual Callgrind
  totals from Docker/Colima replace the historical function-entry proxy.
- **15/15 REAL golden renders are bit-exact.** Every idle length 1–2000 blocks
  also gives identical note PCM and DSP state in normal and ASan/UBSan modes.
- **Real Elite build PASS:** app 415,792 B (+2,288 B versus the oscillator
  checkpoint), RAM/pool unchanged, pool headroom 15,184 B.
- The **12–20% AC79 device CPU target remains unmeasured**. No device was
  accessed. The earlier 1–3k instructions/block aspiration is not met by these
  measurements. The settled path has no per-sample DSP/control loop; exact
  clock/MG updates, admission, history checks and buffer writes remain.

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

## Second Builder continuation (2026-10-29)

The oscillator work was committed first by the orchestrator as
`9621d6cc7be6c333462771a30bf8e98d0701f9fd`. Its recorded source and artifact
hashes still match. Every following reference executable uses that commit's
unchanged implementations. The five public patches below supersede the older
combined continuation handoff. They preserve one local commit per ranked step.

### Exact idle advancement

The idle core admits an event-free block only with no live keys, voice,
assigned voice record or restart. The original disabled voice freezes its
oscillator, voice MG, noise RNG, gain ramps and filter. The shortcut preserves
those frozen values; advancing their RNGs would change the next note.

The two musical clocks now jump across ranges with a constant binary32 ULP
increment. Literal additions resolve tie parity and exponent boundaries;
integer arithmetic jumps only within the proven range, strictly before a
clock edge. This reproduces sequential additions, including fractional BPM,
without replacing them with `BPM * frames`. Gate expiry is advanced by the
exact number of samples. Original callbacks still run at clock/key-reset
edges. BPM and both thresholds remain stable during the admitted span.

Global MG still updates at its exact 690/Fs counter crossings, normally four
or five updates per 256-frame block. Its waveform, phase, noise and helper
counter writes retain their original order. This bounded work preserves float
rounding and sample-and-hold behavior. No approximate float smoother advance
is used. Nonsettled smoothing or signal histories run their original path;
a nonzero tail or EQ residue is never replaced by silence.

Silent Chorus now jumps its integer phase/modulator recurrence. On a segment
with constant phase direction and integer modulator delta, the rounding
condition is affine in the sample index. Endpoint bounds prove every skipped
sample, with literal updates at rounding, turning, wrapping and clipping
boundaries. The zero-history scan uses alias-safe unrolled word loads.
There are no new persistent fields, allocations or static DSP buffers.

### Separate local commits

Full-access continuation verified every source, patch and proof hash before
committing. The oscillator checkpoint remains `9621d6cc`. Public source views
were staged directly so each commit contains only its ranked step:

- Step 2: `a7fd84c9`.
- Step 3: `026f1640`.
- Step 4: `73f1d6b4`.
- Step 5: `4d1a1051`.
- Step 6: `e5985569`.

The older patches and receipts remain under `build/chicago-opt-series/`.
No private inputs or binaries are staged. The earlier sandbox refusal no
longer applies. Docker/Colima is accessible in this full-access run; the
historical unavailable-count record below will be superseded by measurements.

#### Step 2 — committed: exact idle core and clock spans

Skip disabled voice dispatch and advance exact clock/gate/global state in bounded spans. The player hears identical notes after long silence with less idle work.

Patch: `build/chicago-opt-series/02-idle-core.patch`.
Proof: `build/chicago-opt-series/02-idle-core/RESULT.json`;
golden log: `02-idle-core/golden.log`; differential receipt: `02-idle-core/idle/RESULT.json`
(relative to `build/chicago-opt-series/`).

Files:

- `experiments/original_synth/chicago_clock_span.h`
- `experiments/original_synth/chicago_global_mg.c`
- `experiments/original_synth/chicago_global_mg.h`
- `experiments/original_synth/chicago_scheduler.c`
- `experiments/original_synth/chicago_scheduler.h`
- `firmware/src/dsp/original_chicago_backend.c`
- `tests/host/chicago_clock_span_test.c`
- `tests/host/chicago_idle_then_note_test.c`
- `tests/host/run_chicago_clock_span.sh`
- `tests/host/run_chicago_idle_then_note.py`

#### Step 3 — committed: event-free block control entry

Enter control/render ownership once per event-free block. Envelope, portamento, MG, ARP notes and restart operations keep their original sample positions. This reduces dispatch cost without changing musical timing.

Patch: `build/chicago-opt-series/03-block-control.patch`.
Proof: `build/chicago-opt-series/03-block-control/RESULT.json`;
golden log: `03-block-control/golden.log`; differential receipt: `03-block-control/idle/RESULT.json`
(relative to `build/chicago-opt-series/`).

Files:

- `experiments/original_synth/chicago_voice_owner.c`
- `experiments/original_synth/chicago_voice_owner.h`
- `firmware/src/dsp/original_chicago_backend.c`

#### Step 4 — committed: exact silent Chorus advance

The existing exact Drive/output and FX blend skips remain in use. Add the bounded Chorus recurrence jump and faster zero-history proof so silent Chorus no longer runs its modulation loop once per sample. Nonzero histories and unsettled controls retain the plain path.

Patch: `build/chicago-opt-series/04-silent-chorus.patch`.
Proof: `build/chicago-opt-series/04-silent-chorus/RESULT.json`;
golden log: `04-silent-chorus/golden.log`; differential receipt: `04-silent-chorus/idle/RESULT.json`
(relative to `build/chicago-opt-series/`).

Files:

- `experiments/original_synth/chicago_chorus_span.h`
- `experiments/original_synth/chicago_modulation.c`
- `tests/host/chicago_chorus_span_test.c`
- `tests/host/run_chicago_chorus_span.sh`

#### Step 5 — committed: one serial block admission

Admit stable voice/table/control/BPM inputs once per event-free block and remove duplicate workspace/output geometry checks from whole-block rendering. Public checked voice and split-frame begin/end APIs retain validation. This reduces repeated checks while preserving invalid-argument admission and audible behavior.

Patch: `build/chicago-opt-series/05-block-validation.patch`.
Proof: `build/chicago-opt-series/05-block-validation/RESULT.json`;
golden log: `05-block-validation/golden.log`; differential receipt: `05-block-validation/idle/RESULT.json`
(relative to `build/chicago-opt-series/`).

Files:

- `experiments/original_synth/chicago_voice.c`
- `experiments/original_synth/chicago_voice.h`
- `experiments/original_synth/chicago_voice_owner.c`
- `experiments/original_synth/chicago_voice_owner.h`
- `firmware/src/dsp/original_chicago_backend.c`

#### Step 6 — committed: settled Chicago master on zero

Skip master multiplication and cleanup after the existing output proof produces exact stereo zero and master gain/mute have settled. Preserve target/current/step writes. Ramps, mute transitions and nonzero FX residue still run the original master. This leaves more idle headroom with identical sound; master.c and Miami-specific code are unchanged.

Patch: `build/chicago-opt-series/06-silent-master.patch`.
Proof: `build/chicago-opt-series/06-silent-master/RESULT.json`;
golden log: `06-silent-master/golden.log`; differential receipt: `06-silent-master/idle/RESULT.json`
(relative to `build/chicago-opt-series/`).

Files:

- `experiments/original_synth/chicago_engine_output.c`
- `tests/host/original_idle_test.c`
- `tests/host/original_idle_test.sh`
- `CHICAGO-OPT.md` (this final proof and local commit handoff)

### Proof of every checkpoint

- All five checkpoints separately pass **15/15 REAL golden renders** against
  `build/synth684-golden-9d1cf9fb`, including all four unchanged Miami cases.
- All five separately pass the normal/instrumented and ASan/UBSan
  idle-then-note test against `9621d6cc`: every silence length **1 through 2000
  blocks**, across eleven Chicago program/FX cases and all seven compact FX
  choices. Every length forks a fresh note at a varying frame offset, asserts
  audible output, then restores the untouched idle prefix at the same storage
  addresses. Every PCM sample and pointer-free DSP state snapshot matches.
- Each full comparison covers **136,400 render calls**, including the earlier
  long-silence, release/tail, mute, Drive-edit and irregular-frame cases. Both
  modes produce **1,107,861,392 identical transcript bytes**, SHA-256
  `b57b4321aa6f170d6856519b45e264fe5d961011dfb72ad64a9055166116548c`.
  Receipts record compiler flags, reference units, source views, complete input
  hashes and source stability. Optional transcript cleanup runs only after
  byte comparisons, mode comparison and source-closure verification pass.
- `sh tests/host/run_chicago_clock_span.sh`: **414,240 exact comparisons PASS**
  in normal and ASan/UBSan builds, including ties, exponent changes, fractional
  BPM, threshold neighbors and accumulator stagnation.
- `sh tests/host/run_chicago_chorus_span.sh`: **324,902 exact comparisons PASS**
  in normal and ASan/UBSan builds, including signed phase wrap, triangle turns,
  saturation, zero speed, negative speed and randomized spans.
- Final idle-owner regression: **432,418 PCM/state checks PASS**, normal and
  ASan/UBSan, PCM digest `35ca5fe82aea117f`. The test checks rejected shortcuts,
  pending events, dirty controls, nonzero histories and constant EQ residue.
- Read-only review found no blocking arithmetic, overflow, callback-order or
  admission defect. `git diff --check` passes. No Miami-specific source or
  private header changed.

Reproduce a checkpoint with:

```sh
CHICAGO_CANDIDATE_SOURCE="$PWD/build/chicago-opt-series/02-idle-core/source" \
CLEAN_TRANSCRIPTS=1 /opt/homebrew/bin/python3 \
  tests/host/run_chicago_idle_then_note.py build/chicago-opt-step2-repeat
```

Use the other source views for the other steps. The ignored `cc-view` compiler
wrapper applies the same public source view to the golden harness; its
commands and complete evidence are retained under `build/chicago-opt-series/`.
Reuse valid matching receipts rather than rerunning unchanged proven sources.

### Historical sandbox work proxy (superseded by the full-access run)

**Actual idle instructions per 256-frame block: before UNAVAILABLE; after
UNAVAILABLE.** Valgrind is not installed on macOS. Docker/Colima was attempted
again: `docker info` fails with permission denied on
`/Users/mac/.colima/default/docker.sock`, before any container can start.
The recorded `docker run --rm ubuntu:22.04 true` attempt failed for the same
reason. Installing Valgrind inside Docker and running `--icount` are therefore
impossible in this sandbox. No instruction count or CPU percentage is claimed.

The best available deterministic proxy counts all compiled C function entries
only during rendering of 704 settled idle blocks (64 per case, eleven cases,
256 frames each). Reference and candidate use the same compiler and flags;
normal instrumentation is also checked against the sanitized transcript.
Static inline helper entries count too: these are work counts, not machine
instructions, and relative results across different helpers have limits.

- Unoptimized backend: **6,461,014 / 704 = 9177.58 entries/block**.
- After 02-idle-core: **2,768,000 / 704 = 3931.82 entries/block**.
- After 03-block-control: **2,768,000 / 704 = 3931.82 entries/block**.
- After 04-silent-chorus: **2,113,280 / 704 = 3001.82 entries/block**.
- After 05-block-validation: **2,080,192 / 704 = 2954.82 entries/block**.
- After 06-silent-master: **275,840 / 704 = 391.82 entries/block**.
- Final per-case means: **378.80–406.30 entries/block**, including Chorus.
- Final versus reference: **95.7% fewer function entries**. Counted backend
  frame calls **180,224 → 0**; master process calls **704 → 0**.

The requested **1–3k instructions/block/part** and **12–20% AC79 idle CPU**
remain **UNVERIFIED**. Function entries cannot establish either threshold.
The source retains bounded global update/clock-edge work, zero-history scans
and buffer writes; it does not claim literally zero instructions.

The required `--elite --golden` harness run was attempted on the final source.
Its host renders are **15/15 identical** (recorded separately), but the Elite
build stops at missing `cpu/wl82/tools/uboot.boot` / `AC79_SDK`. Logs:
`build/chicago-opt-exact-elite.log` and
`build/chicago-opt-exact-golden-compare.log`. No new target flash/RAM/stack
result is claimed. The oscillator Elite ELF above is stale for this backend
continuation. Host GNU `size -A` failures are not target size measurements.

All work remained local. No push, plugin/firmware binary staging, installation,
flashing or device access occurred. Private inputs remain excluded. Final
public-file, patch and proof hashes are in
`build/chicago-opt-second-builder-receipt.json`.


## Full-access follow-up: omit the idle zero-buffer rescan

The backend now passes its immediately zero-filled core buffers through an
explicit serial idle-owner entry. This removes the remaining per-sample input
inspection in the settled zero output path. State and geometry admission are
unchanged; unsettled smoothing and nonzero tails still run their original
paths. The ordinary public output entry still checks arbitrary input buffers.
The player gets identical output and more idle CPU headroom. No persistent
state or RAM is added.

The focused output regression compares this entry against the ordinary entry
on signed-zero inputs, settling controls, transitions and tails. It passes
432,418 PCM/state checks in normal and ASan/UBSan modes, with PCM digest
`35ca5fe82aea117f`. All 15 REAL golden files are byte-identical. Every idle
length 1–2000 blocks across eleven program/FX cases also passes the normal and
ASan/UBSan PCM/state comparison against `9621d6cc`; both transcripts retain
SHA-256 `b57b4321aa6f170d6856519b45e264fe5d961011dfb72ad64a9055166116548c`.

Evidence: `build/chicago-opt-no-rescan-RESULT.json`,
`build/chicago-opt-fullaccess-idle-note/RESULT.json`, and
`build/chicago-opt-fullaccess-idle-owners/RESULT.json`.


## Target link repair: exact 32-bit Chorus span divisions

The first real AC79 link exposed an unavailable `__divdi3` runtime call in the
Chorus span helper. The host proof had passed, but that candidate could not
link on the device toolchain. The helper now computes the same quotient with
bounded 32-bit unsigned divisions: phase distances fit 31 bits, and rounding
room measured from the admitted first step fits 24 bits. Large 64-bit strides
are compared before narrowing. No arithmetic approximation or new DSP state
is introduced. This lets the player use the exact idle optimization in an
actual Elite build.

Proof after this repair:

- `build/chicago-opt-target-span.log`: 324,902 exact comparisons in normal and
  ASan/UBSan modes, both PASS.
- `build/chicago-opt-target-golden.log`: REAL, 15/15 golden files identical.
- `build/chicago-opt-target-idle-note/RESULT.json`: every gap 1–2000 blocks;
  normal and ASan/UBSan PCM/state transcripts both byte-identical to
  `9621d6cc`, SHA-256
  `b57b4321aa6f170d6856519b45e264fe5d961011dfb72ad64a9055166116548c`.
- `build/chicago-opt-target-idle-owners/RESULT.json`: 432,418 PCM/state checks
  in normal and ASan/UBSan modes, PASS; digest `35ca5fe82aea117f`.
- Real Elite build: PASS, including link, no soft double, RAM and code-area
  checks. App image **415,792 B**, versus oscillator checkpoint **413,504 B**
  (**+2,288 B**). `.data + .bss` **97,432 B**, pool **328,880 B**, both unchanged;
  **15,184 B** of pool headroom. No package was installed or flashed.

The local target environment needed Pillow and a toolchain include-path alias
inside its Docker container. After repair, the same run's successful objects
were retained and only failed/changed units were rebuilt. The final resume
reused 72 matching compile outputs and recompiled `chicago_modulation.c`, then
ran the ordinary link, image and build checks. Input and reused-object hashes
are recorded in `build/chicago-opt-resume-elite-RESULT.json` (PASS). The selector
was corrected to match the full source path before the final successful run.
Build log: `build/chicago-opt-target-final-build2.log`.

Final Elite ELF SHA-256:
`6e94430ea405df17f2efe6a8cb75a2079fbdaa9fe4a734ce41510e77c29a30db`.
Final app image SHA-256:
`84479f7e6fc4da8c676e0b49bcdcb62f0b8b409d7b78400f849901d9411d7fb0`.


## Actual idle instruction measurements

Measured in `x0x37-final-cache:dev-532d91f`, Linux x86-64 under Colima on the
Mac, using GCC 12.2.0 and Valgrind 3.18.1 Callgrind. Both executables use `-Os`,
the same strict floating-point flags, real pinned private inputs, 120 BPM,
44,100 Hz and 256-frame stereo blocks. Each case first runs 1024 uncounted
settling blocks, then 64 measured blocks. Idle mode runs no note script and
uses one timing pass; the default active golden mode retains its 15 passes.

Collection covers `original_runtime_render` plus about 20 client-request
boundary instructions per block. Initialization, prepare, warmup, retirement,
clock reads and transcript I/O are excluded. The same boundary overhead is
present before and after. These are host instructions, not AC79 cycles or a
measured CPU percentage. The baseline is the committed oscillator checkpoint
`9621d6cc`; the final source is `b079b95b`.

Instructions per block, baseline → final:

- `chicago-p00-fx0-idle`: **144,070 → 19,486**.
- `chicago-p00-fx1-idle`: **159,882 → 22,358**.
- `chicago-p00-fx2-idle`: **144,160 → 19,576**.
- `chicago-p00-fx3-idle`: **144,115 → 19,531**.
- `chicago-p00-fx4-idle`: **144,158 → 19,574**.
- `chicago-p00-fx5-idle`: **144,092 → 19,508**.
- `chicago-p00-fx6-idle`: **144,151 → 19,567**.
- `chicago-p07-fx0-idle`: **144,056 → 19,452**.
- `chicago-p19-fx0-idle`: **144,103 → 19,505**.
- `chicago-p33-fx0-idle`: **144,117 → 19,533**.
- `chicago-p54-fx0-idle`: **159,849 → 28,929**.

The mean is **146,977.55 → 20,638.09 instructions/block**, a **85.96%
reduction**. Final case means range from **19,452 to
28,929**. The sum across the eleven alternative scenarios is
**1,616,753 → 227,019**; that sum is not one device block.
All four unchanged Miami idle counts match exactly between executables.
All fifteen idle PCM files also match between baseline/final and `-O2`/`-Os`.

The original parser selected `summary: 0` before a nonzero final `totals`
line. It now prefers `totals` and refuses incomplete or zero count reports.
The baseline's older running process used that parser; its completed raw
Callgrind parts were reparsed and its size/count JSON was corrected from the
actual totals. No zero count is treated as a measurement. The macOS target
size reader now invokes the same Docker objdump used for the build; unsupported
host `size -A` returns an explicit error instead of fictitious zero sizes.

The final target size receipt reports Chicago **74,774 B**, shared **9,679 B**,
and Miami **21,171 B** of attributed flash. ELF symbol attribution is approximate;
the linked app image and RAM/pool totals above are the build's actual totals.

Evidence:

- `build/chicago-opt-fullaccess-RESULT.json`: source/artifact hashes, all raw
  Callgrind hashes, paired counts and proof closure (PASS).
- `build/chicago-opt-icount-before/sizes.json` and
  `build/chicago-opt-icount-final/sizes.json`: corrected/final instruction reports.
- `build/chicago-opt-icount-before-corrected.log` and
  `build/chicago-opt-icount-final.log`: readable counts.
- `build/chicago-opt-target-elite-sizes.json`: linked per-engine size receipt.
- `build/chicago-opt-identical-tu-cache.jsonl`: baseline object reuse only
  after byte-identical complete preprocessing under both header searches.
- Final measurement reused unchanged same-compiler objects after verifying
  source closure. Only `chicago_modulation.c` includes the changed Chorus span
  header; that unit was rebuilt in both host optimization modes. All reused
  objects are byte-identical to their recorded originals.

Reproduce the final idle run in the prepared local container:

```sh
docker exec chicago-opt-linux python3 tools/synth684_harness/run.py \
  --icount --idle --warmup 1024 --blocks 64 \
  --out build/chicago-opt-icount-repeat
```

`VALGRIND` and `VALGRIND_LIB` are set on that container. The prepared public
baseline source view is `build/chicago-opt-linux-reference`; its checked
compiler overlay is `build/chicago-opt-cc-view.py`. No private headers were
copied into Git. All commits are local; no push, installation, flashing or
SMK 37 Pro access occurred.


## Isolated clock-span follow-up — 2026-10-30 (chat/chicago-opt)

Purpose: reduce the remaining Chicago idle-clock CPU expense without changing
the audio or any state transition. The existing exact binary32 clock-span proof
had one unsigned division per stable binade jump. When the *entire* remaining
range is provably inside the current exponent and before the threshold, the
new path uses bounded 32-bit multiplication instead. It keeps the existing
division for all large spans, large ULP steps and boundary cases; it never
replaces sequential float32 additions with tempo times elapsed frames.

- Touched only the shared Chicago clock-span inline header, its focused host
  test and this lane record. No Miami source, private input or binary changed.
- In an isolated Linux x86_64 C99 reconstruction of the exact fetched clock
  helper and regression logic (GCC 14, strict FP, `-Os`; ASan/UBSan `-O1`):
  `sh tests/host/run_chicago_clock_span.sh build/chicago-guard` passed
  **416,330/416,330 exact comparisons** in each mode. The original 414,240
  cases plus 2,090 new guard, long-span and borrowed-negative-state checks.
- The previous measured **20,638.09 instructions/block** remains the
  baseline for this new branch, NOT a newly measured result. The Mac must
  repeat actual Callgrind, all 15 REAL golden renders and idle-then-note
  differential proof before any 1–3k, target CPU, or bit-exact whole-engine
  claim. A local focused arithmetic PASS is not hardware verification.
- No firmware builds or proprietary headers were used in this ChatGPT
  environment. A repository checkout and Git transport were unavailable to
  its Linux shell, so only the exact focused C test was run here.

Run the following from the current `chat/chicago-opt` checkout on the Mac,
with its existing private #684 input kit and JieLi environment provisioned:

```sh
sh tests/host/run_chicago_clock_span.sh build/chicago-opt-chat-clock
sh tests/host/run_chicago_chorus_span.sh build/chicago-opt-chat-chorus
sh tests/host/original_idle_test.sh build/chicago-opt-chat-owner
BASE_REF=9621d6cc7be6c333462771a30bf8e98d0701f9fd CLEAN_TRANSCRIPTS=1 python3 tests/host/run_chicago_idle_then_note.py build/chicago-opt-chat-idle-note
python3 tools/synth684_harness/run.py --golden build/synth684-golden-9d1cf9fb --out build/chicago-opt-chat-golden
docker exec chicago-opt-linux python3 tools/synth684_harness/run.py --icount --idle --warmup 1024 --blocks 64 --out build/chicago-opt-chat-icount
BOARD=smk37 MODEL=elite sh build.sh
BOARD=smk37 MODEL=pro sh build.sh
```

Do not accept a `STUB-NOT-VALID` harness report. Require 15/15 REAL PCM
matches including Miami, normal/sanitized idle-then-note byte-for-byte
state/PCM matches, positive Callgrind totals for every case, and real
Elite/Pro budget/link results. Check the exact source SHA and package sizes
before labeling anything DEV CHECKED. Nothing is authorized to flash or
publish here.
