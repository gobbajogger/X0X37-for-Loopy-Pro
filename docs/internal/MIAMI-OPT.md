# Miami CPU optimization

The inherited optimization at `f95e230e` is bit-exact in the checks below.
Its AC79 Elite build passes. The earlier tooling restrictions and READY TO
COMMIT notes below are historical; the full-access continuation records
local commits and Docker/Valgrind qualification at the end of this file.
The purpose is to reduce the work per audio block while preserving the sound
and voice/FX state heard when playing Miami.

## Source and measurement scope

- Baseline: `40935bd153d86764913c5a317fbc4f10ddf2b600`.
- Optimized engine source: `f95e230ed04c08027946a073e4ccb51f86fc48c8`.
- Golden: `build/synth684-golden-9d1cf9fb`, 15 stereo render files, 172 blocks
  of 256 frames each, including all 11 Chicago scenarios.
- Pinned external inputs: all 12 pass the existing manifest check; active
  renders use real inputs. No stub result is used as evidence.
- No shared engine source or Chicago-specific source changes in this lane.
  The finishing change repairs the Miami host test driver only.

## Changes and costs

### 1. Exact Miami nonlinear interpolation

`miami_interpolation.h` replaces the general software FMA only for a guarded
positive-normal base, smaller negative-normal difference, and fraction in
`[0,1)`. An integer significand product and explicit ties-to-even rounding
preserve the fused result, including the lower binade boundary. Other inputs
retain the complete FMA implementation. The helper is selected only by
Miami's nonlinear lookup when `ORIGINAL_USE_SOFT_FMA` is enabled.

This avoids general FMA alignment and rounding work on the filter sample
path. There are no new coefficient tables, approximations, or state fields.

Instruction counts before, for Miami programs 00 / 09 / 18 / 29, are the
owner-supplied baseline: **605,301 / 584,573 / 634,560 / 628,582** instructions
per block (sum 2,453,016). Counts after this isolated change: **not run**.
These baseline counts have not been independently reproduced in this session.

AC79 Elite linked `.text`: **409,760 → 410,032 bytes (+272)**. The app image
is **413,488 → 413,760 bytes (+272)**. RAM change: **0 bytes** in every
allocated RAM section. The harness's approximate Miami symbol attribution
is 21,171 → 21,453 flash bytes.

### 2. Settled Miami FX2 bypass

`original_miami_fx_bypass_process()` skips the general integer mixer when
FX2 is clean and its nonnegative gain state has settled at either endpoint.
After FX1, every dry sample is in `[-32767,32767]`; the remaining endpoint
product is exactly `dry - (dry > 0)`. The helper preserves scratch samples
and the fractional gain-state remainder. Dirty, ramping, and other states
retain the checked wrapper. Both Miami output entry points use this helper.

This reduces bypass mixing work without changing the bypass's integer
rounding or its state, so existing presets retain their sound.

Instruction counts before this change: **not run** for the interpolation-only
intermediate tree. Counts after both changes: **not run**.

AC79 Elite linked `.text`: **410,032 → 410,128 bytes (+96)**. The app image
is **413,760 → 413,856 bytes (+96)**. RAM change: **0 bytes** in every
allocated RAM section. Approximate Miami symbol attribution is
21,453 → 21,545 flash bytes. Combined exact app/`.text` cost: **+368 bytes**.

### 3. Finish the host proof driver

Reference export now copies regular files and directories explicitly. It
rejects links and paths outside the fresh export tree, and works on the Mac's
default Python 3.9 as well as Python 3.12. A standalone invocation now runs
the all-program PCM/state workload instead of stopping after compilation.
This makes the regression proof usable when maintaining the audio paths.
It changes no engine code, instruction count, flash, or RAM.

## Local proof, 2026-10-29

- WIP: `python3.12 tools/synth684_harness/run.py --golden
  build/synth684-golden-9d1cf9fb --out build/wip-host`: **15/15 identical**
  on Apple arm64 Clang 21.
- WIP: `python3 tools/synth684_harness/run.py --elite --golden
  build/synth684-golden-9d1cf9fb --out build/wip-linux`: **15/15 identical**,
  real inputs, AC79 Elite build successful. This ran inside an offline x86-64
  Linux VM, with `JIELI_TOOLCHAIN=/toolchain`, `AC79_SDK=/sdk`, and
  `X0X_DEV_PADCAL=1`. The toolchain and SDK mounts were read-only.
- Baseline: the same Linux/Elite harness in `build/miami-baseline`, exported
  from `40935bd1`: **15/15 identical**, target build successful.
- Interpolation-only: the same Linux/Elite harness in
  `build/miami-interpolation-only`: **15/15 identical**, target build successful.
  Linux host compiler: Ubuntu GCC 13.3.0; target compiler: JieLi Clang 4.0.1.
- `python3 tests/host/miami_cpu_test.py --out build/miami-finished-proof
  --compare-ref 40935bd1`: **PASS** on the default Python 3.9. All 30 factory
  programs match the baseline byte for byte, including PCM, workspace,
  pointer-free state, irregular block lengths, note/macro/mute events,
  damper, tempo, factory reload, panic, and transport operations.
- The same source comparison also passes with `--sanitize` (ASan/UBSan)
  and separately with `--native-fma`, using Python 3.12. Engine source did
  not change after those runs.
- Each unit-test run passes **902,199** exact interpolation/nonlinear checks
  against the 18-limb FMA oracle, and **3,968** bypass comparisons including
  every admitted signed dry value, state, scratch, endpoints, ramps, and
  zero-length calls.
- Standalone mode: `python3 tests/host/miami_cpu_test.py --out
  build/miami-finished-standalone`: **PASS**, including the all-program trace.
- All baseline/candidate comparison traces: 31,020,360 bytes; SHA-256
  `7f40bab46929807ab98de998f6ac151513759b6f6e5b74506c0d5a97dc985dd8`.

Across all three target builds, `.data` is 404 bytes, `.bss` is 96,984 bytes,
`.pool` is 328,880 bytes, `.ram_sink` is 1,024 bytes, `.ram_text` is 2,712 bytes,
`.c1_text` is 608 bytes, `.noinit` is 308 bytes, and `.ble_ram` is 3,880 bytes.
Chicago symbol flash is 72,468 bytes and shared symbol flash is 9,659 bytes
throughout. The exact allocated-section and image hashes are recorded locally
in `build/miami-size-proof.json`; the per-engine split in each harness
`sizes.json` is approximate because of static symbol ownership.

Logs, traces, target outputs, and source exports remain ignored under `build/`.
The target ELF was built for size/compile proof; no target runtime cycle
measurement or device test is claimed.

## Instruction-count follow-up

Valgrind is absent from the offline Linux guest and cached images. The host
sandbox rejects the Docker socket and SSH to the existing Colima instance;
its shell cannot resolve the package mirror. A private offline VM allowed
target compilation and Linux render proof, but supplied no Valgrind binary.
No timing result is used as a substitute for instruction counts.

On a qualified Linux runner, run the existing harness for the baseline,
interpolation-only tree, and final engine tree, each with:

```sh
python3 tools/synth684_harness/run.py --elite --icount \
  --golden build/synth684-golden-9d1cf9fb --out build/miami-measured
```

Use the same compiler and flags for all three. Reproduce the baseline before
comparing with the supplied counts; retain `sizes.json` and Callgrind files.
The interpolation-only export is `f95e230e` with `miami_fx_chain.c`,
`miami_fx_chain.h`, and `miami_output.c` restored from `40935bd1`. Its ignored
local source is `build/miami-interpolation-only`.

