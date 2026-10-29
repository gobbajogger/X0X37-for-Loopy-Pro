# FMB guards and slots

Current follow-up state: Step 1 is committed as `674b3930`, and the GCC `FM_ONE` static-inline fix is committed as `457e7094`. Both fastmath and smk-clip-keys now compile, link and pass at `-O0` with GCC 15.2.0 and warnings as errors. Docker access is available. The earlier sandbox denials below are historical; the owner's current full-access instruction authorizes local commits and build-only Docker work. Steps 2, 3 and 5 are being committed separately before fresh Step 4 validation. No push or device action is authorized.

Owner request: 2026-10-29. Lane: `codex/fmb-guards-slots`, starting at `e888d046`; comparison base: `5ed2693d`. Ordinary workflow. Only `X0X_ORIGINAL_SYNTH=1` ships. Firmware integration and the connected device remain with their assigned owners. No push, install or flash is authorized.

The lane was clean at intake. Other lane refs were inspected; this task owns only the requested guards, slot hooks, regressions and migration. No supported project-chat reader is available in this session. Private headers and binaries must never enter the index.

## Step 1: host compiler attributes

Added `FM_WEAK` and `CK_MINSIZE`. Clang (including the target compiler) retains the weak and minsize attributes; host GCC omits those unsupported attributes. This lets the same sound and clip-key logic compile in host checks without changing target behavior. Target byte equality still requires a target build.

Target execution is blocked: Docker returned permission denied for `/Users/mac/.colima/default/docker.sock`. The denial will not be bypassed. No current flash/RAM measurement is available yet.

GCC 15 validation: `CC=gcc-15 sh tests/run_tests.sh fastmath smk-clip-keys`: 2 PASS, 0 FAIL (`/tmp/fmb-step1.log`). The shared arp definition also needed `CK_MINSIZE`; without it smk-clip-keys failed under `-Werror=attributes`.

**Step 1 initial READY checkpoint (subsequently committed as `0ff43a8b`).** Git could not create the lane's `index.lock` (Operation not permitted); staging did not complete. Exact files: `firmware/src/dsp/fastmath.h`, `firmware/src/app/smk_clip_keys.h`, `firmware/src/app/smk_clip_arp_share.c`, `FMB-GUARDS-SLOTS.md`. No retry or alternate index will be used. Keep subsequent steps as separate commits when an authorized environment can write the index.

## Step 2: original-engine hooks and fixed slots

The shipping path now excludes bass303/WBL1 headers, envelope helpers and legacy glyph/control mappings. The temporary zero-time `bass303_env_time_ms` guard is gone. Original controls keep their existing presentation and native parameter semantics; the reference `=0` path retains its own helpers. Miami's files under `experiments/wobble/` are unchanged. Physical wheel writes use the original parameter IDs. Engine identity constants and saved part IDs are unchanged. Existing defaults and presentation order remain 909, 808, BREAK, CHICAGO (303B), MIAMI (303A).

Reused the existing original-control presentation. Actual host UI validation: `tests/host/original_release_ui_test.sh build/fmb-step2-ui` passed normal and ASan/UBSan execution, 22,949 assertions, 36 captures, and identical normal/sanitized PNGs (`/tmp/fmb-step2-ui.log`). The shipping host app builds. On macOS, validation uses Python 3.12 and a `cc` wrapper adding `_DARWIN_C_SOURCE` for the SDK's libproc header; system Python 3.9 cannot import the new build tooling. These environment differences are recorded separately from source regressions.

**Step 2 READY TO COMMIT.** The prior index.lock denial still applies. Exact files: `firmware/src/app/ui.c`, `firmware/src/app/main_fm1.c`, `FMB-GUARDS-SLOTS.md`.

## Step 3: host regressions

Restored the pre-merge stream semantic checks for both build profiles. The merge had put the full recorded-endpoint and per-note checks under `X0X_ORIGINAL_SYNTH`, then added a legacy branch that rejected all upper step flags. This rejected valid `=0` recorded releases and legacy adoption. The restored code validates each known stream's actual semantics and retains rejection of unknown/frozen layouts.

Added postponed annotations in `tools/rom_guard.py`. System Python 3.9 previously failed to import `Node.address: int | None`, which prevented the shipping host build from reaching compilation. Its 32 ROM-guard tests pass. The original build-composition and external-input unit tests also pass. All 12 external input files match their pinned manifest; their bytes remain local and unchanged.

