# Minimum local proof

Use this selector matrix only to plan scoped local checks. [AGENTS.md](../../AGENTS.md), the [maintained workflow](../../skills/x0x37-workflow/SKILL.md), current owner instructions, and live branch protections govern review, merge, and publication. GitHub Actions and `@codex` are not substitutes for local test results.

Plan a tip with:

```bash
python3 tools/local_proof_plan.py --base <pr-base> --head HEAD --branch <head-branch>
```

Prefer the plan's `x0x_tests`, `verify`, and `skip` lists over a fixed kitchen-sink selector string. Prefer skip or narrow over delete. Package integrity gates stay required when they apply.

## Reuse exact-input proof

Reuse recorded checks only when the checked source tree, configuration, tools and toolchain, dependencies and references, selected tests, and artifact/package/controller identities still match. Verify that the evidence names those inputs and its actual result. A new commit ID alone does not require a retest; an unchanged commit ID does not validate mutable inputs. Rerun only invalidated or failed checks after repair. See [validation and evidence](../../skills/x0x37-check/references/validation.md).

Local proof establishes only its recorded coverage. It does not authorize a stage advance, merge, device action, or publication. Missing or failed required checks remain open.

## PR kind → minimum proof

Classify from the three-dot diff vs the PR base and the actual affected inputs. Stage labels below describe evidence shape only; they do not create successor, merge, or publication authority.

| Kind | Typical tip | Must run | Must not re-run by default |
| --- | --- | --- | --- |
| audit-evidence / evidence-only | `evidence/*-stage1/*` only | Scope/whitespace; CHECKS vs exact inputs; `stage-handoff` if HANDOFF/SUCCESSOR changed; live catalog MATCH only when claiming a live page | Full `tests/run_tests.sh`; publish/web kitchen sink; package rebuild; `node web/test_web.mjs` |
| implement (firmware/tests) | Repair + host test + evidence | Host BF selectors for changed firmware paths (for clock/seq: `sequencer clk-lock clock-handover`); `stage-handoff` when handoff files change | Package rebuild; unrelated publish/web selectors; full suite |
| test-evidence | `evidence/*-stage3/*` only | Scope; verify CHECKS / test-run-summary against exact source, configuration, tools, dependencies, and selectors; rerun invalidated checks | Full suite; package rebuild; default publish/web kitchen sink; duplicate ALL_PASS reconfirm |
| validate-evidence | `evidence/*-stage4/*` only, records already `DEV CHECKED` | Scope; `python3 tools/verify_dev_checked_records.py --evidence <dir>`; CHECKS digest spot-check only if all validation inputs still match | **Elite+Pro `dev_validate.py run` rebuild**; full suite; default publish/web kitchen sink |
| validate (product change) | Firmware/tools build inputs in the tip | Full affected Elite/Pro `dev_validate.py run`; `pool_free >= 8192`; record digests MATCH | Unrelated host suites not touched by the change |
| integration-evidence | `evidence/*-stage5/*` only | Scope; tree gate vs validated inputs; install prompt digests vs DEV records; `stage-handoff`; `install-gate-guard` if touched | Package rebuild; publish/web kitchen sink; `test_web.mjs`; host BF suite already proven for exact inputs |
| publish-evidence | `evidence/*-stage7/*` only | Scope; live catalog + package/controller hash MATCH if publication is separately authorized; qualification claims | Package rebuild when all validation inputs and digests remain identical; unrelated host suites |
| docs-only / harness | `docs/`, `tools/` planner, tests for tools | Planner/unit tests for touched harness; selectors only for touched tools | Firmware BF suite; package rebuild |
| docs-only skill edit | `AGENTS.md`, docs, or skills only | Diff and link review; changed skill validation | Firmware and package work |

## Default kitchen sink to stop using

Older local reviews ran this on every stage PR:

```bash
X0X_TESTS='stage-handoff nightly-publish installer-targets installer-digests install-gate-guard projects-web projects-page'
node web/test_web.mjs
```

Keep those selectors when the three-dot diff touches the matching tools/web paths. Do not run them as a default tax on evidence-only tips. `tools/local_proof_plan.py` encodes that rule.

## Package rebuild rule

Rebuild Elite and Pro development packages in local review only when:

1. Checked firmware, configuration, toolchain, tools, dependencies, references, packaging inputs, or artifact bytes changed or cannot be shown identical, or
2. Tip records are missing, not `DEV CHECKED`, or fail `verify_dev_checked_records.py`.

When the tip is evidence-only and `DEV CHECKED` records cover the exact source, configuration, tools, dependencies, selectors, and artifact digests, verify those records and CHECKS claims instead of rebuilding.

## Integrity gates that stay

Do not remove or weaken:

- `DEV CHECKED` result on affected Elite/Pro records
- `pool_free >= 8192`
- Package and controller SHA-256 MATCH for publish and install prompts
- Required BF host regressions when firmware, matching host tests, or other checked inputs change and prior proof does not cover the exact inputs
- Live page hash MATCH before treating a nightly publish as done

## Policy

GitHub Actions and `@codex` are not local test results. Inspect required checks and current branch protection before any separately authorized merge. A local proof record does not override a required check, review, or permission.