That session performed no push, firmware install, or flashing. Its prepared
changes contained only this note and `tests/host/miami_cpu_test.py`; no private
input header or plugin/package binary was staged. Its local commit was pending:
`git add` was denied when Git tried to create the worktree's `index.lock` at
`/Users/mac/Documents/x0x37/x0x37-for-loopy-pro/.git/worktrees/miami-opt2/index.lock`.
No new commit was created in that session. The orchestrator subsequently
committed those two files as `f9ffd12e`.

## Miami idle/block follow-up, 2026-10-29

This work continues `codex/miami-opt2` from `f9ffd12e` and keeps `f95e230e`.
The goal is lower idle load on the 360 MHz AC79, with identical sound when
playing again. No Chicago source changes, push, installation or flashing are
part of this work.

### Step 1: exactly inactive blocks

The backend admits only event-free blocks with both voices inactive, no
pending restart, no held or physical keys, no active allocator records, ARP
disabled, no gate countdown/key reset, and zero held Decimator samples. This
uses exact state predicates, with no amplitude threshold or tail truncation.
Inactive voice histories remain frozen just as in the scalar path. Clocks
retain **each** binary32 addition, including fractional BPM. At clock
thresholds the original scheduler performs the notifications in order. The
original global helper still runs at each due frame; its intervening integer
counter increments and diagnostic entry count are preserved. FX, workspace,
Decimator phase/pending commits and master state use the existing output path.

Proof against the inherited `f9ffd12e` engine:

- `python3 tests/host/miami_cpu_test.py --out build/miami-idle-step1
  --compare-ref f9ffd12e`: PASS for the original all-30-program PCM/state
  trace and the new all-30-program long-idle/note trace. The new workload
  covers cold silence, silence after release/FX history, panic, explicit
  preparation, interior/frame-zero note events, irregular block sizes, and
  tempos 20/120/217.5/400 BPM. Every output sample is compared; periodic full
  pointer-free state/workspace snapshots catch drift before note key-sync.
- Long-idle trace: 152,416,080 bytes, SHA-256
  `401cc4104a90e550ec36372a722248829b4afc620b3fb18d769c7ed1851b5e62`.
  Original trace remains 31,020,360 bytes with SHA-256
  `7f40bab46929807ab98de998f6ac151513759b6f6e5b74506c0d5a97dc985dd8`.
- `python3 tools/synth684_harness/run.py --elite --golden
  build/synth684-golden-9d1cf9fb --out build/miami-idle-step1-linux`:
  **15/15 identical**, REAL inputs, Elite build PASS in an isolated x86-64
  Linux VM, GCC 13.3 / JieLi Clang 4.0.1, `X0X_DEV_PADCAL=1`.
- Python 3.12 host golden run: **15/15 identical**. The default Mac Python
  3.9 cannot import the golden harness's existing `rom_guard.py` annotations;
  no source workaround was used. The Miami host test above runs on Python 3.9.
- Source snapshot and hashes: `build/miami-idle-step1-source/`.

Idle CPU instructions per 256-frame block, before and after: **NOT MEASURED**.
`docker info` is denied access to the Colima socket by the sandbox. A separate
VM from a copy of the local Linux image can compile and render, but contains
no Valgrind. `apt-get install valgrind` failed with Ubuntu mirror DNS errors
(`build/miami-idle-valgrind-install.log`). No hardware CPU percentage is claimed.

The deterministic fallback is Clang's executed coverage-edge count, using
`-Os -fsanitize-coverage=trace-pc-guard,no-prune`, instrumenting the same host
engine sources in both builds. `--idle-proxy --compare-ref f9ffd12e` records
only render calls after 64 warmup blocks, then 1,024 idle blocks at 217.5 BPM
for each of all 30 programs. Apple Clang 21 mean edges per block:
**32,986.198 before → 5,722.506 after (82.65% fewer)**. This measures compiler
instrumented execution paths, not retired instructions, cycles or device load.
Raw per-program counts and compiler flags are in
`build/miami-idle-step1-proxy/`. The 12–20% device-load target remains unverified.

Step 1 file list: `firmware/src/dsp/original_miami_backend.c`,
`firmware/src/dsp/original_miami_backend.h`, `tests/host/miami_cpu_test.c`,
`tests/host/miami_cpu_test.py`, and `MIAMI-OPT.md`.

**Step 1: READY TO COMMIT.** The explicit-file `git add` failed creating the
worktree `index.lock` (`Operation not permitted`). No files were staged and no
commit was created. The source snapshot above preserves this verified step
for the orchestrator; later steps extend the working tree.

### Step 2: event-free control spans — READY TO COMMIT

Event-free, two-voice, ARP-off blocks now enter one block control dispatcher.
It calls the original control graph at the first frame and at every voice or
global 690 Hz boundary, and advances the intervening integer counters without
re-entering that graph. The scheduler still runs before each frame. Events
and ARP-enabled blocks retain the scalar dispatcher. This removes redundant
control calls while preserving the envelope and modulation movement heard
within a block. A literal single control **tick** per block is not exact and
is deliberately not used. The idle guard also now requires the exact selected
two-voice capacity before bypassing any render checks.

- `build/miami-block-step2/`: original and long-idle/note all-30-program
  PCM/state comparisons PASS against `f9ffd12e`, with the same two trace hashes
  recorded for Step 1. Interpolation/bypass oracle unit checks PASS.
- `python3 tools/synth684_harness/run.py --elite --golden
  build/synth684-golden-9d1cf9fb --out build/miami-block-step2-linux`:
  **15/15 identical**, REAL inputs, Elite build PASS in the same Linux VM.
- Idle instructions/block before and after: **NOT MEASURED**, for the tooling
  reasons above. `build/miami-block-step2-proxy/` confirms unchanged mean idle
  proxy work: **5,722.506 edges/block**, versus inherited **32,986.198**.
- Exact linked `.text`: inherited 410,128 bytes → Step 1 410,656 → Step 2
  410,976. Every allocated RAM section is unchanged. Source snapshots, patches,
  hashes, target ELF copies and section sizes are in
  `build/miami-idle-step1-source/` and `build/miami-block-step2-source/`.

Incremental Step 2 file list: `firmware/src/dsp/original_miami_backend.c` and
`MIAMI-OPT.md`. Commit remains pending because of the recorded `index.lock`
sandbox denial. No private input or plugin/package binary is in that file list.

### Step 3: validate at block entry — READY TO COMMIT

Event-free blocks validate voice/control/table readiness and BPM once per
voice at entry, then use internal prevalidated control/render seams. The
serial backend keeps those inputs immutable until block exit. Dynamic pending
restart checks and downstream DSP checks remain in the sample path, and all
existing public note/event/scalar APIs retain their checked entry points.
This removes repeated immutable checks without changing the sound or the
time of a control update.

- `python3 tests/host/miami_cpu_test.py --out build/miami-valid-step3
  --compare-ref f9ffd12e`: both all-30-program PCM/state traces PASS.
  Separate `--sanitize` and `--native-fma` runs also PASS, including the
  long-idle/note workload and the interpolation/bypass unit checks. All three
  retain the two exact trace hashes recorded for Step 1.
