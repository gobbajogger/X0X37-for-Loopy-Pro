# Miami CPU optimization

The inherited optimization at `f95e230e` is bit-exact in the checks below.
Its AC79 Elite build passes. CPU qualification is **pending**: this session
cannot run Valgrind, so there is no measured instruction-count reduction yet.
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
