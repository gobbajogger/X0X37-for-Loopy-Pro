# PR merge trials — 2026-10-29

Status: in progress. This follow-up has writable Git metadata and a working
Colima Docker worker. The earlier run made no merge or commit. Its permission
failures are historical, not current blockers.

## Inputs and boundaries

- Lane: `codex/pr-merges`; starting HEAD `5ed2693d384821ff7c32927b2da4ea7e3ed67f90`
  (`nightly-20261029`), tree `ec4918b8f36792958582c8975b0c164a1a619195`.
- Trial order: #212, #677, #651, #650, #638, #635, #636.
- Fetch each with `git fetch origin pull/N/head:prN`. Origin is read-only.
- Preserve 909/808/BREAK/CHICAGO/MIAMI display order and nightly's SMC fixes.
- PR #684 is excluded. Commit locally only; no push, installation, flash,
  connected-device access, release build or full gate.
- Never stage or commit `*_private.h`, `fx_tables.h`, `miami_factory_typed.h`,
  `.component`/`.vst3` bundles, `.fwsc`, `ctl.img` or `out/`.
  The starting tree already tracks `evidence/final-wbl1-tip-packages/elite/ctl.img`;
  leave it untouched. Audit staged paths and run `git status` before every commit.
- No supported reader for other project chats was found. Work stays in the
  assigned lane; no other lane or connected device is modified.

## #212 — aborted: 18 conflicted files

Fetched head: `9aab1f1deb8da75a0ae3dcae59c5841787f50cd1`.
Trial: `git merge --no-commit --no-ff pr212` against starting HEAD.
The merge failed and was aborted with `git merge --abort`. No candidate build
or host result is possible for an unresolved merge; both are NOT_RUN.

The PR contains an older selectable-engine/wobble composition. Nightly already
has later independently integrated implementations. Fifteen files conflict as add/add, plus three content conflicts; these are
not eighteen missing features. Each conflict and suggested resolution follows.

- `docs/engineering-delivery/wobble-combined-native.md` (add/add): separate
  copies of the same delivery report; the PR adds its historical stage-release
  and coordinator notes. Retain current facts and append only useful historical
  provenance, without reinstating old writer leases or stage gates.
- `experiments/wobble/README.md` (add/add): PR's older control description lacks
  nightly's owner ADSR extension. Retain the current control contract.
- `experiments/wobble/compare.py` (add/add): fixed 48,000-frame comparison versus
  nightly's timing-aware comparison battery. Retain the current timing interface;
  port only independently useful comparison changes.
- `experiments/wobble/foundation_adapter.h` (add/add): old 16-parameter adapter
  versus nightly's extended adapter. Keep the current parameter/schema mapping;
  preserve legacy bank compatibility explicitly.
- `experiments/wobble/render.c` (add/add): fixed render duration/note-off versus
  configurable comparison timing. Keep current timing controls and callers.
- `experiments/wobble/test_wobble.c` (add/add): old short tail tests omit nightly's
  ADSR extension coverage. Keep the current envelope and tail tests.
- `experiments/wobble/wobble.c` (add/add): older BLEP square and fixed-tempo AR
  synth versus current refined Fourier/pulse, tempo, ADSR, split and legato DSP.
  Keep current DSP; evaluate any missing foundation behavior as a separate port.
- `experiments/wobble/wobble.h` (add/add): older parameter/state/API layout lacks
  tempo, ADSR, split and slide APIs. Retain current ABI and legacy serialization.
- `firmware/src/app/engine.c` (content): foundation registration/render/state
  hooks overlap current Chicago/Miami, recording, mixer and SMC integration.
  Port only proven missing foundation hooks into nightly; retain part mapping,
  SMC bus FX/solo handling, recording safety, memory layout and current mixer.
- `firmware/src/dsp/synth_engine.h` (add/add): hard-coded 16-parameter WBL1
  admission versus nightly's current parameter-count validation. Keep current
  validation and test legacy bank admission separately.