- `python3 tools/synth684_harness/run.py --elite --golden
  build/synth684-golden-9d1cf9fb --out build/miami-valid-step3-linux`:
  **15/15 identical**, REAL inputs, Elite build PASS in the same Linux VM.
- Idle instructions per block before and after: **NOT MEASURED**. Docker/
  Colima access and Valgrind installation failed as recorded above. Final
  deterministic Clang edge proxy: inherited **32,986.198 → 5,718.506 per
  256-frame block (82.66% fewer)**, using the same compiler, flags and workload.
  Logs and all 30 per-program counts: `build/miami-valid-step3-proxy/`.
  These counts do not prove the requested **12–20% AC79 CPU load**. That
  qualification remains pending; no connected-device runtime is measured.
- Final linked `.text`: **411,312 bytes**, +336 from Step 2 and +1,184 from
  inherited `f9ffd12e`. All allocated RAM section sizes remain unchanged.
  Chicago/shared symbol flash remains 72,468/9,659 bytes. Approximate Miami
  attribution is 22,731 flash bytes. Final target ELF SHA-256:
  `227a632b8c66fe518475ded8608d85466570aeb040667c12405fd0ca31c08db9`.

Incremental Step 3 file list:

- `firmware/src/dsp/original_miami_backend.c`
- `experiments/original_synth/miami_selected_voice.c`
- `experiments/original_synth/miami_selected_voice.h`
- `experiments/original_synth/miami_voice_owner.c`
- `experiments/original_synth/miami_voice_owner.h`
- `MIAMI-OPT.md`

Commit remains pending because of the same `index.lock` sandbox denial.
`build/miami-valid-step3-source/` saves the final public source overlay,
cumulative patch, source hashes, target ELF and section sizes. For separate
commits, apply only each step's listed source/docs files from its snapshot,
in order on `f9ffd12e`. Do not stage the ignored snapshot directories or their
binary artifacts. `build/miami-final-proof.json` binds the final source hashes,
golden hashes, trace hashes and all three successful Elite/golden records.

Final scope/whitespace and pinned-input checks PASS. `f95e230e` remains an
ancestor. No Chicago-specific file was edited. The index is empty: no private
headers, plugin binaries or firmware packages were staged. No push, firmware
installation or flashing occurred. The temporary Linux VM was powered off.

## Second builder: exact idle batching, 2026-10-29

This continuation preserves `f95e230e` and the three verified overlays above.
The remaining per-frame idle clock/output work is now removed. The reference
for every new comparison is the unoptimized idle path at `f9ffd12e`.

### Step 4: batch clocks and skip settled zero output — READY TO COMMIT

`miami_idle_clock.h` advances repeated binary32 clock additions by integer
significand strides within a binade. It takes scalar additions at exponent
and tempo thresholds; equal successive strides handle ties-to-even. It never
replaces a rounded float recurrence with `BPM * frames`. The backend visits
only the actual global 690 Hz boundaries and tempo notifications. The original
scheduler handles each notification, preserving ARP/selector ordering.

Inactive voices freeze oscillator, filter, noise, LFO and ramp histories in
the reference; the idle shortcut freezes those same values. Selected global
LFO shapes 0–3 do not advance their noise seeds. Disabled ARP freezes its RNG.
No held-key, pending-restart, gate-countdown or nonzero Decimator-held state
is admitted. Active/releasing voices and ARP-enabled blocks keep their prior
paths. There is no silence threshold or tail truncation.

The backend now passes its known-zero input directly to a Miami-only output
seam. Settled FX1/FX2 mixers, conversion, and output scanning disappear;
Decimator phase and pending step advance with its existing integer formula.
Workspace/output buffers still get their exact zeros. A settled master skips
sample multiplication while retaining its exact target/current/step fields.
Dirty or moving fixed FX and moving/mute-transition master state use the
original scalar output path. No moving float smoother uses a closed form.

New proof expands the prior workload: for all 30 factory programs it probes
a note after **every integer idle duration 1 through 2000 blocks**, using
full-precision fractional BPMs near 20/120/217.12346/400. Each probe retains
the raw next 512 stereo samples for direct byte comparison, then restores
the saved engine/workspace to continue the uninterrupted idle timeline.
Periodic pre-note state snapshots catch drift that key-sync could hide.
The prior long-idle/release/panic/irregular-block workload is retained.

- `build/miami-zero-step4-complete/`: all three PCM/workspace/state traces
  byte-identical to `f9ffd12e`. Original and long-idle hashes remain those
  recorded above. New exhaustive-range trace: 377,165,880 bytes, SHA-256
  `eb851def16340159318879c9658d9411417824f1bec3eb96fe044d0617fb54d6`.
- `tests/host/miami_idle_clock_test.c`: **1,012,366** comparisons against
  literal scalar additions, including all finite binade boundaries,
  halfway/tie parity, arbitrary binary32 BPMs, threshold hits and lengths
  0–256. Existing 902,199 interpolation and 3,968 bypass checks also PASS.
- Isolated frozen Step 4 export: `python3 tools/synth684_harness/run.py
  --elite --golden build/synth684-golden-9d1cf9fb --out
  build/miami-zero-step4-linux`: **15/15 identical**, REAL inputs,
  Elite build PASS. Log: `build/miami-zero-step4-full-linux.log`;
  export/artifacts: `build/miami-zero-step4-full/`. Same GCC 13.3/JieLi
  Clang 4.0.1 toolchain as above, with `X0X_DEV_PADCAL=1`.
- Linked `.text`: **411,312 → 411,904 bytes (+592)** from Step 3.
  All allocated RAM section sizes remain unchanged. Chicago/shared symbol
  flash remains 72,468/9,659 bytes.

Idle **instructions per 256-frame block before/after: NOT MEASURED**.
Docker/Colima's socket again rejects access. A separate isolated QEMU Linux
guest has no Valgrind; a fresh `apt-get install valgrind` also fails resolving
Ubuntu mirrors (`build/miami-second-builder-valgrind.log`). The deterministic
Clang edge proxy, same flags/workload as above, is **32,986.198 before →
998.134 after (96.97% fewer)**; immediately before Step 4 it was 5,718.506.
Raw counts: `build/miami-zero-step4-proxy/`. Edges are not instructions,
cycles, or an AC79 CPU percentage; the 1–3k instruction and 12–20% load
targets remain unqualified.

Incremental Step 4 file list:

- `experiments/original_synth/miami_idle_clock.h` (new)
- `experiments/original_synth/miami_output.c`
- `experiments/original_synth/miami_output.h`
- `firmware/src/dsp/original_miami_backend.c`
- `tests/host/miami_idle_clock_test.c` (new)
- `tests/host/miami_cpu_test.c`
- `tests/host/miami_cpu_test.py`
- `MIAMI-OPT.md`

Public source/hashes are saved in `build/miami-zero-step4-source/`; apply only
these incremental files after Steps 1–3. The explicit `git add -- MIAMI-OPT.md`
again fails creating the worktree `index.lock` with `Operation not permitted`.
No commit or staging occurred. The orchestrator can commit each saved step
in order without private headers or ignored build artifacts.

### Step 5: exact idle global LFO ticks — READY TO COMMIT

The Miami idle path now evaluates its selected synchronized LFO waveforms
without the general software FMA. The guard admits zero or phase magnitudes
from `2*FLT_MIN` through 1, where multiplication by 1/2 is exact. The clock
ratio is in [0,1], where multiplication by 2 is exact. The subsequent add or
subtract therefore has the identical single rounding as the original FMA.
Tiny/invalid phases or clocks outside that domain use the original global
helper before any mutation. Old-phase waveform evaluation, new-phase order,
wrapped/held/noise fields, counter remainder and helper-entry count survive
unchanged. No Chicago source or shared DSP implementation was edited.

