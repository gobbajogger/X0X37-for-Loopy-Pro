# Minimum local proof

Use this matrix for stage workers and for `internal/pr-local-reviews` workers. All review and tests stay in the agent cloud VM or on the Mac. Never use GitHub Actions or `@codex` as the review gate or as a merge blocker after local proof.

Plan a tip with:

```bash
python3 tools/local_proof_plan.py --base <pr-base> --head HEAD --branch <head-branch>
```

Prefer the plan's `x0x_tests`, `verify`, and `skip` lists over a fixed kitchen-sink selector string. Prefer skip or narrow over delete. Package integrity gates stay required when they apply.

## No retest of proven heads

If the same commit and artifact hashes already have valid local proof (CHECKS / ALL_PASS / `DEV CHECKED` digests that name that commit and those hashes), do **not** re-run those checks.

Prove-merge and stage advance mean:

1. Confirm the recorded proof still matches the head and claimed artifact hashes.
2. Merge or publish.

Retest only when the head, artifacts, or claimed tests changed. There is no “always reconfirm ALL_PASS” tax.

## What each stage already proves

| Stage | Stage worker runs | Done evidence |
| --- | --- | --- |
| 1 Audit | Inspection only; no production edits | Audit recorded; successor path started (see workflow handoff). Unchanged tip + no defect → WAIT_FOR_TIP; no empty evidence PR |
| 2 Implement | Scoped host checks for the approved fix (`X0X_DEV_PADCAL=1`) | Checked commits + stage-3 successor |
| 3 Test | Relevant host selectors; failure disposition | ALL_PASS (or named FAIL disposition) + stage-4 successor |
| 4 Validate | Elite + Pro `tools/dev_validate.py run`; budgets; identities | Both models `DEV CHECKED` + stage-5 successor |
| 5 Merge | Tree gate vs records; nightly FF or recorded refusal; stage-6 prompt | Handoff + stage-7 successor |
| 7 Publish | Prehardware gates; live catalog/package hash MATCH | Live page serves candidate + next stage-1 successor |

Local review re-proves only what the PR tip can still falsify. It does not re-own an earlier stage's full suite when the three-dot diff is evidence-only and prior ALL_PASS / DEV CHECKED still apply to the same commit and hashes.

## PR kind → minimum proof

Classify from the three-dot diff vs the PR base (and the stage number when known).

| Kind | Typical tip | Must run | Must not re-run by default |
| --- | --- | --- | --- |
| audit-evidence / evidence-only (stage 1) | `evidence/*-stage1/*` only | Scope/whitespace; CHECKS vs SHAs; `stage-handoff` if HANDOFF/SUCCESSOR changed; live catalog MATCH only when the audit claims a live page | Full `tests/run_tests.sh`; publish/web kitchen sink; package rebuild; `node web/test_web.mjs` |
| implement (stage 2, firmware/tests) | Repair + host test + evidence | Host BF selectors for changed firmware paths (for clock/seq: `sequencer clk-lock clock-handover`); `stage-handoff` when handoff files change | Package rebuild; unrelated publish/web selectors; full suite |
| test-evidence (stage 3) | `evidence/*-stage3/*` only | Scope; verify tip CHECKS / test-run-summary still name this head — re-run named host selectors only if head or claimed selectors changed; `stage-handoff` when handoff changes | Full suite; package rebuild; default publish/web kitchen sink; duplicate ALL_PASS reconfirm |
| validate-evidence (stage 4 tip) | `evidence/*-stage4/*` only, records already `DEV CHECKED` | Scope; `python3 tools/verify_dev_checked_records.py --evidence <dir>`; CHECKS digest spot-check (skip `X0X_TESTS=dev-validate` rebuild when digests already MATCH) | **Elite+Pro `dev_validate.py run` rebuild**; full suite; default publish/web kitchen sink |
| validate (stage 4 with product change) | Firmware/tools build inputs in the tip | Full Elite+Pro `dev_validate.py run`; `pool_free >= 8192`; record digests MATCH | Unrelated host suites not touched by the change |
| merge-evidence (stage 5) | `evidence/*-stage5/*` only | Scope; tree gate vs validated SHA; stage-6 prompt digests vs stage-4 records; nightly FF or refusal log; `stage-handoff`; `install-gate-guard` | Package rebuild; publish/web kitchen sink; `test_web.mjs`; host BF suite already proven for that SHA |
| publish-evidence (stage 7) | `evidence/*-stage7/*` only | Scope; live catalog + package/controller hash MATCH; qualification claims | Package rebuild when stage-4 digests unchanged; unrelated host suites |
| docs-only / harness | `docs/`, `tools/` planner, tests for tools | Planner/unit tests for touched harness; selectors only for touched tools | Firmware BF suite; package rebuild |
| docs-only edit outside the stage loop | docs/skills only | Diff review; no stage loop | Firmware and package work |

## Default kitchen sink to stop using

Local reviews often ran this on every stage PR:

```bash
X0X_TESTS='stage-handoff nightly-publish installer-targets installer-digests install-gate-guard projects-web projects-page'
node web/test_web.mjs
```

Keep those selectors when the three-dot diff touches the matching tools/web paths. Do not run them as a default tax on evidence-only tips. `tools/local_proof_plan.py` encodes that rule.

## Package rebuild rule

Rebuild Elite and Pro development packages in local review only when:

1. The three-dot diff changes firmware, build inputs, `tools/dev_validate.py`, packaging tools, or other validation inputs, or
2. Tip records are missing, not `DEV CHECKED`, or fail `verify_dev_checked_records.py`.

When the tip is evidence-only and stage 4 already recorded `DEV CHECKED` digests, verify records and CHECKS claims instead of rebuilding.

## Integrity gates that stay

Do not remove or weaken:

- `DEV CHECKED` result on affected Elite/Pro records
- `pool_free >= 8192`
- Package and controller SHA-256 MATCH for publish and install prompts
- Required BF host regressions when firmware (or the matching host test) changes and prior proof does not already cover that exact head
- Live page hash MATCH before treating a nightly publish as done

## Policy

GitHub Actions and `@codex` are never test results and never merge blockers after local proof. Subscribe to Project PRs for merge/status awareness only.
