# F2 recording patch comparison

Comparison date: 2026-11-01 (Asia/Vladivostok).

## Outcome and scope

The five available recording commits have no missing recording behavior identified by patch comparison against `build/0.0.23` at `bb1907494b8d1b52d52ac220dfeb9a0663ea3ee2` (tree `e340d89d180879b8d8a2fea07c56800a2feaac5c`). Their recording code is already represented, sometimes with newer safeguards. This recording comparison ports no firmware hunks. It records the comparison and the unresolved F2 acceptance work, rather than applying duplicate or superseded code.

The owner explicitly permitted skipping `a91e1a433`, `24373f293`, `494ca491a` and the unavailable `restore-features` branch. They were not reconstructed or replaced with invented implementations.

This is a source comparison, not a runtime acceptance result. The broader six-row F2 brief remains open. No engine, UI, sequencer, recording, project format, test harness, build configuration or device behavior changes in this recording comparison.

N6 was subsequently added to PR #791 with a host regression and suite registration.
See [N6 comparison and executed checks](n6-pattern-base-comparison.md); statements
about no new tests below describe only the five-commit recording comparison.

## Method and provenance

1. Resolve the base through the connected GitHub API and fetch the five commits with their patches.
2. Fetch the union of their 19 changed files at the immutable base SHA.
3. Reconstruct the original Git trees and unsigned commit, verifying the exact base commit and root tree hashes. Verify every downloaded comparison file against its tree blob hash. The local checkout is sparse; this does not claim a complete build input closure.
4. Compare the added lines of every hunk against the current file. Inspect nonmatching additions in the surrounding functions, including the old code removed by later patches. Textual presence alone is insufficient: queue readers/writers, record masks, cancellation callers, automation playback and test registrations were inspected.
5. Preserve newer behavior. Ancestry is not equivalence: GitHub compares these original commits as diverged from the base. No `git cherry` result is claimed because the original commit histories were not downloaded into the sparse checkout.

Native Git transport failed before a server response because the configured proxy was unreachable. The connected GitHub API supplied verified source bytes; it was not used to bypass a permission or policy denial.

## Commit decisions