- `python3 tests/host/miami_cpu_test.py --out build/miami-global-step5
  --compare-ref f9ffd12e`: **PASS**, including every idle length 1–2000,
  all 30 programs, both older PCM/state workloads and all unit oracles.
- Separate Python 3.12 `--sanitize` (ASan/UBSan) and `--native-fma` runs:
  **PASS** in `build/miami-global-step5-sanitized/` and
  `build/miami-global-step5-native/`. All three runs retain exactly the same
  three trace sizes/hashes as Step 4. No golden PCM changed.
- `python3 tools/synth684_harness/run.py --elite --golden
  build/synth684-golden-9d1cf9fb --out build/miami-global-step5-linux`:
  **15/15 identical**, REAL inputs, Elite build PASS in the isolated Linux
  guest. Log: `build/miami-global-step5-linux.log`.
- Independent read-only review found no correctness defect in the clock,
  output/master or global-LFO guards and ordering.
- Final `.text`: **412,208 bytes**, +304 from Step 4, +2,080 from inherited
  `f9ffd12e`. Every allocated RAM section is unchanged. Final target ELF
  SHA-256: `36655661d47c5cd5ca873ba85c6fcd9193effe12184465c9b8e09594b30dfd9c`.

Idle **instructions per 256-frame block before/after: NOT MEASURED**, for
the Docker/Valgrind restrictions recorded in Step 4. Final deterministic
Clang proxy: inherited **32,986.198 → 573.669 edges/block (98.26% fewer)**;
Step 4 was 998.134. All 30 final program means range from 569.459 to 585.019.
Raw data and exact compile commands: `build/miami-global-step5-proxy/`.
This is substantial host execution-path reduction, but **does not prove
1–3k instructions/block or 12–20% AC79 CPU load**. Those measurements require
a qualified instruction-count runner and, for CPU percentage, device testing
in another authorized session. No connected-device measurement is claimed.

Incremental Step 5 file list:

- `firmware/src/dsp/original_miami_backend.c`
- `experiments/original_synth/miami_output.h` (clarifies the private seam's
  already-enforced nonnull/nonzero call contract)
- `MIAMI-OPT.md`

`build/miami-global-step5-source/` saves the final public source overlay,
source hashes, cumulative patch, target ELF and section sizes.
`build/miami-second-builder-proof.json` binds both new golden runs, all final
host comparison records, proxy counts, source hashes and artifact identities.
Apply Steps 4 and 5 after the earlier three READY TO COMMIT overlays; each
step's snapshot contains its own documentation version. Commit remains
blocked by the same `index.lock` sandbox denial. No private header, plugin,
firmware package or controller image was staged. There was no push, firmware
installation or flashing; the connected SMK 37 Pro was not accessed.

## Full-access continuation, 2026-10-30

The saved source overlays and their SHA-256 records matched before committing.
All five steps now have separate local commits on `codex/miami-opt2`:

- Step 1: `b2d7accc`, exactly inactive voice blocks.
- Step 2: `7076218b`, event-free control spans.
- Step 3: `0b18f5d3`, immutable voice inputs validated at block entry.
- Step 4: `5b133035`, exact clock batching and settled zero output.
- Step 5: `d706977e`, exact selected global LFO arithmetic.

Only documentation EOF whitespace was normalized while committing snapshots.
The final source, host-proof records, golden logs/reports and all 15 golden
render hashes match `build/miami-second-builder-proof.json`. Prior proof was
reused for those exact sources. `f95e230e` remains an ancestor.

### Step 6: retain only the final two synchronized global LFO ticks

A selected synchronized phase is calculated from its clock ratio, and its
waveform output uses the preceding phase. Thus two final global ticks replace
all earlier phase/output writes in an idle block. The idle path now advances
the prefix clocks directly, retains real tempo notifications in order, and
updates the integer counter and helper-entry count in aggregate. It then
evaluates only the final two ticks. This removes more idle work while keeping
the sound and stored state identical when a player resumes a note.

The admission guard still requires shapes 0–3, synchronization, empty keys,
inactive voices, disabled ARP and no pending gate/restart. Global held/noise
state and disabled ARP RNG state freeze exactly as before. Per-voice state
also freezes in the reference. Moving float smoothers still use their scalar
updates; only exactly settled output/master state uses the zero shortcut.
There are no per-sample DSP loops in the settled idle branch. Buffer clearing
remains necessary to return exact output and workspace bytes to callers.

`python3 tests/host/miami_cpu_test.py --out build/miami-last-ticks-step6
--compare-ref f9ffd12e` passes the original, long-idle and every-duration
1–2000-block idle-then-note workloads for all 30 programs. All three trace
sizes and SHA-256 values remain those recorded for Step 4. Independent
read-only review found no intermediate phase/output consumer in the idle
scheduler and no change to notification order or retained state.

The separate `--sanitize` comparison also passes in
`build/miami-last-ticks-step6-sanitized/` with the same three trace hashes.
The complete native 172-block golden render is 15/15 identical.

The first Elite attempt lacked Pillow in Python 3.12. Pillow was installed
under ignored `build/miami-python-deps/`. The Mac Docker wrapper then exposed
a separate path issue: standard include paths referred to the host toolchain
location, while its container mounted that tree at `/opt/jieli`. The follow-up
build runs directly in Linux with the SDK/toolchain mounted read-only at their
matching absolute paths. Neither environment repair changes engine source.

The direct Linux Elite build passes (`build/miami-step6-elite-linux-build.log`,
`X0X_DEV_PADCAL=1`, JieLi Clang 4.0.1). Linked `.text` is **412,320 bytes**,
**+112 bytes** from Step 5. Every allocated RAM section remains unchanged;
`.pool` remains 328,880 bytes with 15,184 bytes free. The app image is
416,048 bytes. Target ELF SHA-256:
`e680a8b071b04329a0b5ac7a81026aec12e7d545e6310ac86d09c792e8c4f825`.

Step 6 file list: `firmware/src/dsp/original_miami_backend.c` and
`MIAMI-OPT.md`. Its instruction-count qualification follows with the
measurement harness step below. No device was accessed.

### Step 7: qualified Docker/Valgrind idle measurement

`tools/synth684_harness/run.py --icount --idle-icount --golden
build/synth684-golden-9d1cf9fb` now renders all 15 scripted scenarios at both
`-O2` and `-Os`, then counts only idle Miami calls. Each of all 30 programs
has 64 uninstrumented warmup blocks and 172 measured 256-frame blocks at
217.5 BPM. Only `original_runtime_render()` is counted, including its float
bridge and buffer validation. Preparation, initialization and file I/O are
excluded. Every idle PCM byte must be positive zero. Both optimization levels
must match all 15 goldens when `--golden` is requested.

The runner is Docker/Colima x86-64, local image
`x0x37-final-cache:dev-532d91f`, GCC 12.2.0 and Valgrind 3.19.0. Valgrind was
installed successfully with apt. All compared engines use identical flags,
including `-Os`, software FMA and strict floating-point contraction settings.
The first counting attempts also instrumented irrelevant active scenarios;
those were stopped and superseded by the idle-only collector. Exact previously
compiled engine objects were reused after the benchmark-driver change; the
new driver was compiled normally. No source or compiler flag changed for any
reused engine object.