- `firmware/src/dsp/wobble_backend.h` (add/add): experimental 16-parameter backend
  overlaps product backend, switch normalization and held-slide behavior.
  Retain product mappings, legato and legacy-bank handling.
- `firmware/src/dsp/wobble_config.h` (add/add): DEV-only opt-in/release exclusion
  versus current product-default WBL1. Keep nightly's product configuration.
- `tests/host/check_wobble_stereo_mix.py` (add/add): older exact-engine/reference
  wrapper assumes obsolete source layout; nightly uses the current pool
  declaration, parameter unit and compatible scenario. Retain current wrapper;
  port only additional assertions valid for current mixer inputs.
- `tests/host/measure_wobble_native.py` (add/add): PR's compile-unit list omits
  `dsp/x0x_param.c`. Keep the current source closure and measure the actual build.
- `tests/host/run_wobble_backend.sh` (add/add): older experimental exclusion
  assertions disagree with current product-enable assertions. Keep current
  product contract and test any legacy-only configuration separately.
- `tests/host/wobble_backend_test.c` (add/add): older minimal adapter/state test
  overlaps current legato, envelope, switch and ownership tests. Retain current
  coverage and add only missing foundation cases.
- `tests/run_tests.sh` (content): old foundation selectors/classes overlap the
  expanded nightly test registry. Add missing selectors without removing SMC,
  BLE, recording, preset or generated-asset admission checks.
- `tools/build.py` (content): old experimental compile flags, source lists and
  generation logic overlap current product flags, header setup, empty factory
  bank policy and section/memory savings. Retain nightly build policy and port
  only required missing source units; do not restore old memory costs.

Evidence: `build/pr-merges/pr212-merge.log`,
`pr212-conflicts.json`, `pr212-conflict-detail.diff`, `pr212-core-detail.diff`.
All paths in this report without another prefix are under `build/pr-merges/`.

## #677 — aborted: one conflicted file

Fetched head: `2d555bee09021d8d71b59a5903eb1c93dd74f8e2`.
Trial: `git merge --no-commit --no-ff pr677` against starting HEAD.
The merge failed and was aborted. Candidate build and host tests: NOT_RUN.

- `firmware/src/app/engine.c` (content): immediately after
  `seq_advance(&seq, 0, &SINK)`, nightly calls `brk_midi_drain()` while the PR
  inserts a loop updating each Chicago BPM-delay insert from the sequencer
  tempo. Suggested resolution: retain the drain and add the delay update loop
  after it, before motion processing. Verify MIDI ordering and Chicago effect
  runtime tests plus part-order and SMC regressions. Do not replace the drain.

Evidence: `pr677-merge.log`, `pr677-conflict-detail.diff`.

## #651 — clean merge, build and host checks passed

Fetched head: `c1f3f776b13ad8f4f56bcbdb8a8061924191b93d`.
Trial: `git merge --no-commit --no-ff pr651` against starting HEAD.
Result: clean; 29 changed paths. Candidate source tree before this report:
`bb5576344100ded00cf7b3d286535907648cc623`.

This is the complete #638 ancestry plus the host-proof repair, not only an
isolated host-test patch. It includes Miami stock-rate handling, UI surfaces,
waveform cues and shared glyphs, strict C99 linkage and generator output checks.
The purpose is to preserve tested playback/control behavior while making those
surfaces and host checks consistent. No private header or binary is in the
staged change list.

Elite development compile/link/package: PASS, BUILD_ONLY. Command (inside the
Linux/amd64 Docker worker):

```sh
JIELI_TOOLCHAIN=/Users/mac/.jieli/toolchain \
AC79_SDK=/Users/mac/fw-AC79_AIoT_SDK X0X_DEV_PADCAL=1 CC=cc CXX=c++ \
python3 tools/build.py --board smk37 --model elite
```

