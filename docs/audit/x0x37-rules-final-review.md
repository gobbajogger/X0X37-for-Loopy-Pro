# Independent final rule review

State: docs review begun; enforcement changes and final combined diff pending.
Tree: /Users/mac/Documents/x0x37/rules_audit. No project files changed by reviewer.

## Current observations

- The current rules permit authorized local work on Mac and Linux/container. They separate source review, host tests, target build, formal gate, install, device test, and release evidence.
- Firmware model guards, licensing, owner loop data, exact gated artifacts, bootloader/range/lock checks, static RAM >= 0 B and pool >= 8192 B remain explicit requirements.
- Retired roles and old rosters do not own current work. One integration task still owns firmware integration.
- Every archived file under docs/archive/rules-2026-10-17 matches its original workspace file byte for byte (13 files).

## Pending discovery blockers

1. AGENTS.md links docs/SKILLS.md; it is absent at initial read. docs/rules/style.md:9 and docs/AGENTS_AND_ROOMS.md:11 also require it. Fresh discovery cannot yet identify exact skill paths. Truthful status: exact asd-ste and caveman skills were not read or verified from the entry links. Continue independent work in plain, correct, brief English. Parent says skill files are pending.
2. docs/rules/docs.md:19 links docs/rules/SCRIPT-CHECKS.md; it is absent at initial read. Parent says sibling files are pending.
3. Rollout workspace entry claims flash_review_integration/docs is maintained source. Parent confirms canonical docs will be merged there before real rollout. Final verification pending.

## Representative tasks: next action and done criteria

| Task | Next action | Completion criteria |
|---|---|---|
| Mac docs repair, box offline | Use isolated rule lane; edit owned docs; run relevant docs/enforcement checks and bounded review. Do not wait for box. | Reviewed commit and requested handoff. Report checks and evidence limits. No target, gate, install, or device PASS claim. |
| Linux x86-64 local container target work | Mount actual toolchain/SDK/reference paths; use documented variables and a writable mounted work path. Run affected checks. | Local code task can end with checked commit/review/handoff. A formal gate may run locally when its full setup is present. Preserve reference pins and budgets. |
| Host repair with one optional DSP reference absent | Run independent affected host tests. Name absent reference and the blocked reference check. | Finish independent repair and handoff, with missing exact path recorded. Full gate/release stays pending if it needs that reference. |
| Device unavailable after host regression fix | Finish source fix and affected host checks. Keep device result NOT TESTED. | Commit/review/handoff can complete local task. Named installer waits for device and exact gate/artifact/data guards; no substitute fallback install. |
| Helper handoff | Give current entry/style/skill paths, input tree, owned files, exact task and done criteria. Keep one owner for each remaining action. | Receiving helper can find exact skills or report missing source, finish its scope, and return commit/checks/limits/next action without a retired-role approval cycle. |

## Checks still required

- Re-read combined docs and actual exact skill records after sibling edits arrive.
- Review final enforcement diff; run relevant test scripts on temporary repositories.
- Test rollout on a temporary multi-checkout workspace and confirm dry-run/idempotence/backups/no firmware writes.
- Verify changed paths contain no firmware source or original archive modifications.

## First-pass result

One concrete rollout defect found: copying only docs/agent-rules.md made a checkout look self-contained even if core rules or SKILLS.md were missing. Parent reports repair: compare the complete discovery set and use the wrapper otherwise; regression fixture now covers the partial checkout. Final test and diff verification pending.

No further blockers in the current core, style, shared-work, commit, device-bug, docs, owner-message, integration, install, roster, preference, and audit text. This is a docs first-pass result, not final approval of unreviewed enforcement or skill outputs. Final verdict remains pending.

## Follow-up review: canonical source and available files

The revised plan can complete current rule rollout without editing frozen firmware scripts. Integrate the rules and enforcement lane into separate main smk37 history, run rollout from stable x0x37/docs, and leave frozen source 532d91f and its running scripts with the firmware coordinator. Give the firmware coordinator the tested enforcement commit as a pending handoff. A blocked app message must not be repeated or treated as sent.

Fresh discovery now succeeds: AGENTS.md -> docs/SKILLS.md names exact recorded paths `/home/box/agent-data/workflows/asd-ste/SKILL.md` and `/home/box/agent-data/workflows/caveman/SKILL.md`, states both exact files unavailable, and forbids claiming installation/use. No namesake substitute is presented as an original. Missing original recovery remains a real dependency, not a barrier to independent authorized work.

Independent checks on current in-progress files:

- tools rollout test: PASS, including partial checkout with copied entry but missing core/skill docs.
- Additional scratch rollout from canonical `x0x37`: PASS. Dynamic wrapper/skill links point to x0x37/docs; refs-oscuro and third-party entries unchanged; firmware and archive bytes unchanged; repeat dry-run reports zero changes.
- Owner message tests: 12/12 PASS.
- Quickcheck STE: 13/13 PASS.
- Release-tree protection: 14/14 PASS.
- Remote allowlist: 13/13 PASS.
- Quickcheck rules: 9/10 PASS; one finding below. A requested test_prepush_quickcheck.sh path does not exist; no result is claimed for it.

Open findings sent to parent:

1. tools/quickcheck.sh:69: `flock` is absent on macOS. Both lock calls fail and execution proceeds to mutate the shared quickcheck clone/worktree without a lock. Mac docs mode needs a portable lock or a fail-closed setup result before mutation. Reproduced in quickcheck STE/rules output.
2. tests/host/test_quickcheck_rules.sh advisory grammar case fails because its bad rule fixture also invalidates current preference coverage (PREFS FAIL 2/4). Isolate the advisory grammar expectation without weakening preference coverage.
3. tools/quickcheck.sh header lines 22/28 still claim marker/rules-mode strictness; implementation now requires explicit X0X_STE_STRICT=1. tools/pre-push.hook failure text still names retired Build and Test Runner route. Align the current instructions.
4. docs/rules/SCRIPT-CHECKS.md remains absent at this read. Parent says pending sibling output.

Final combined verdict remains pending repairs and final tree.

## Final combined review result

No blocking defect remains in the combined rules lane reviewed on 2026-10-19. Approve this lane for its authorized commit and rule rollout. This is source/doc/enforcement review, not firmware gate, install, device, or release certification.

All earlier findings are closed:

- Complete discovery-set comparison prevents a partial checkout from receiving a false self-contained entry point.
- SKILLS.md is present and truthfully reports both exact originals unavailable. Exact skill recovery remains an external dependency, with source paths and recovery evidence.
- SCRIPT-CHECKS.md is present and entry/rule/skill Markdown links resolve.
- Missing macOS flock fails before cache writes. Mac folder docs mode works. Commit-mode lock setup is checked and the wait is bounded.
- Grammar-only fixture now preserves valid preference coverage. Rules quickcheck passes all 10 cases.
- Quickcheck strictness and missing-runner messages match the implementation.
- Rollout uses its actual canonical source path and safely repairs only recognized Mac hook repair wording. It preserves installed Mac hook policy, guard code and executable mode, and saves old bytes.

Independent final verification:

- Native Mac rollout test PASS, including dry run, exact backup, repeated-run behavior, partial discovery and recognized Mac hook wording/guard-tail/executable preservation.
- Native Mac current preference lint 4/4 and diff whitespace PASS.
- Actual Mac scratch docs folder mode PASS. Commit mode without flock fails before any shared-cache creation. Formal gate returns setup VOID on Mac before Linux signal work.
- Isolated local Linux image with network disabled: rule quickcheck 10/10, portable rule tests 5/5, device-bug evidence 9/9 PASS.
- Earlier independent owner messages 12/12, grammar quickcheck 13/13, release-tree guards 14/14 and remote allowlist 13/13 PASS.
- Additional independent rollout scratch case used canonical x0x37 and confirmed dynamic links, refs-oscuro/third-party exclusion, untouched firmware/archive bytes and zero repeat changes.
- All 14 original archive files match original workspace bytes. Changed/untracked path review contains no firmware source or proprietary SDK/toolchain files.

Rollout coordination limit: this review does not assert that a future merge or workspace rollout already happened. Main smk37 and frozen firmware history stay separate. Preserve the active firmware owner's frozen source and running scripts. A tested enforcement lane may be handed to that owner without taking over firmware integration. Report any enforcement merge still pending. If the main canonical checkout receives the new BUILDING.md Mac quickcheck instructions, it must also receive matching portable quickcheck behavior, or explicitly route that command to the reviewed lane/container. Do not leave a documented native Mac command pointing at the old GNU-date implementation.

The five representative task scenarios above still hold. Mac docs work uses folder mode with an explicit base; Linux/container uses real accessible tool/reference paths; missing optional references block only their dependent checks; unavailable hardware keeps device evidence NOT TESTED; helper handoff gives exact current sources or truthful missing skill status and one owner for each next action.

## Bounded main-port review

Main port: /Users/mac/Documents/x0x37/rules_main_rollout, based on 8ba4981.

No new blocking defect found. Approve main rollout after the parent's selected Linux checks pass. The first reported container attempt failed on unresolved linked-worktree metadata before tests ran; it provides no test result. Parent is rerunning with the workspace mounted at its same absolute host path.

- Compared all 113 ported entry/docs/tools/tests/fixtures with rules_audit: exact byte matches, including the existing guarded fm1_install.py. No firmware/host/lib paths changed.
- BUILDING.md changes comprise current authority notice and reviewed validation guidance. Original product build text is retained. The final worktree-mount clarification correctly states that Git common metadata must resolve; mounting the workspace at the same absolute path or using an independent clone avoids that dependency.
- Main native Mac command with X0X_QUICK_ONLY=ste, X0X_QC_BASE=HEAD, X0X_QC_WORK=/proc/x0x37-review-unused and the main worktree folder returned QUICK PASS. It reported 47 advisory grammar issues; no shared cache was needed.
- Main current entry/rule/skill links and diff whitespace checks PASS.
- Main's existing firmware test runner is preserved; it adds six standalone rule checks instead of importing the new firmware runner/tests.

This closes the earlier main-sync caveat: canonical main now includes the current portable quickcheck implementation matching BUILDING.md. Full Linux test outcome remains the parent's running validation responsibility; no firmware gate/install/device PASS is inferred.