Mean **idle instructions per 256-frame public render call**, all 30 programs:
**168,948.867 before (`f9ffd12e`) → 10,612.767 at Step 5 → 10,349.033 at
Step 6 (`16ee9453`)**. This is **93.874% fewer** than the unoptimized idle
path. Baseline program means range 168,016–170,828; Step 6 means range
10,322–10,421. All three runs use REAL pinned inputs and pass **15/15 goldens
at both `-O2` and `-Os`**. Raw reports/logs are named
`build/miami-qualified-{baseline,step5,step6}*`.

A representative Step 6 Callgrind dump attributes about 5,120 instructions
to zero-fill string operations and 2,048 to float-bridge copy operations per
block. These are required buffer bytes, rather than per-sample DSP updates.
The full public call is still above the older 1–3k instruction goal; the next
step addresses this measured buffer cost. No AC79 CPU percentage is claimed,
and the 12–20% device-load target remains for the authorized perf lane.

Step 7 file list: `tools/synth684_harness/run.py`,
`tools/synth684_harness/synth684_bench.c`, and `MIAMI-OPT.md`.

## Step 8: bulk zero-store follow-up (2026-10-30)

The qualified Step 6 measurement above is the comparison baseline for this
follow-up: 10,349.033 mean Callgrind instructions per public 256-frame idle
render, versus 168,948.867 in `f9ffd12e`. About 5,120 of those Step 6
instructions were attributed to necessary zero-buffer writes; this follow-up
targets their implementation without removing any observable write.

`experiments/original_synth/miami_zero.h` now uses an explicit compiler
bulk-memory builtin (or the standard `memset` fallback) instead of manually
expanding 32 individually addressed stores. It retains the zero-length/NULL
no-op behavior and preserves every dry, scratch, intermediate word and public
float output byte. Only IEEE-754 binary32 positive zero is admitted by the
existing Miami path. The idle admission guards, frozen state, clocks, active
DSP processing, master smoother and normal render bridge are unchanged.
The public API still writes outputs on *every* call, so a zero-instruction
block cannot honestly be claimed.

New isolated source test: `tests/host/miami_zero_test.c`. On Linux x86-64,
GCC and Clang at each of `-Os` and `-O2` pass 9,252 guard-tested
length/offset/repetition combinations for each of three buffer types.
GCC and Clang ASan/UBSan runs also pass. The deterministic test covers
every frame count 0..256, offsets, random surrounding bytes and NULL/n=0.
It imports no private synth source. An informal 256-word `-Os` host
microbenchmark measured 37.66 -> 13.74 ns/block with GCC and 19.47 -> 14.35
ns/block with Clang for the previous unrolled versus bulk-store helper.
Those timings are not Callgrind counts, AC79 CPU utilization or proof of
whole-engine improvement.

**Still required on the Mac for this exact new header:** re-run all 30
program PCM/state comparisons (including every 1..2000-block idle-then-note
duration), native-FMA comparison, sanitizers, all 15 real-input golden renders,
Elite build/size, and the matched 30-program Valgrind instruction count at
both `-Os` and `-O2`. The private #684 headers and pinned golden directory
are intentionally absent from public GitHub; this session does not claim those
checks passed at Step 8. No hardware/device CPU percentage was measured.

Run from the checked-out `chat/miami-opt` branch in the existing configured
Mac build environment:

```sh
mkdir -p build
cc -std=c99 -Os -fno-builtin -Wall -Wextra -Werror \
  -Iexperiments/original_synth tests/host/miami_zero_test.c \
  -o build/miami-zero-test
./build/miami-zero-test
python3 tests/host/miami_cpu_test.py \
  --out build/miami-bulk-state --compare-ref f9ffd12e
python3 tests/host/miami_cpu_test.py --sanitize \
  --out build/miami-bulk-sanitize --compare-ref f9ffd12e
python3 tests/host/miami_cpu_test.py --native-fma \
  --out build/miami-bulk-native-fma --compare-ref f9ffd12e
python3 tools/synth684_harness/run.py --elite \
  --golden build/synth684-golden-9d1cf9fb \
  --out build/miami-bulk-elite
python3 tools/synth684_harness/run.py --icount --idle-icount \
  --golden build/synth684-golden-9d1cf9fb \
  --out build/miami-bulk-icount
```

Compare the new `-Os` idle program totals against
`build/miami-qualified-step6*` from the same qualified Docker/Valgrind
compiler configuration; report improvement or regression rather than
substituting the standalone memory-store timing. The final executable
candidate remains UNQUALIFIED until these runs succeed.

## Step 9: temporary reference-audio investigation (historical; superseded by Step 12)

The owner reported that the locally built `c287cfe4` perf firmware (reported
base `9d1cf9fb`) sounds muted/dull in Miami, despite approximately 0.2% idle
CPU. That commit is **not resolvable on this GitHub remote**, and neither its
device PCM nor its complete build inputs are available in this Chat session.
A target-specific cause therefore cannot be certified without the Mac
comparison. This change must **not** be described as device sound FIXED or
bit-exact for all programs until that comparison passes.

To isolate both suspect original/optimized active paths, the selected
`miami_ms20.c` software-FMA nonlinear interpolation now calls the same
full-range `original_rne_fmaf` used by the baseline, and the selected
`miami_fx_chain.c` FX2 bypass calls the original checked integer-mixer
wrapper, including scratch, state and the exact floor/rounding behavior.
These defensive reference restores are *active-audio only*. The already
qualified settled-idle control/clock/output paths and their exact zero
buffer stores are untouched. No filter topology, gains, presets, UI, pad
routing, project revision or shared Chicago DSP path changes.

As a separate exact active-control optimization, `miami_pow2_fma.h` replaces
power-of-two synchronized MG shape FMAs only when the binary32 multiply is
exact; tiny phase values retain the original fused fallback. Three VCF
envelope FMA operations with constant operands are replaced by their exact
round-to-nearest binary32 bit patterns, 0x3a83126f, 0x3fc820c5 and
0x3b03126f, without changing recurrence order or update counts.

**Host evidence in this chat:** a standalone 55,000,000-case original
specialized nonlinear interpolation versus libc `fmaf` fuzz passed on
Linux/GCC; it did *not* reproduce the reported dullness and does not
qualify AC79 codegen. The new `miami_pow2_fma_test.c` passed 6,000,000 random
phases plus 24 boundary phases with GCC and Clang at both `-Os` and `-O2`,
in both native and software-FMA-stub variants; native ASan/UBSan variants
also passed. Its software-FMA stub uses `fmaf` as the isolated oracle; the
full 18-limb implementation and entire engine need the Mac regressions.
An existing repository bypass unit test targets the original checked
wrapper; whole-engine traces and Callgrind have **not** been run on the
current Step 9 source. No measured active CPU improvement or device
percentage is claimed. The sound outcome is **UNVERIFIED**.

### Mac source-sound, full-state and performance acceptance

The owner Mac must have the original local Git commits and the 12 pinned
private #684 headers available outside Git (never stage/commit/push them),
plus the existing 15-file golden directory. Run from the latest
`chat/miami-opt` worktree, without switching any production or nightly
branch, and keep generated outputs under the ignored `build/` tree.

