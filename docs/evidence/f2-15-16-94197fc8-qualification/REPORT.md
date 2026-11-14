# F2 #15/#16: qualification FAIL on 94197fc8

Both rows remain open. New production-path host regressions reproduce cutoff preview restoration defects (#15) and premature held-note cancellation (#16). No firmware or UI implementation was changed.

## Inputs and evidence recovered

- Repository: gobbajogger/X0X37-for-Loopy-Pro. Exact source: 94197fc839b00aff47b98ca02f3c6396e4a6eabf; tree 72d28fbc741244d18fa3d18495da16ab0fcb1d24. All 5,345 Git blobs matched the GitHub API tree before and after execution, zero omissions or differences.
- Canonical ROADMAP: 32b0cacd17c89f23bb2f16915732750d158b33e7, blob b3682dcbc5a720b99cadf67089e3f50393fb3e2d. Accepted historical map: d5c8c9d2f653066c01ecad7fb5a5a7ad2310a1a6. Selected rows retained in historical-selected-map.json.
- F2 audit blob 7d27858d3926146b27ddf0ebae39bb577fd7f8d0 consumed. Its historical NOTE_SOUND opt-in assumptions do not describe the later implementation. Current engine.h enables note-sound support; GLO > AUDIO > NOTE LOCK controls runtime capture. No old flags or decisions were restored.
- PR791 comment6552099499 covers N6 motion restoration and provenance only, explicitly excluding #15/#16 acceptance. Reused as scoped historical evidence; not rerun.
- PR810 comment6550858606 reports 17,957 live-key checks at 45a7285e, with normal and sanitizer passes. Its predecessor comment6549061580 documents the corrected per-voice wait assertion and cancellation negative controls. Recovered in receipts-810.json.
- Five production definitions (ui_live_key_owner, ui_live_cancel_voice, live_edge, live_boundary, live_central_event) are byte-identical between 45a7285e and 94197fc8. This supports retaining their earlier cancellation rationale, not upgrading it to full combined acceptance. Older owner_switches() explicitly expects cancellation on part/view/modal ownership loss. That is a documented mismatch with the unfinished HOLD-NOTES-UI requirement, not a newly introduced regression attributed to 941.
- Exact-941 PR869 comments6846929049/6847431743/6847451235 retain d408 production-input reuse and four changed shell-fixture passes. No complete suite was rerun here. Their broader native/performance/package claims were not used as #15/#16 runtime acceptance.
- Existing held_step_release_test.c covers SEQ → EDIT → same-page release across kits/voices and step banks. New fixtures cover different page/list/context transitions. Old F6a and 0.0.24.1 summaries were inspected as historical inputs, not relabelled exact-941 results.

## Executed results

Linux x86_64; GCC 14.2.0; Python 3.12.14. python3 tools/check_original_inputs.py passed: 10 active inputs, manifest 9c7ac9b0a71e775fb7009e2d4035fce827b2c56c4e8e692a7a8910c9c6c881bc.

| Check | Result |
|---|---|
| step-final.log | FAIL, exit 1: 426 assertions, 12 failed; 36 navigation cases, 12 fail |
| step-sanitized.log | FAIL, exit 1: same 426/12; no reported ASan/UBSan diagnostic |
| live-normal.log | FAIL, exit 1: 408 assertions, 20 failed; 40 navigation cases, 20 fail |
| live-sanitized.log | FAIL, exit 1: same 408/20; no reported ASan/UBSan diagnostic |
| Source preservation | PASS: 5,345 original blobs unchanged after execution |

Failures are behavioral assertion failures, not compiler or sanitizer failures. Initial scratch helper-name collisions were corrected before these recorded executable results. The final live fixture adds diagnostic PASS lines after the normal run; its assertions and tested production inputs are unchanged, and the sanitizer run uses that final fixture.

## #15 minimal defect scope: stopped preview restoration

Repro: stopped SEQ, hold an existing bass step, tap EDIT, turn the cutoff knob, then either (a) tap EDIT to another page, (b) tap SELECT to open the list, or (c) open the list and select another page; release the step.

Expected: keep the saved cutoff lock and restore the live cutoff to its exact base after release. Actual: saved lane bytes survive, held-step state clears and note releases, but the edited cutoff remains in the engine. All three paths fail on Chicago/Miami × parts A/B: 12 failures.

Example Chicago A: base bits 3f010204 (64/127), post-release 3f0b162c (69/127). Miami B: base 3f010204, observed 3f376ede. Values are actual engine macro bits from engine_original_value_bits(), not UI text.

Source: ui.c:2995–3019. lock_release() enumerates only pg.r[cur_page()] rather than the edited gesture's references. A different page misses cutoff. bass_step_cancel() calls restoration only when lock_step() >= 0; an open list makes lock_step() return -1 at ui.c:2844–2851, so restoration is skipped.

Passing controls: same-page release, HOME cancellation before leaving EDIT, part change through EN_ALGO, blocked EDIT → SEQ while held, pattern change, HELP release. Other-part cutoff remains unchanged; saved lane bytes remain unchanged; no held bass key remains after release in all 36 cases. Older-step release does not cancel the successor held step for 909/808/A/B; final release clears it.

Owner action: fix restoration against the captured gesture owner and edited references, retaining lock bytes, exact base values, source-part isolation and existing cancellation semantics. This qualification does not assign the shared UI writer or implement that fix.

## #16 minimal defect scope: note retention across ordinary UI

Repro: EDIT with Free Play active, press physical key17 and keep it down, then:
- open a settings list with SELECT and close it with HOME;
- change part with EN_ALGO;
- open GLO;
- open FX;
- move SMC fader0 to display the real production popup.

Expected for this unfinished requirement: the source note remains owned/sounding until physical release. Actual: source engine gate and source token become zero before key-up. These five paths fail for Chicago/Miami × parts A/B: 20 failures. Closing the list with HOME returns to the same EDIT page yet still kills the note.

Source: ui.c:4279–4306 and 4360–4380. ui_live_key_owner() excludes HOME/SAVE, other views and active smc_ui_active(). A changed owner makes live_boundary() call ui_live_cancel_voice(), publishing cancellation, discarding the held inventory and suppressing keys until release. Part changes use the same cancellation boundary. This is consistent with older safety tests; it does not satisfy ownership through ordinary navigation.

Passing controls: opening the settings list, EDIT paging, SEQ navigation with Free Play, octave change and pattern navigation retain the held source note. All tested physical final releases leave both synth gates/key masks clear. Delayed/duplicate old A release leaves a fresh B key sounding; simultaneous older/latest releases leave no note. No wrong-part note or hanging note was observed in these bounded sequences.

Owner action: preserve the admitted press-time owner through allowed ordinary UI navigation/popups while retaining cancellation for genuine project/engine/transport invalidation and stale-event safety. The existing firmware/UI owner and #891 must disposition the navigation contract; no competing implementation or planning owner was created.

## Commands and reproduction

From the exact source directory:

SMC_UI_TEST_FILE=/workspace/f2-15-16-qualification/step-navigation.c SMC_UI_OUT=/workspace/f2-15-16-qualification/step-final sh tests/host/smc_ui_test.sh

SAN_FLAGS='-fsanitize=address,undefined -fno-omit-frame-pointer' SMC_UI_TEST_FILE=/workspace/f2-15-16-qualification/step-navigation.c SMC_UI_OUT=/workspace/f2-15-16-qualification/step-sanitized sh tests/host/smc_ui_test.sh

X0X_HOST_DEFS=-DX0X_LIVE_KEY_TEST=1 SMC_UI_TEST_FILE=/workspace/f2-15-16-qualification/live-navigation.c SMC_UI_OUT=/workspace/f2-15-16-qualification/live-normal sh tests/host/smc_ui_test.sh

X0X_HOST_DEFS=-DX0X_LIVE_KEY_TEST=1 SAN_FLAGS='-fsanitize=address,undefined -fno-omit-frame-pointer' SMC_UI_TEST_FILE=/workspace/f2-15-16-qualification/live-navigation.c SMC_UI_OUT=/workspace/f2-15-16-qualification/live-sanitized sh tests/host/smc_ui_test.sh

Portable replay: python3 replay.py --source /path/to/exact/source --case step [--sanitize], or --case live. The wrapper records the actual exit and selected environment values. Exit 1 is expected until the behavioral defects are resolved.

## Evidence boundary and next consumer

These are native-compiled Linux host checks using production ui.c, engine.c, sequencer, motion and smc_mixer.c. The physical transport is extracted from the pinned HAL/platform functions by the existing freeplay fixture generator; electrical key samples and timing are substituted. The inherited smc_ui fixture substitutes hardware, clip storage/painter and device endpoints. Real SMC queue/popup dispatch and synth DSP execute. This is not instruction-set firmware emulator evidence, target SDK evidence, radio/ADC evidence, audible device qualification or physical hardware acceptance. The fixture inherits X0X_DEV_PADCAL=1; it is not a PADCAL=0 target build.

No complete F2, REC full-keybed, performance or UX audit ran. No SDK access remedy was needed: required host inputs were already valid. No physical action, shared source edit, merge or publication occurred. Native target/emulator/device, save/load recall and full engine/SMC gesture matrix remain outside this bounded qualification.

State: FAIL / owner-actionable qualification delivered to creator. Branch: no new Git branch; source archive pinned as above; qualification artifacts NOT PUSHED. PR: none. Existing shared checkout remains clean.

Supported-tool discovery found no thread_management.create_thread/read_thread or saved thread-configuration management tools. environment_status exposed this instance's saved configuration IDs only. No successor was created, browser creation attempted, uncertain admission retried, or independent handoff claimed. NEXT-STAGE.json returns the concrete repair/requalification packet to the creator; receiver execution is NOT_OBSERVED.
