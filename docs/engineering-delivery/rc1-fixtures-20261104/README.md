# RC1 fixture repairs: ROADMAP QA2 subset

The stale release UI, sound matrix and glyph fixtures could not compile against preserved RC1 source. This test-only repair restores executable checks of the UI owners already implemented, so qualification can catch regressions before a candidate reaches the player. It changes no firmware behavior.

ROADMAP QA2 at #812 commit 496c87269349b70f9a900bd95b996d765aa51a80, line 300. These three fixtures are only a subset of QA2; neither QA2 nor RC1 qualification is closed.

## Exact source

- Base branch: chat/025-rc1-source-c32c54ac-20261103.
- Base commit: c32c54ac8ec68aaca53adccf7af66f155590f2dc.
- Base root: ac490f7fae9198748aa54bd914d42f99d45444f9.
- Fixture-only composed root: e27c281bcbdbcd0f9040db294a7a21598360ddf4. This is a Git tree object, not a new commit or firmware build. Delivery documentation produces a different final complete root, recorded in the handoff.
- [source-inputs.json](source-inputs.json) identifies every repaired fixture blob and SHA-256; the fixture diff digest is b016b6f3f34160a705604e0cc8d02ac2ee6891b2888747abe78e8d88486cd61c.
- Exactly three tracked test files change; production, shared runners, #823 and ui_feedback_test.c remain untouched. No other repair PR is composed into this source.

## Verified causes and intended behavior

1. original_release_ui_test.c called removed header_transport and expected obsolete normal-header content coordinates. Merge 314f181f intentionally reconciled #807 with #800/#788: the normal header uses the position badge, the Part header retains the queued content picture, and the play triangle uses seq.metro_view. The repaired fixture keeps content/bounds/state-preservation, absent/equal cue, stopped-stability and playing-step checks at the real Part-header position. It exercises both real header paths with STOP and external-clock WAIT neutral, phases 1/4 white, phases 5/32 highlighted, independent of UI frame and track playhead. Existing footer/direction assertions stay.
2. sound_matrix_test.c used adsr_inline_page and omitted the page argument to adsr_values. Production 6ab7b2dac7b7fffc9a2fcde6b4ec421bc54e93b7 introduced adsr_row(page) and adsr_values(e,page), allowing the Chicago envelope row to render when another row is active. The repair passes the real row and adds an inactive-row assertion; all native envelope laws, geometry, locks, motion and no-Miami-ADSR checks stay.
3. ui_glyph_polish_test.c retained the three-argument draw_adsr_inline and omitted the adsr_values page. The repair supplies page/active/y/height/offset without changing expected RGB565 geometry or labels. The preceding CUTOFF sweep also left cutoff at 127, clipping two positive ENV target ticks to the same column. Centring cutoff before the ENV sweep preserves the original distinct-shape assertion. No production clamp or visual design changed.

Test diff: +60/-24 across three fixtures. No production changes were needed.

## Evidence

The root baseline selected ten UI suites at the exact base: the four affected selectors failed compilation; six adjacent UI selectors passed. [baseline-selected.json](baseline-selected.json), [baseline-selected.log](baseline-selected.log), and [baseline-errors.log](baseline-errors.log) retain command, exit and diagnostics.

The isolated fixture runs produced these repaired checks; [worker-checks.json](worker-checks.json) records exact commands and mode details:

| Check | Result | Meaning |
| --- | --- | --- |
| original-release-ui | PASS, 35,996 checks / 97 captures | Normal and ASan/UBSan; exact log/PNG equality |
| ui-theme | PASS, 14,865,102 checks / 259 captures | Same shared fixture, production SMK renderer; normal and ASan/UBSan equality |
| sound-matrix | PASS, 15,161 checks | Normal and ASan/UBSan; 15,160 original checks retained plus inactive-row contract |
| ui-glyph-polish | PASS, 16,828 assertions | Normal and ASan/UBSan; original pixel expectations retained |

Three disposable preprocessed production mutants compiled successfully, then failed the intended release assertions: track-step phase, neutral-state blinking, and wrong queued content. The temporary selected-row-only ADSR wrapper also fails. Removing the ENV cutoff reset reproduces the fixture-state collision. These are executable negative controls, distinct from the additional wrong-expected-value assertion controls; see the linked runtime excerpts and worker receipt. None changes tracked production source.

The root runs the complete sh tests/run_tests.sh command with X0X_TEST_JOBS=4, X0X_TESTS empty, on the composed three-fixture input. Its result is pending while execution continues and will be retained as a separate receipt. The independent Sol reviewer ran the four affected selectors in an isolated exact-input checkout: all four PASS, exit 0. Independent source review approved the unchanged +60/-24 fixture diff with no findings. See [sol-focused.log](sol-focused.log) for the independent focused run. The negative controls were inspected but not independently rerun. Do not interpret the scoped positives as a full-suite pass.

## Scope and remaining gates

[PLAN.md](PLAN.md) records the test repair and acceptance. The public source is already present at the pinned canonical repository commit. Only the three fixture changes and sanitized technical evidence are proposed.

Preserve [#816 independent QA HOLD](https://github.com/gobbajogger/X0X37-for-Loopy-Pro/pull/816#issuecomment-6555217066) and all existing full-host, audio identity/performance, Elite/Pro native, memory/pool, package, rights, ROADMAP, publication and hardware gates. Native target builds, device operations and release/publication checks were NOT_RUN here. Missing/failed/skipped coverage is never PASS.

## Integrator handoff

Consume the reviewed exact three-fixture change from the QA2 repair PR and its remotely verified head/tree; preserve #834-#838 and #823 as independently owned changes. Apply only through the existing integration route. On the selected combined source, rerun the four affected selectors and reconcile the remaining QA2/full-host failures and unavailable checks. RC1 independent QA still decides qualification. A PR is not merge or release acceptance.

## Post-Deploy Monitoring & Validation

No additional operational monitoring required: this change affects host fixtures only. The consuming QA owner should require all four affected selectors to pass on the chosen combined source and verify the firmware inputs remain unchanged by this repair.