Focused legacy proof: bass-tagged-storage passes 1,621 checks with WBL0 and 1,670 with WBL1 plus 19 real browser/adoption protocol checks; rec-endpoint-storage passes 1,472 checks; rec-endpoint-lifecycle passes 686,525 checks. These are the affected legacy storage/recording paths.

**Step 3 READY TO COMMIT.** Exact files: `firmware/src/app/project.c` (the `stream_parts_ok` regression repair), `tools/rom_guard.py`, `FMB-GUARDS-SLOTS.md`. Step 5 adds separate migration hunks in `project.c`. The index.lock denial still applies. Full-suite comparisons are pending below; the owner's earlier count of 55 is historical evidence, not this environment's measured result.

## Step 5: project revision 13 migration

Known stored formats 1/2/3/6/7/8/9/12 with legacy sound revisions through 12 now load into revision 13. The converter preserves note pitches, gates, accents, slides, velocities/endpoints, pattern positions, song assignments and compatible shared-instrument settings. Legacy synth cells are replaced by the default Chicago and Miami patches in the same physical slots. Legacy synth parameter-motion lanes are cleared because their pot indices have different meanings in the new engines; probability and velocity lanes are retained. Known per-note sound trailers in streams 7/9 are discarded while their musical notes survive. Diagnostic revision 12 patches also reset to defaults. Unknown stream layouts remain unsupported rather than being guessed.

The portable importer also migrates revision 10 files without identity trailers, revision 11 files with trailers and diagnostic revision 12 files. The real editor VALIDATE step canonicalizes the expanded revision 13 payload and header; COMMIT remains a separate operation. Loading/validating does not save or modify the source objects. The next explicit save writes revision/stream 13.

`tests/host/project_original_migration_test.sh` passes 944 checks, also under ASan/UBSan. Coverage includes all five stored formats, exact pattern/arrangement preservation, both default engine identities, save/reload, unknown-format and CRC rejection, both portable layouts and the actual editor WRITE/VALIDATE/COMMIT path. Logs: `/tmp/fmb-migration.log`, `/tmp/fmb-migration-san.log`. The test is registered as `project-original-migration` in the host runner.

Original integration remains PASS: 210,508 checks, matching normal/sanitized output and audio hash `79d686b167ffdfef` (`/tmp/fmb-integration.log`).

**Step 5 READY TO COMMIT.** Exact files: `firmware/src/app/project.c` (migration hunks), `firmware/src/app/engine.c`, `firmware/src/app/engine.h`, `firmware/src/app/editor.c`, `tests/host/project_original_migration_test.c`, `tests/host/project_original_migration_test.sh`, `tests/run_tests.sh`, `FMB-GUARDS-SLOTS.md`. No index retry is authorized after the denial.

Further Step 3 repairs: the host-setup fixture now provides a synthetic original-source dispatcher and checks default `=1`, explicit legacy `=0` and failure propagation before compilation (9 PASS). Its old fixture copied only the shell builder, so the new default's import of `tools/build.py` failed. The bass-tagged-storage tutorial producer now explicitly selects `=0`, matching the legacy tagged decoder it tests. Add `tests/host/test_host_setup.py` and `tests/host/bass_tagged_storage_test.sh` to Step 3's exact READY TO COMMIT file list.

Additional proof: original wheel policy PASS (`/tmp/fmb-wheels.log`); build composition 7 PASS, external-input verification unit tests 7 PASS, ROM guard 32 PASS. Frozen public project fixtures `migrate.x0x` and `migrate0102.x0x` both pass through the actual `=1` simulator (`/tmp/fmb-real-migrate.log`, `/tmp/fmb-real-migrate0102.log`).

GCC 15 fastmath and smk-clip-keys pass with warnings as errors. A separate GCC UBSan attempt failed at link time because this macOS GCC installation has no `libubsan`; that invocation is FAIL (`/tmp/fmb-gcc/fastmath-ubsan.log`). Clang sanitizer checks are reported separately.

Add `firmware/src/app/x0x.h` (portable-import API comment) to Step 5's exact READY TO COMMIT file list. The engine API comment now distinguishes music migration from incompatible legacy sound pots.