| Original commit | Recording behavior and current evidence | Decision |
|---|---|---|
| [0ea7319bada2ac8d09fa144dbbf8c32c49007c34](https://github.com/gobbajogger/X0X37-for-Loopy-Pro/commit/0ea7319bada2ac8d09fa144dbbf8c32c49007c34) | All added lines are present. `engine.c:midi_in` calls `seq_rec_bass` for accepted original-synth MIDI edges; source lifecycle cleanup remains present. `BEHAVIOR.md:REC-07` is present. | Already represented; no port. |
| [ac2e726e8f1cadaac54039a28007f69a6de08d11](https://github.com/gobbajogger/X0X37-for-Loopy-Pro/commit/ac2e726e8f1cadaac54039a28007f69a6de08d11) | `engine.c:original_rec_cancel_cut`, keyboard-release handling, `original_runtime_cut_source`, stale counts, all-off cleanup and the endpoint regression changes are present. The simple stale-off hunk evolved into `runtime_stale_tick`, `stale_ttl` and `stale_pending`: an ambiguous off is bounded to avoid a stuck fresh note, and `original_rec_recover_off` reconciles recorder state. The commit's unrelated Solo/corrupt-patch/UI/test-fixture additions are also present; no F1 or UI changes are imported. | Already represented; preserve later bounded recovery. |
| [d951fc18765ac5080c8f257256d976f847574615](https://github.com/gobbajogger/X0X37-for-Loopy-Pro/commit/d951fc18765ac5080c8f257256d976f847574615) | `engine.c:rec_ev_put/engine_rec_ev_get`, the 909/808/BREAK MIDI producer calls, `engine.h:REC_EV` and reader declaration, `ui.c:drum_rec_write/brk_slice_write/ext_rec_drain` and the external-MIDI host test are present. The ring now stores `{event,generation}`, rejects REC-off input and discards stale generations. The UI validates and writes each event inside `plat_cq_lock/plat_cq_unlock`. Selected-part/SEQ-only restrictions were superseded by the next commit. | Already represented; preserve generation and transaction guards. |
| [3ad78ea634337e805d03c4bd7637742860ef12cc](https://github.com/gobbajogger/X0X37-for-Loopy-Pro/commit/3ad78ea634337e805d03c4bd7637742860ef12cc) | Added engine/sequencer/header/test lines are present. `rec_ext_mask` enables both bass parts for external input while keyboard capture retains `rec_mask`. Drum/BREAK writes use `pat_of(part)` and `ext_rec_drain` has no selected-part/view gate. Its old standalone guard is replaced by the locked transaction above. `test_all_parts` remains in the external-MIDI host test. REC-07A now also documents stale-onset generation rules; REC-07B remains present. | Already represented; no port. |
| [8401fc231e9e9774422ff478060da408268cd6da](https://github.com/gobbajogger/X0X37-for-Loopy-Pro/commit/8401fc231e9e9774422ff478060da408268cd6da) | Added production lines in `engine.c`, `engine.h`, the physical wheel handler and host entry point are present. `perf_apply`, `mod_apply`, `mot_base`, `mot_apply`, `mot_restore` and wheel/external MIDI paths retain bend/mod/damper automation. The test and runner are present; `perf-wheel-motion` is still registered and classified in `tests/run_tests.sh` (the classifier line has additional selectors). BAS-11 additionally specifies exact neutral bend. | Already represented; no port. |

The older FM-1 test-runner variant in `d951fc187` was removed by `3ad78ea63`. The base retains the SMK-only runner and `ext-midi-rec-smk` registration. It must not be restored. No test check is weakened or deleted here, and no new test is introduced.

## F2 rows and remaining work

All row observations below refer to the pinned base. None is a new runtime PASS.

| Row | Source observation | Port / acceptance disposition |
|---|---|---|
| 8: full sound lock on one note and marker | `bass_note_sound.h` defines the saved payload; `ui.c:bass_write` captures/clears it under `X0X_BASS_NOTE_SOUND`; sequencer note-sound paths are conditional. `tools/build.py` accepts this opt-in flag and requires tagged sound with it. | **Unported.** This recording comparison does not enable NOTE_SOUND, recover its last working historical implementation, or add the step marker. Historical recovery, full playback/save/load acceptance, RAM measurement and real 240x240 framebuffer BEFORE/AFTER remain required. Owner approval is required before merging a future visual change. |
| 15: keep cutoff locks on navigation | `ui.c:set_view` preserves the SEQ-to-EDIT held-step path, blocks return until release and otherwise calls step-cancel helpers. `lock_release` restores stopped previews. | **No new port; acceptance open.** The five patches do not establish navigation/cutoff-lock completeness. Add or run the targeted navigation regression before marking this row done. |
| 16: gesture owned by source part until release | `ui.c:bass_step_cancel` releases the captured `held_step_part`; drum cancellation checks the captured drum. | **No new port; acceptance open.** These paths are source evidence, not proof of every press/navigation/release sequence. The broader source-part ownership brief is not certified here. |
| 17: safe motion/record lifecycle | The five patches' external recording, cancellation and performance automation behavior is present. `seq_advance` cancels invalid recorder contexts; newer queue generations and bounded stale-off recovery are retained. | **Recording patches already represented.** Full motion/record lifecycle acceptance remains open; no runtime suite result is claimed. |
| 18: one internal/external clock | `sequencer.c:clock_source/seq_advance` select the clock mode, clear external pending ticks in INTERNAL, lock an external owner, hand over in AUTO and pause on lost EXTERNAL clock. | **No new port; acceptance open.** These patches contain no missing clock implementation to recover. Clock handover, disconnect and boundary regressions still need executed proof. |
| 23: separate 909 and 808 models; registry | `pattern.h` has independent `drum[NKIT]` rows with `NKIT=2`. `engine.h` has distinct targets and part IDs. Existing `drum_kit_identity_test.c` covers eleven-voice identities, independent hits/accent/sound and copy behavior; its runner is registered as `drum-kit-identity`. | **Source already present; registry clarified by this recording comparison.** No model implementation is ported. The identity test was not executed here; full acceptance remains unverified. |

## Verification and delivery limits

| Check | Result |
|---|---|
| Exact base commit/tree and comparison-file Git blob identities | PASS: reconstructed source matched the pinned Git identities. |
| Five-commit source/patch comparison and all nonmatching-hunk inspection | Completed source review; no missing recording behavior identified. This is not a runtime test. |
| Documentation checks | See the PR validation receipt for executed checks. |
| New or changed runtime tests | None; documentation-only change. Existing registrations retained. |
| Full host suite, external-MIDI recording, endpoint lifecycle and wheel-motion execution | NOT_RUN. The sparse checkout does not provide a complete app/private-synth build closure. |
| Elite, SDK V1.2.1, X0X_DEV_PADCAL=0 | NOT_RUN; free static RAM not measured. |
| Pro, SDK V1.2.1, X0X_DEV_PADCAL=0 | NOT_RUN; free static RAM not measured. |
| RAM impact | No firmware source/configuration changes. The owner's 2352 B baseline is reported context, not a measurement of this recording comparison. |
| Mockups / framebuffer PNGs | None. No visual change is implemented. |
| Hardware, install, merge, release or publication | Not performed or authorized by this recording comparison. |

Next work: recover and validate row 8 separately from this comparison; execute the targeted acceptance tests for rows 15-18 and 23 on a complete exact checkout. Keep all six rows open until their own evidence meets the owner's brief.