```sh
git fetch origin chat/miami-opt
git switch chat/miami-opt
git pull --ff-only origin chat/miami-opt
git cat-file -e 40935bd1^{commit}
git cat-file -e f9ffd12e^{commit}
git cat-file -e 9d1cf9fb^{commit}
python3 tools/check_original_inputs.py --scope all
mkdir -p build
cc -std=c99 -Os -ffp-contract=off -fno-fast-math \
  -Wall -Wextra -Werror -Iexperiments/original_synth \
  tests/host/miami_pow2_fma_test.c -lm -o build/miami-pow2-fma-test
./build/miami-pow2-fma-test
OUT="build/miami-ref-active-$(date +%Y%m%d-%H%M%S)"
python3 tests/host/miami_cpu_test.py --compare-ref 40935bd1 \
  --bench-rounds 3 --repeats 5 --out "$OUT"
python3 tests/host/miami_cpu_test.py --compare-ref 40935bd1 \
  --sanitize --out "$OUT-sanitize"
python3 tests/host/miami_cpu_test.py --compare-ref 40935bd1 \
  --native-fma --out "$OUT-native-fma"
python3 tests/host/miami_cpu_test.py --compare-ref f9ffd12e \
  --out "$OUT-f9-diff"
python3 tools/synth684_harness/run.py \
  --golden build/synth684-golden-9d1cf9fb \
  --out "$OUT-goldens"
```

The 30-factory PCM/state equality includes all individual idle durations
1..2000 blocks followed by a note, release/FX history, panic, note and
transport events. A fail is a **sound blocker**, not a result to waive.
The 15 real-input golden render files must match exactly; STUB-NOT-VALID
renders are never proof.

The same source must pass a qualified `-Os` active+idle instruction
measurement and, separately, a real AC79 Elite/Pro target build with actual
private inputs:

```sh
OUT="build/miami-perf-$(date +%Y%m%d-%H%M%S)"
python3 tools/synth684_harness/run.py --icount \
  --golden build/synth684-golden-9d1cf9fb --out "$OUT-active"
python3 tools/synth684_harness/run.py --icount --idle-icount \
  --golden build/synth684-golden-9d1cf9fb --out "$OUT-idle"
X0X_DEV_PADCAL=1 python3 tools/synth684_harness/run.py --elite \
  --golden build/synth684-golden-9d1cf9fb --out "$OUT-elite"
```

The `--icount` runs require the Mac's qualified x86-64 Linux
Docker/Colima runner with Valgrind and the same compiler/flags as the
original measurement, not native macOS Valgrind. Compare per-scenario
Callgrind instruction counts and the 30-program idle workload against the
saved `build/miami-qualified-{baseline,step5,step6}*` records. The active
regression's benchmark JSON reports per-factory host timing only, not AC79
cycles. Check `c287cfe4` against the original Mac renders using its actual
build/source receipt before attributing the owner's listening difference
to either suspected optimization.

Acceptance is sound-first: **30/30 full-state equality**, **15/15 real
goldens**, target build, *then* measured active CPU change and owner device
check. No merge into `wip/one-build` or `nightly` on the strength of these
standalone arithmetic tests alone.

## Step 10: original-AU PCM equality gate (2026-10-29)

**Corrected owner acceptance:** Match original Miami completely (level AND
sound) with identical binary32 PCM, not merely matching RMS/peak or similar
tone. Preserve idle/active CPU gains only when the 30-program PCM/state and
the true original-source golden comparisons pass. Do not introduce a
speculative gain boost, normalization, filter change or amplitude threshold.

### Distinguish two independent reference datasets

- `build/synth684-golden-9d1cf9fb` is a 15-file, 44.1-kHz **internal
  port regression** collection, including Chicago scenarios. Exact parity
  here catches port-source regressions, but is not original-Audio-Unit proof.
- The independent **original Miami 3.1.6 Audio Unit** investigation, retained
  in private Library `Miami-original-preset-findings-20261028.json`,
  describes 16 stereo IEEE-float32 WAV captures: 44.1 kHz, 132300 frames
  each, note 48 at velocity 80, note-on frame 4410, note-off frame 92610.
  Eight waveform configurations have two fresh-instance takes each. The
  Library report records their peak/RMS/PCM hashes, not actual playable
  WAV bytes in this container. To compare them, produce candidate files
  from IDENTICAL program, macro, sample rate, event timing and cold state.
  Do not compare these 3-second captures directly with 172x256-frame
  internal port golden files: stimuli and lengths differ.
- The historical `tests/host/render_miami_au_compare.py` is NOT
  a valid strict gate: it can skip, truncates mismatched lengths, scores
  similarity, uses a 48-kHz scenario profile and its builder references
  retired `wobble.c`. Do not run or revive that retired engine.

### Verified observations and diagnostic gate

Current source applies original voice and master volume controls, unison
gain correction, FX1/FX2 fixed-point shifts, signed-int16 conversion,
divide-by-32767 reconstitution and the firmware part mixer. No isolated
multiplier has been demonstrated wrong. Independently run Linux host
comparisons of the optimized interpolation against fmaf on 55,000,000
inputs, and the settled FX2 endpoint mixer on 131,070 dry/endpoint
combinations, all PASS. This does NOT establish source-to-original-AU
parity or the cause of the reported low level. Previous Step 9 DSP edits
remain on the branch; no additional unproven audio edits were made here.

New `tools/synth684_harness/compare_pcm.py` is deliberately fail-closed:
supports stereo IEEE-float WAV and stereo raw .f32; checks identical
sample rate, names and frame count; compares each binary32 word including
signed zero; reports per-channel peak, RMS, fitted gain, gain-residual,
first divergence, mismatches and SHA256 for diagnosis ONLY. Peak/RMS
equality or near-perfect correlation never changes a FAIL to PASS.
New `tests/host/miami_pcm_compare_test.py` **8/8 tests PASS** under
Linux Python, and both files compile through `python3 -m py_compile`.
Tests cover exact matches, half-gain mismatch, altered waveform,
signed-zero mismatch, unsupported lengths, WAV/rate mismatch,
missing scenarios and correct failing exit status. This is a local
standalone proof of the diagnostic, NOT a source engine golden PASS.

### Exact Mac commands: internal port regression

```sh
git fetch origin chat/miami-opt
git switch chat/miami-opt
git pull --ff-only origin chat/miami-opt
python3 tools/check_original_inputs.py --scope all
python3 -B tests/host/miami_pcm_compare_test.py -q
python3 -B tests/host/miami_cpu_test.py \
  --compare-ref 40935bd1 --out build/miami-20261029-pcm-state
python3 -B tests/host/miami_cpu_test.py \
  --compare-ref 40935bd1 --sanitize --out build/miami-20261029-sanitize
python3 -B tests/host/miami_cpu_test.py \
  --compare-ref 40935bd1 --native-fma --out build/miami-20261029-native
python3 tools/synth684_harness/run.py \
  --golden build/synth684-golden-9d1cf9fb \
  --out build/miami-20261029-port-golden
python3 tools/synth684_harness/compare_pcm.py \
  build/synth684-golden-9d1cf9fb \
  build/miami-20261029-port-golden/renders \
  --require-count 15 --rate 44100 \
  --json build/miami-20261029-port-golden/peak-rms-exact.json
```

### Exact Mac commands: original Audio Unit reference

Set the environment variables to the ACTUAL, verified 16 original AU WAV
directory and 16 matching 44.1-kHz current Miami .f32 render directory.
The command refuses absent inputs or any discrepancy and does not assume
such a candidate renderer has already been created:

```sh
test -n "$MIAMI_AU_REFS" && test -d "$MIAMI_AU_REFS"
test -n "$MIAMI_AU_CAND" && test -d "$MIAMI_AU_CAND"
python3 tools/synth684_harness/compare_pcm.py \
  "$MIAMI_AU_REFS" "$MIAMI_AU_CAND" \
  --require-count 16 --rate 44100 \
  --json build/miami-20261029-original-au-peak-rms-exact.json
```

### PERF after exact render proof only

Run the following in the qualified Linux x86-64 Docker/Colima Valgrind
environment with the same compiler, flags and real pinned inputs; the
Elite build additionally requires the owner's real JieLi SDK V1.2.1
inputs (V1.2.0 cannot substitute).

```sh
python3 tools/synth684_harness/run.py --icount \
  --golden build/synth684-golden-9d1cf9fb \
  --out build/miami-20261029-active-perf
python3 tools/synth684_harness/run.py --icount --idle-icount \
  --golden build/synth684-golden-9d1cf9fb \
  --out build/miami-20261029-idle-perf
X0X_DEV_PADCAL=1 python3 tools/synth684_harness/run.py --elite \
  --golden build/synth684-golden-9d1cf9fb \
  --out build/miami-20261029-elite
```

Acceptance remains PENDING until the owner Mac supplies exact reference
bytes and qualified comparisons. A bit mismatch is not fixed by matching
RMS. Preserve the first divergence and inspect the actual first mismatched
DSP/mixer stage. Do not claim hardware audio fixed, new CPU percentages,
a target firmware PASS or permission to integrate into `wip/one-build`.

## Step 11: withdrawn device gain investigation (owner correction, 2026-10-30)

The earlier **-37.1 dBFS / -49 dBFS** Miami capture was **NOT** made
with firmware OUTPUT=LOUD. The owner repeated the same device test with
OUTPUT=LOUD and maximum volume: **peak -1.9 dBFS, sounding-part
RMS -11.0 dBFS**. Miami's output level is acceptable. Therefore,
the prior 27–30 dB device attenuation hypothesis, the mixer LEVEL=20
and low-velocity explanations, and all proposed gain/shift/USB
adjustments are withdrawn. **No gain adjustment was ever committed on
this lane, so there is none to revert.** Do not ask for more mixer,
velocity, knob or LOUD checks on this resolved point.

The obsolete exploratory `tools/miami_gain_audit.py` and its test
are removed from this PR's current tree to avoid a misleading active
diagnostic path. The historical 2026-10-29 commit and evidence remain
in Git history for provenance only. `tools/synth684_harness/compare_pcm.py`
remains for the actual **sound** parity task.

## Step 12: resume exact active CPU optimizations, no gain change

**Source change:** restore two exact, previously qualified fastpaths from
`wip/miami-opt2` that Step 9 temporarily disabled during the now
withdrawn "dull/muted" report:

1. In `experiments/original_synth/miami_ms20.c`, use
   `original_miami_interpolate_rne` for nonlinear coefficient
   interpolation in the selected software-FMA branch. Its guarded
   integer-precision path is exactly round-to-nearest-even, with the
   original full 18-limb `original_rne_fmaf` fallback outside the
   guard. The public output and filter coefficients are unchanged.
   Keep the independently checked MG/VCF optimizations from Step 9.
2. Restore **byte-for-byte** the original
   `experiments/original_synth/miami_fx_chain.c` from the pre-Step-9
   lane (Git blob `651688c8c52ad1a0360c043459cf77d8fc19ca7d`).
   Its FX2 settled-endpoint shortcut copies the original signed16
   scratch data and applies the exact floor-shift attenuation
   (for positive dry samples, dry-1; negatives unchanged) while
   retaining the original state, gain and transition fallback.
   No output-level multiplier is added.
3. Chicago, Miami native gain, FX1, master volume, firmware part
   mixer, LOUD, USB UAC, I2S, UI, preset/selector/pads, persistence
   revision 13 and protected regions are all unchanged.

**Actually executed local independent host arithmetic proof:**
- On Linux x86-64, GCC 14.2 and Clang 17, each at `-Os` and `-O2`,
  **1,100,280 binary32 input triples** per run matched the independently
  implemented exact 18-limb fused multiply-add oracle, including normal
  guarded domains, arbitrary finite operands, tie cases, binade seams,
  signed zero and subnormal cases. All four runs PASS.
- FX2 direct shortcut vs exact source
  `original_fixed_fx_process` (original Git blob
  `6644ea5155f5c2e7790b1a9bf75fb9f5b265f796`):
  **12,640 state/scratch/PCM comparisons** per compiler/config, including
  settled gain endpoints, nonzero fractional remainders, transitions,
  dirty states, positive/negative dry values and varied frame counts.
  GCC/Clang `-Os`/`-O2` all PASS.
- In one **isolated host-only** -O2 microbenchmark using original
  nonlinear-table inputs (16,384 triples, 3,276,800 calls per side),
  reference software FMA averaged **60.240 ns/call** and guarded
  optimized interpolation averaged **21.943 ns/call** (**63.6%
  reduction**). This is not whole-voice, target DSP, I2S, or real
  device performance. No new idle Callgrind count is claimed.
- New source still needs full exact candidate-vs-reference PCM/state
  trace for all 30 programs, 15/15 internal real-input goldens, active
  and idle Valgrind counts, original-AU selected waveform comparisons
  under identical stimuli, and Elite/Pro builds on owner's Mac.
  Reusing a pre-Step-9 blob is not proof those integrated runs passed
  on this new commit.

**Additional authentic original waveform references were located:**
personal Library `Miami-original-44100-reference-handoff-20261028.zip`
contains **seven unique original Miami 3.1.6 stereo float WAVs** at
44.1 kHz, with 16 documented same-input capture aliases, control
settings and frame-exact events. This private archive is not an
internal factory-p00 render; it is independent selected-waveform
evidence and must never be committed or published. The owner-reported
host 30-program parity remains a separate acceptance witness. Do
not mistake equality to the internal port baseline for equality
to all original proprietary AU behavior.

### Mac verification from exact HEAD with pinned private inputs

```sh
git fetch origin chat/miami-opt
git switch chat/miami-opt
git pull --ff-only origin chat/miami-opt
git status --short
python3 tools/check_original_inputs.py --scope all

OUT="build/miami-exact-$(git rev-parse --short HEAD)-$(date +%Y%m%d-%H%M%S)"
python3 tests/host/miami_cpu_test.py \
  --compare-ref f9ffd12e --bench-rounds 5 --repeats 10 --out "$OUT"
python3 tests/host/miami_cpu_test.py \
  --compare-ref f9ffd12e --sanitize --out "$OUT-sanitize"
python3 tests/host/miami_cpu_test.py \
  --compare-ref f9ffd12e --native-fma --out "$OUT-native"
python3 tools/synth684_harness/run.py \
  --golden build/synth684-golden-9d1cf9fb --out "$OUT-golden"
python3 tools/synth684_harness/compare_pcm.py \
  build/synth684-golden-9d1cf9fb "$OUT-golden/renders" \
  --require-count 15 --rate 44100 \
  --json "$OUT-golden/pcm-audit.json"
```