The full `=1` comparison isolated three more changed selectors. `scenario-clkmove` exposed missing clock/clip normalization on current-format settings; normalization now runs after both current and migrated loads. `scenario-edit_midi_rec_guard` exposed a real original MIDI path defect: accepted MIDI edges reached the synth but never called `seq_rec_bass`. The path now records accepted onsets and final raw releases with the existing source, view, lifetime and legato rules. `scenario-jam` assumed the old factory send of 80; its fixture now explicitly sets that send to 80 before testing the -10 gesture, so the same routing assertion applies to both profiles without changing either factory patch.

Add `firmware/src/app/engine.c` (MIDI recorder hunks) and `tests/scenarios/jam.x0x` to Step 3's exact READY TO COMMIT file list. The settings-normalization hunk in `project.c` also belongs to Step 3. Migration/default-helper hunks remain Step 5.

The migration check was expanded after inspecting the documented diagnostic revision 12 and per-note stream contracts. It now passes 1,076 checks in normal and ASan/UBSan runs, including streams 7/9/12 and portable revision 12. The earlier 944-check result records the narrower first pass.

## Step 4: final host proof and blocked target build

The final shipping host app compiles with `X0X_ORIGINAL_SYNTH=1`. The target compiler needs Linux x86-64; Docker returned `permission denied` on `/Users/mac/.colima/default/docker.sock`. The denied operation was not retried or routed around. Therefore the target firmware build is NOT_RUN, target byte equality for Step 1 is unverified, and flash/RAM free are NOT MEASURED. No target artifact, DEV CHECKED, install, device or release PASS is claimed.

Full commands used `PATH=/tmp/fmb-native:$PATH X0X_ORIGINAL_SYNTH=<0|1> sh tests/run_tests.sh`. The wrapper `/tmp/fmb-native/cc` runs `/usr/bin/cc -D_DARWIN_C_SOURCE` so the macOS SDK exposes libproc types under the host's POSIX C mode. Tools: Darwin arm64, Apple Clang 21.0.0, Python 3.9.6, GCC 15.2.0 for the separate attribute checks. The postponed-annotations fix lets the shipping builder use system Python and its installed Pillow.

The baseline at `5ed2693d` was replayed from an exported source tree: 264 PASS, 45 FAIL, 5 NOT_RUN. This does not reproduce the owner's earlier 55-failure environment. Comparison uses matching local tooling and exported-tree fixtures. The new migration selector adds one test to the candidate's total.

The complete candidate `=0` invocation initially had 264 PASS, 46 FAIL, 5 NOT_RUN. After the repaired host-setup fixture passed its scoped rerun, the result ledger is **265 PASS, 45 FAIL, 5 NOT_RUN**. The jam fixture also passes on the legacy host after its explicit starting-send change.

The complete candidate `=1` invocation initially had 261 PASS, 49 FAIL, 5 NOT_RUN. The repaired host-setup test passes all 9 cases. All 57 scenarios were rerun on the final shipping host after the clock/MIDI repairs; the three new failures now pass and the remaining scenario failures match the baseline. The final result ledger is **265 PASS, 45 FAIL, 5 NOT_RUN**. All baseline selectors are accounted for, and both profiles have **zero new failures**. `smk-clip-keys`, fastmath and Clang fastmath-UBSan pass in both profiles. The earlier incomplete runs and results-directory collisions are excluded from these counts.

Final separate checks: migration 1,076 assertions in normal and ASan/UBSan runs; original integration 210,508 assertions in normal and sanitized runs with the same audio hash `79d686b167ffdfef`; original UI 22,949 assertions and 36 matching normal/sanitized captures; original wheel policy PASS after the MIDI repair. ROM guard 32 PASS, build composition 7 PASS, external-input unit tests 7 PASS, and all 12 pinned external files MATCH. GCC fastmath and smk-clip-keys PASS; the separately attempted GCC UBSan link remains FAIL for missing `libubsan`.

Evidence is retained locally under `build/fmb-evidence/`: raw full-run logs, final per-selector result ledgers, scenario rerun status, focused checks and `source-files.sha256`. The public changed-source manifest SHA-256 is `9a177a78b30cf2247ed82e8bf3f0f0c5c2d321fb11d9c706fb229ad16aff7116`. It covers changed source/test files and excludes private inputs, generated binaries and this evolving report. Full-run scratch roots: `/tmp/fmb-base`, `/tmp/fmb-candidate0`, `/tmp/fmb-candidate1`. Final scenario outputs are in `build/fmb-final-scenarios/`.

