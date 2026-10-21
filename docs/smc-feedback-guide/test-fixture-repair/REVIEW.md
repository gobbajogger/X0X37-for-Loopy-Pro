# Clip-visibility fixture repair — independent review

**Verdict: approve this isolated test-only correction.** It preserves visibility and no-replay checks; it does not change firmware behavior or claim a hardware PASS.

**Build/source:** `4cf8c2f1d641bf91041a39f0d34d77cd51fddbf8`. Reviewed independently on 2026-10-21. The correction is supplied as [fixture.patch](fixture.patch) and the [resulting fixture](smc_clip_visibility_test.c). Nothing was applied to the active reference checkout.

**Replaces:** stale direct-`mark_dirty` expectations in `tests/host/smc_clip_visibility_test.c`, not the production persistence implementation.

**Checked against decisions:** visible controls must edit only their current owner; covered inputs must be drained rather than replayed after an overlay closes. Keep `-Wall -Wextra -Werror` intact.

## Cause and exact correction

The test extracts production `smk_ck_smc_turn`, SMC queue/dispatch and knob routing, but substitutes `ck_get`/`ck_set`. Current `smk_ck_smc_turn` calls `ck_set` only when a value changes (`firmware/src/app/main_fm1.c:1360–1380`); it no longer calls `mark_dirty` directly. Production `ck_set` owns persistent-setting dirty marking (`:1305–1338`). Its ON and LATCH branches return before that marking because those flags are runtime-only.

The old fixture still defines unused `mark_dirty`, so normal compilation fails under `-Werror`. Removing that function alone is insufficient: exactly nine assertions still expect the old dirty counter to increase, one for each of three clip pages on each of three routes.

The proposed patch removes the unused stub and changes the counter to `field_writes`, incremented by the substituted `ck_set`. It asserts one setter call for a visible first-field edit and zero setter calls for covered fields. The first field is ON, making a blanket dirty expectation particularly inappropriate. This is routing/setter-observation coverage, not proof of autosave or production `ck_set` behavior.

## Assertions preserved

The diff changes only the counter name, substituted setter accounting, unused stub, and associated comparisons. It retains every test route, page, check label and other assertion:

- BLE DAW, BLE User, and host User routes.
- All three clip editors and clip-grid/X0X fallback contexts.
- Exact before/after field-array comparisons.
- Expected menu fallback turns and fallback page-value changes.
- No notification, fallback or activity-time changes under help/confirmation.
- Both SMC queue read/write-index equalities after covered input.
- Closing the overlay must not replay field writes, fallback turns or page-value changes.
- Visible-stage resumption and visible-X0X fallback behavior.

The menu fallback in this fixture is intentionally a permissive stub: all eight test turns reach it. This fixture does **not** assert that eight real menu fields are editable; the actual six-row map is tested by `smc_mapping_test.py` and documented separately.

## Independent run and negative controls

Run the ordinary repaired test:

```sh
sh smc-feedback-guide/test-fixture-repair/run.sh /path/to/reference-smc-integration
```

Reproduce the independent review controls and save logs:

```sh
python3 smc-feedback-guide/test-fixture-repair/review.py \
  /path/to/reference-smc-integration /path/to/review-logs
```

`review.py` uses a temporary directory, applies the patch to the exact original fixture, checks byte equality with the supplied repaired fixture, and copies the reference test script unchanged. A separate positive run adds only `-DX0X_SMC_VENDOR_TEST=1` through a compiler wrapper to cover the current TEST queue/role branch too; warning/error flags stay intact. It uses only the required source files and deletes all compiled binaries/negative-control source copies on exit. It does not run a target toolchain or vendor archive.

| Case | Result |
|---|---|
| Original fixture | Compile fails: unused `mark_dirty`; `-Werror` intact |
| Remove only unused function | Compiles; 129 passes, exactly 9 stale assertions fail |
| Proposed patch, default host profile | Compiles; **138 passes, 0 failures** |
| Proposed patch, `X0X_SMC_VENDOR_TEST=1` | Compiles; **138 passes, 0 failures** |
| Hidden-editor negative control: bypass menu guard | Compiles; 117 passes, **21 failures**, including covered-field ownership |
| Replay negative control: return before draining help/confirmation input | Compiles; 93 passes, **45 failures**, including post-overlay replay |

[Review summary](../validation/independent-review/fixture-review.log), [baseline](../validation/independent-review/baseline.log), [remove-only](../validation/independent-review/remove-only.log), [repaired](../validation/independent-review/repaired.log), [TEST-profile repaired](../validation/independent-review/repaired-test-profile.log), [hidden-edit control](../validation/independent-review/negative-hidden-edits.log), [replay control](../validation/independent-review/negative-replay.log), [exact patch application](../validation/independent-review/patch-apply.log).

These negative controls prove that the repaired fixture still detects the specific hidden-edit and retained-event regressions; they are not applied firmware fixes. No script flags or thresholds were relaxed.

## Integration boundary

Receiving owners: `/root` for publication decisions; `/root/integrate_security_recovery` for optional inclusion in the reviewed source package. The native reference checkout remains clean. Apply only `fixture.patch` to the corresponding test file if its base still matches; the standalone guide directory does not alter firmware or trigger a target rebuild. Reuse matching mapping evidence only after the integrator checks its inputs. Installation and hardware behavior remain untested here.