Fail on **any** PCM/state difference; no normalization,
threshold match, automatic gain or golden replacement. The
`miami_cpu_test.py` scope also invokes the existing dedicated
`miami_bypass_test.c` and `miami_interpolation_test.c` with
the real 18-limb oracle and authenticated private input tables.

Measure active AND idle CPU in the qualified x86-64 Linux
Docker/Colima+Valgrind runner with the **same compiler, flags and
goldens** as the saved baseline:

```sh
python3 tools/synth684_harness/run.py --icount \
  --golden build/synth684-golden-9d1cf9fb \
  --out "$OUT-active-icount"
python3 tools/synth684_harness/run.py --icount --idle-icount \
  --golden build/synth684-golden-9d1cf9fb \
  --out "$OUT-idle-icount"
X0X_DEV_PADCAL=1 python3 tools/synth684_harness/run.py --elite \
  --golden build/synth684-golden-9d1cf9fb \
  --out "$OUT-elite"
```

The 30-program host runner's unit-test preflight now also executes the
existing `miami_pow2_fma_test.c` in native and software-FMA configurations;
that isolated test was rerun locally with the runner's `-Os` compiler flags
and passed 6,000,000 random plus 24 boundary cases in each mode. Whole-engine
goldens and target performance remain Mac-only acceptance gates.

The SDK V1.2.1, toolchain, the 12 pinned private #684 inputs and
15 real-input golden files remain owner-Mac-only. Do not use the
uploaded SDK V1.2.0 as a replacement. Run Pro build by the current
`BUILDING.md` command under the same pinned configuration. Neither
the local proof nor any PR means DEVICE VERIFIED or approval to
merge/publish/flash. Until these runs pass, label it **SCOPED
HOST-PARITY PASS / WHOLE-ENGINE & DEVICE PENDING**.


## Step 13: guarded Miami filter-table interpolation (2026-10-29)

Continue the owner-authorized active-voice CPU optimization with **no sound,
gain, output, Chicago, UI, pads or slot change**. The selected Miami filter
coefficient updates still invoked the complete software FMA for each of three
MS20-table lookups and the resonance table, even though the same validated
`original_miami_interpolate_rne` helper was already used for nonlinear audio
interpolation.

### Source change

- `experiments/original_synth/miami_ms20.c`: reuse the existing
  `interpolate` macro for the three filter-table coefficient lookups and
  resonance interpolation during the checked control preparation. All other
  arithmetic, table inputs, rounding, ramp writes and audio samples are
  unchanged.
- `experiments/original_synth/miami_selected_voice.c`: under
  `ORIGINAL_USE_SOFT_FMA`, route only the three selected immutable-table
  lookups and resonance interpolation to the existing exact helper. In the
  native-FMA configuration, `table_fma=fmaf` and both code paths are
  unchanged. Its compile-time `MIAMI_VOICE_CHECKED_TABLE_PREP` alternative
  still delegates to the checked prepare function above.
- The preexisting helper preserves exact round-to-nearest-even operation for
  a positive normal base, a suitably small negative difference and a
  fractional coordinate in [0,1). For other cases (ascending, negative,
  zero/subnormal, seam or exponent boundary), it falls back to
  `original_rne_fmaf`. This is an execution optimization, **not** a
  resampler or table approximation. The fractional coordinate is still
  calculated from the original rounded binary32 `q - (float)j`; no higher
  precision, table quantization, changed threshold or reordered state update.
- `tests/host/miami_table_interpolation_test.c`: self-contained public
  test, no #684 private coefficient tables. It tests all 128 indexed
  intervals with directed subnormal/tie/zero/normal table data and randomized
  dynamic filter/resonance values against the full 18-limb FMA oracle. The
  Miami CPU suite preflight now runs this test automatically in addition to
  the existing interpolation, FX2, idle, zero and MG/VCF tests.

### Actual host execution / what remains unverified

Local Linux x86-64 compiler proof with the *exact public helper header*
(Git blob `6258b044484a575d976109bd0a863274cccd041f`), the
historical archived independent 18-limb oracle source, and new test:
- GCC 14.2 and Clang 17 at both `-Os` and `-O2`: **PASS 408,412
  cases each**, including **68,138 guarded and 340,274 fallback**
  comparisons per run. Every observed result matched the full 18-limb
  RNE oracle bit-for-bit.
- GCC and Clang ASan+UBSan with halt-on-error: **PASS 408,412
  cases each**.
- Isolated 1,048,576-call `-Os` table-coordinate microbenchmark
  on Linux with realistic positive descending coefficient values:
  GCC full software FMA ~79–87 ns/call, guarded interpolation
  ~5–9 ns/call; Clang reference ~63–172 ns/call (outlier variation),
  helper ~7–11 ns/call. This is an isolated reference-vs-helper
  microbenchmark, **not** whole-engine, AC79, voice load, or
  reproducible target clock evidence. Whole-engine CPU improvement
  is **not measured** here.

The full Miami program/voice owner and proprietary table inputs were **not**
rebuilt in this Linux shell, and no new original plugin WAV or golden PCM
comparison was run; no claim of full-engine bit-exact PASS is made for
this new HEAD. This preserves the requirement that active and idle CPU
savings are retained only if the real 30-program state/PCM and 15-file
golden gates pass on the owner's Mac. The seven archived KORG AU reference
WAVs are an additional independently selected waveform proof, not a
substitute for the complete pinned factory golden collection.

### Owner Mac qualification (from this exact pushed source)

```sh
git fetch origin chat/miami-opt
git switch chat/miami-opt
git pull --ff-only origin chat/miami-opt
git rev-parse HEAD
python3 tools/check_original_inputs.py --scope all
OUT="build/miami-table-$(git rev-parse --short HEAD)-$(date +%Y%m%d-%H%M%S)"
python3 tests/host/miami_cpu_test.py --compare-ref f9ffd12e \
  --bench-rounds 5 --repeats 10 --out "$OUT"
python3 tests/host/miami_cpu_test.py --compare-ref f9ffd12e \
  --sanitize --out "$OUT-sanitize"
python3 tests/host/miami_cpu_test.py --compare-ref f9ffd12e \
  --native-fma --out "$OUT-native"
python3 tools/synth684_harness/run.py \
  --golden build/synth684-golden-9d1cf9fb --out "$OUT-golden"
python3 tools/synth684_harness/compare_pcm.py \
  build/synth684-golden-9d1cf9fb "$OUT-golden/renders" \
  --require-count 15 --rate 44100 --json "$OUT-golden/exact-audio.json"
```

Run separately in the qualified x86-64 Linux Docker/Colima Valgrind
environment, comparing **active and idle** instruction counts to the
same pinned compiler, flags, input traces and baseline:

```sh
python3 tools/synth684_harness/run.py --icount \
  --golden build/synth684-golden-9d1cf9fb --out "$OUT-active-icount"
python3 tools/synth684_harness/run.py --icount --idle-icount \
  --golden build/synth684-golden-9d1cf9fb --out "$OUT-idle-icount"
X0X_DEV_PADCAL=1 python3 tools/build.py --board smk37 --model elite
X0X_DEV_PADCAL=1 python3 tools/build.py --board smk37 --model pro
```

The native build requires the owner's actual pinned JieLi SDK **V1.2.1**
and excluded private #684 inputs. SDK V1.2.0 is not substitutable.
Until these gates run at this HEAD, mark the optimization
**scoped-host-PASS / full-golden-and-target-NOT_RUN**. Neither a
branch push nor source-level equality is device approval, a merge
instruction, or an installer publication.