Remaining FAIL selectors (all also FAIL in the measured local baseline):

```
project-save-preflight
editor-ota-dispatch
storage-scratch
font-pack
ble-controller-heap
smc-ui-negative
smc-drum-rs
smc-clip-visibility
breaks
gate-trap
ctl-compile
update-names
validation-inputs
dev-validate-composed
quickcheck-devbug
status-lanes
status-isolation
smk-clip-chord-display
padcal-flag
smk-clip-isolation
smk-clip-knob
rec-live
rec-live-smk
ui-feedback
home-overview
sound-matrix
chicago-presentation
scenario-blenotes
scenario-brkslice
scenario-brkslice_mode_toggle
scenario-brkslice_trs_overlap
scenario-getting_started
scenario-midiowner
scenario-patterns
scenario-perf
scenario-plock
scenario-song
scenario-sound_matrix_chicago
scenario-sound_matrix_chicago_idle
scenario-sound_matrix_lock
scenario-sound_matrix_miami
scenario-steps64
scenario-undo
scenario-usbnotes
perf-scratch
```

NOT_RUN selectors:

```
not-run drum909 (../schwung-9W9/src/dsp/er99_engine.c)
not-run drum808 (../schwung-8W8/src/dsp/sc808_engine.cpp)
not-run bass303 (../schwung-303/src/dsp/open303/rosic_Open303.cpp)
not-run chicago-au-compare (CHICAGO_AU_REFS)
not-run miami-au-compare (MIAMI_AU_REFS)
```

**Step 4 READY TO COMMIT (host evidence; target build BLOCKED).** Exact file: `FMB-GUARDS-SLOTS.md`. The existing index.lock denial still applies. The next target action belongs to the assigned integrator in an environment with permitted Docker access: build the final tree as development Elite with `X0X_DEV_PADCAL=1 X0X_ORIGINAL_SYNTH=1 ./build.sh --board smk37 --model elite`, record flash/RAM margins, and compare the attribute-only target output against its pre-guard counterpart. This is build-only work; it grants no device action or push.

Final Git checks: `git diff --check` PASS; the index is empty. Only explicitly selected public guard files were passed to the failed staging attempt. No private header or binary was staged, committed or pushed. All `experiments/wobble/` inputs remain unchanged. Step 1 is already committed; the remaining READY file lists identify the intended separate commits; `project.c` and `engine.c` require their Step 3 repair hunks and Step 5 migration/default-helper hunks to stay separate.

## Observed local commit

At the final Git readback, HEAD had advanced to `0ff43a8b3deb5c267e37624dd90d239b17c846f9`, a Step 1 commit containing only `firmware/src/dsp/fastmath.h`, `firmware/src/app/smk_clip_keys.h` and `firmware/src/app/smk_clip_arp_share.c`. This session did not retry the denied staging/commit operation. The committed guard bytes match the source manifest and checked bytes. The report itself remains uncommitted. Steps 2/3/4/5 retain their READY file lists; the prior denial is not treated as permission for another Git write.

## Follow-up: full-access continuation

The current owner instruction supersedes the historical index.lock and Docker blockers for local commits and builds. HEAD at intake is `457e7094`; the lane is `codex/fmb-guards-slots`. The configured author is genkigenki. No supported project-chat reader is available; work continues in the assigned lane. Ordinary workflow applies.

GCC 15 `-O0` verification: fastmath and smk-clip-keys each compile, link and run with `-Wall -Wextra -Werror` (2 PASS, 0 FAIL). The clip-key link includes `smk_clip_arp_share.c`. Logs: `build/fmb-evidence/followup/gcc-fastmath-o0.log` and `gcc-smk-clip-keys-o0.log`. The earlier source manifest matches every remaining source/test edit; only fastmath differs because of the committed `FM_ONE` correction.

Step 2 commit scope: `firmware/src/app/ui.c`, `firmware/src/app/main_fm1.c`, `FMB-GUARDS-SLOTS.md`. Existing original-control presentation and slot identities are reused. Prior actual UI proof remains 22,949 assertions and 36 matching normal/sanitized captures; fresh final host and target proof follows after the source commits. Miami dependencies under `experiments/wobble/` remain unchanged.
