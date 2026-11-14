# REC1 / REC37 #896 — F1 test-only admission qualification

**Completed at pinned head `8b0b6d311577fcc303212016423dd4f5ea4e8184`.** The substituted predicate fails production conformance in **2,240 of 5,760 fresh-press cases** and **28 of 32 held-view transitions**. The exact production closure passes the new source-derived expectations: **59,354 structural assertions, zero discrepancies, process exit 0**. Independent correctness review approved the frozen test-only patch.

This establishes a fixture-model discrepancy and the current feeder's admission behavior. It does not establish a production repair, recorded pattern contents, audio, firmware or device acceptance.

## Intake and authority

Canonical repository: `gobbajogger/X0X37-for-Loopy-Pro`. Existing [PR #896](https://github.com/gobbajogger/X0X37-for-Loopy-Pro/pull/896) remains open, draft and unmerged. Its branch is `wip/rec37-compact-continuation-20261113`; base `e81b2cf6d2eb322d697a0a45b0b98c68c9d757c3`. Final remote readback at **2026-11-14 10:04:49 UTC** still returned the required full head. The local patch is **NOT PUSHED**; no PR was created or changed.

The first actual task consumption was the successful read of both specified Library reports, followed by pinned public source reads. Both full report reads returned `has_more=false`. Only these expressly named private reports were brought into the isolated workspace. Their exact bytes verified:

| Input | Library identity / version | Bytes | SHA-256 |
|---|---|---:|---|
| Source report | `libfile_fde3_f-2ef875dfe4dc8191a49913222fbb6240_2021479872048191acbfdca1f870a539`, version 0 | 47,512 | `eab907096a75cbf3e8d8d41da92187ab7a6e7c69581f5ce8917706224873d18f` |
| Independent acceptance | `libfile_fde3_f-94b18e3640ec8191b3561d034a2ad454_2021479872048191acbfdca1f870a539`, version 0 | 20,210 | `7f0f763681021fe2792311cc5f1f79268ffeb09ae9bcd2f8a6cb1a28b816eaa2` |

Target AGENTS, task/routing/ownership/writing/workflow instructions, minimum local proof, feature registry and the existing REC plan were read at this pin. Maintained `nightly` resolved to `66073c68dd80d51a7053432c2c161a8b720a1341`; its AGENTS and workflow were read. The live delegated test-only authority governs this bounded execution and supersedes conflicting historical execution-location text. The plan is `PLAN.md` in this same REC1 packet; no new request or shared ledger was created.

The incumbent REC writer retains production and branch ownership. Its release was not inferred. All new files are under `/workspace/rec37-896-f1-qualification`. No existing checkout or live test was modified. Selected public text intake is a scoped fixture input, not a reconstructed full checkout. The earlier Project raw-byte refusal remains binding; neither SDK/source-archive materialization nor an alternate transfer/actor was attempted.

## Exact source closure

Every selected public file was fetched with the full immutable ref and its local Git blob hash recomputed. Complete size/SHA-256/Git receipts are in `source-receipt.json`; the probe's narrower closure and extracted fragment hashes are in `runs/input-receipt.json`.

| Probe input | Exact Git blob |
|---|---|
| `firmware/src/app/ui.c` | `5af70e36fe02e05287760a8e182c80d828d650b9` |
| `firmware/src/app/rec_outer_keymap.h` | `a23a24ae6c2eb7f40f4b1b41eeadf208a9159b8a` |
| `firmware/src/app/engine.h` | `9feda5fee976b901a269a7d13e7a0bc4d78aecad` |
| `firmware/src/app/plat.h` | `b57f2737c5090feac762286794c3f4b9fa1b28ae` |
| `firmware/src/seq/pattern.h` | `9810f8977dccb726e09d34666657f1e01bf83b1e` |

The new probe extracts unchanged production `part_is_303`, `is_303`, `bidx`, `kbd_on`, `live_token`, `bass_kbd_physical`, the feeder and its static state. It extracts the actual view/overlay/part/button enums and NBASS definition. Production predicate:

```c
static int kbd_on(void) { return is_303() && (ui.view == V_PART || ui.rec || ui.freeplay[bidx()]); }
```

The baseline injects exactly the historical fixture expression from blob `14c88ca4d011f9793f5693d99cd318812b87b57f`:

```c
static int kbd_on(void) {return ui.view==V_SEQ || ui.view==V_PART;}
```

All other generated source is identical except the baseline-selection macro used by model-specific structural assertions. This compares the predicate substitution under the same production closure; it does **not** rerun the original 4,304-check fixture.

Expected behavior comes from the actual code. With a synth selected and REC on, production `kbd_on` is true in all nine views. Provided busy/question/performance-test/clip guards are clear, the feeder admits a fresh outer press. With REC off, the feeder admits none even if the keyboard predicate itself is true. The exact physical wrapper passes `ui.view == V_SEQ` as `step_record`: SEQ has recording intent; every other view has audition intent. The actual downstream writer was inspected, but is mocked here. No audible sound or pattern-write count is claimed.

## New execution only

Compiler: **Debian GCC 14.2.0**; flags `-std=c99 -O1 -g -Wall -Wextra -Werror`. Full compiler identity, exact commands, generated-C hashes and individual exits are retained in `runs/`.

Each variant covers:

- **360 predicate rows:** nine actual views × five actual parts × REC off/on × all four two-slot Free Play masks. These also check synth classification and selected-slot `bidx`.
- **5,760 fresh-press scenarios:** nine views × both synth slots × REC off/on × stopped/playing × no modifier/HOME/SAVE/both × no overlay/settings-list overlay × all ten outer keys. Free Play flags are zero in this matrix. It checks single admission, pitch/token, duplicate scan, wrapper intent and mocked release.
- **32 held-view transitions:** SEQ to each other view and back, both slots and transport states. Hold p0, change view, press/release p36 there, return to SEQ, release and freshly press p0. The two models' different cancellation/rearm behavior is retained.

| Final new result | Substituted baseline | Exact production |
|---|---:|---:|
| Compile exit | 0 | 0 |
| Runtime exit against production conformance | **1 — FAIL** | **0 — PASS** |
| Structural assertions against each model's mechanics | 45,802 | 59,354 |
| Structural assertion failures | 0 | 0 |
| Predicate rows differing from production | **136 / 360** | **0 / 360** |
| Fresh admissions differing from production | **2,240 / 5,760** | **0 / 5,760** |
| Held transitions differing from production | **28 / 32** | **0 / 32** |
| Mocked onset / release callbacks | 708 / 708 | 2,976 / 2,976 |
| Wrapper SEQ / audition intent callbacks | 384 / 324 | 384 / 2,592 |

The driver exits 0 only after requiring the baseline's real exit 1 and the exact version's exit 0. That driver success does not relabel the baseline as conformant. The discrepancies are overlapping observations of one model error, not thousands of distinct defects.

For each view, the REC-on subset has **320 fresh scenarios**; all 320 REC-off scenarios reject admission in both variants:

| View | Baseline REC-on admissions | Production REC-on admissions |
|---|---:|---:|
| HOME | 0 | 320 |
| PART | 320 | 320 |
| GEN | 0 | 320 |
| FX | 0 | 320 |
| MIX | 0 | 320 |
| GLO | 0 | 320 |
| SONG | 0 | 320 |
| PERF, test idle | 0 | 320 |
| SEQ | 320 | 320 |

HOME/SAVE and settings-list combinations preserve the current feeder behavior. No new modifier restriction was introduced. GEN/GLO entries report this predicate's behavior without choosing a new central-dispatch policy. Idle PERF is distinct from active performance testing, which is already guarded and was not rerun as a new scenario.

In the seven views outside SEQ/PART, the baseline cancels p0 on departure and suppresses it on reentry; production keeps it admitted while held. This is an admission-mask/mocked-release observation within one selected slot. It does not establish real held-inventory ownership, asynchronous STOP/PANIC behavior or a policy for slot/pattern changes.

A draft probe compilation initially failed `-Werror` because the substituted predicate left `bidx` unused. The final predicate table explicitly verifies `bidx` and `is_303`; no warning was suppressed. The failed draft build log is retained as `runs/attempt-01-substituted-build.log`. Final new-probe runs above passed the stated qualification; provisional new-probe work is not counted as historical evidence.

## Local test-only correction and independent review

Patch: `rec37-f1-test-only.patch`, **13,632 bytes**, SHA-256:

`89785c38b7b4884c544b1994df66b5242103274402b3036380bb1b6b90883351`

It changes only:

1. `tests/host/rec37_owner_integration_test.c`: remove the three substituted predicate functions; retain all historical test bodies.
2. `tests/host/rec37_owner_source_test.py`: extract the four exact one-line predicate/dependency functions and inject them before the existing production functions.
3. Add `tests/host/rec37_admission_probe.py`: the pinned standalone new-scenario driver and explicit historical baseline.

The corrected existing fixture was generated and passed **syntax-only compilation, exit 0**. Its historical scenarios were **NOT_RUN**. Patch dry-run and application to a disposable two-file copy passed; all three applied files match the candidate hashes. Original selected public source hashes still match. Individual validation receipts are in `runs/validation.json`.

The separate read-only reviewer **APPROVED** this exact patch, independently checking all source blobs, five probe-input receipts, generated C, matrix completeness/uniqueness, counts, variant isolation, historical-body preservation and patch application. It found no blocking issue and did not rerun the matrix or historical suites. The exact review and its limits are retained in `REVIEW.md`. No Sol-specific runtime is claimed.

## Retained evidence and limits

The earlier **4,304 / 130 on / 130 off** result remains a reported historical pass for its substituted model and mocked dependencies. It is not disproved by this stage and cannot qualify production cross-view admission. The **258,202-check compact-mask** result remains reported historical mask-equivalence evidence. Neither unchanged suite was rerun.

Current results qualify only extracted admission, token/pitch routing, wrapper intent and a mocked release boundary. Central `input_body` dispatch, real `bass_kbd_at`, real held inventories/release ownership, engine/audio, actual stopped/playing pattern writes, sanitizer runs, full-app REC/Free Play/lifecycle, latency, sound identity, exact Elite/Pro builds, linked flash/pool and hardware remain **NOT_RUN / NOT_MEASURED** in this stage. Unchanged guard coverage is reused only at its previously reported scope.

There were **zero production edits, existing-checkout/live-test edits, public writes, firmware builds, SDK/private-header inputs or device actions**. F2's blanket HOME/SAVE restriction and F3's blanket STOP/PANIC/context cancellation were not implemented. F4 remains separately accepted and outside this stage. No outer drum/BREAK map or SMC focus policy was invented.

## First consumer action and next owner

The **existing creator/coordinator** receives this Library packet through the delegated completion result, then the **incumbent REC writer** consumes the test-only patch. No writer release or downstream consumption is claimed.

First action: verify the packet manifest, exact public input blobs and patch hash; reproduce the bounded probe against the supplied selected-text intake:

```sh
python3 candidate/tests/host/rec37_admission_probe.py source fresh-runs
```

The expected receipt is baseline compile 0/run **1**, exact compile 0/run **0**, with the discrepancy counts above. The driver pins all five production inputs and rejects changed bytes. The original baseline expression is embedded so applying the correction cannot erase the comparison. The incumbent owner can apply the patch in its own owned checkout after reconciling any later head; a later source pin needs new affected qualification. No production fix is part of this packet.

**No source/test blocker remains for this bounded qualification.** Whole-app/native/device gates and separate accepted REC work remain open under their existing owners. The producer has stopped at this requested stage.

**Library delivery is BLOCKED.** The required prepared-upload helper failed during tool discovery with HTTP 401, before preparation; it returned no mutation receipt or Library IDs. The report and reproducible evidence archive remain local. No fallback upload or retry occurred. The creator/coordinator must complete the Library save through an authenticated supported route, then pass the test-only packet to the incumbent REC writer. See the separate local `library-delivery-status.json` for this exact failed action and final artifact hashes.