Worker image: `x0x37-validation:local`, immutable ID
`sha256:0cffe038efa0f50556de3fb444de6386729d495f63703c8c3b40fdfd0482d4b1`.
Source, linked-worktree metadata and SDK/toolchain were mounted at their actual
paths; the SDK/toolchain and Git metadata were read-only. Image 350,196 B;
app code area has 202,476 B free; static RAM 92,856/98,304 B;
pool 327,820/344,064 B (16,244 B free, above 8,192 B minimum).
Log: `pr651-elite-build.log`. Artifact sizes/hashes: `pr651-build-receipt.json`.
No controller rebuild, DEV CHECKED record, Pro build, release/full gate or device
result is claimed. Pro package checks in the host runner were skipped.

Completed Linux GCC checks: `dsp-host-linkage`, `fastmath`, `fastmath-ubsan` PASS.
The remaining Linux suite was stopped during the slow simulator compile,
container exit 137; that interrupted compile has no PASS/FAIL result.
Log: `pr651-host-suite-linux-interrupted.log`. The supported native macOS checks
then ran with `CC=cc CXX=c++ X0X_HOST_DEFS=-D_DARWIN_C_SOURCE X0X_TEST_JOBS=1`:

```sh
sh tests/run_tests.sh host-build chicago-ens rec-held-duration part-order \
  smc-protocol smc-queue smc-mapping smc-link smc-ui smc-ui-bounds \
  ble-command-credit ble-central-first-notification miami-mw-wobble
sh tests/host/night_ui_test.sh
sh tests/host/ui_depth_wave_test.sh
sh tests/host/ui_drum_mix_material_test.sh
sh tests/host/ui_shared_glyph_test.sh
sh tests/host/miami_tempo_test.sh
sh tests/host/miami_stock_rates_test.sh
```

All above PASS. Waveform fixture: 220,849 checks, including exact shape
comparisons and pixel bounds. Drum MIX: 10,728 checks. Shared glyphs: 18,865,419
checks across 337 mapped slots/93 pages. All 30 Miami stock presets, eight rate
labels, internal/external tempo and project/sample/panel preservation passed.
Native logs: `pr651-host-suite.log`, `pr651-night-ui.log`,
`pr651-ui-depth-wave.log`, `pr651-ui-drum-mix-material.log`,
`pr651-ui-shared-glyph.log`, `pr651-miami-tempo.log`,
`pr651-miami-stock-rates.log`; commands, exits and hashes in
`pr651-host-summary.json`. Existing CK_FN redefinition warnings remain visible;
they did not fail these checks. No warning was suppressed for this trial.

Linux generator output checks: PASS, six tests, including `/dev/full` write
and close failures. Command: `CC=cc python3 tests/host/test_builtin_break_output.py`
in the same Docker image; log: `pr651-builtin-break-output-linux.log`.
Independent source and actual-render review found no blocking issue after
inspecting 24 candidate PNGs. This is representative static host coverage;
physical-device readability and timing remain untested.
Protected-source comparison `pr651-protected-source.diff` is empty for SMC,
BLE and part-order paths. The part-order and SMC tests above cover their consumers.
Staged-path and new-ancestry object-name audits found no excluded file.
Evidence: `pr651-merge.log`; candidate renders under
`build/host/night-ui`, `ui-depth-wave`, `ui-drum-mix-material`, `ui-shared-glyph`.

## Remaining trials

#650, #638, #635 and #636 will be fetched and trialed after #651's validation and
local commit or failure disposition. They have not yet been trialed in this run.

## Earlier baseline evidence

The earlier read-only run checked starting HEAD. `host-build` initially failed
with Darwin SDK declarations hidden by `_POSIX_C_SOURCE`; rerunning with the
supported `X0X_HOST_DEFS=-D_DARWIN_C_SOURCE` passed. The original result remains
FAIL. Logs: `baseline-host-build-initial-fail.log` and
`baseline-host-build-darwin.log`.

Baseline `part-order` (261 assertions), `smc-protocol`, `smc-queue`, `smc-mapping`,
`smc-link`, `ble-command-credit` and `ble-central-first-notification` passed.
These baseline results do not validate an untested candidate. Package tests
were skipped because artifacts were absent. No prior target build or device
result is reused.
