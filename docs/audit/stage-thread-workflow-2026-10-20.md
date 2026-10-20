# Seven-stage thread workflow — 2026-10-20

Trigger: the owner's seven-stage workflow request in
`01a1592b-f948-7f30-9fad-0d77131922eb`, followed by "Update rules only".
The exact instructions are recorded in [preferences](../USER_PREFERENCES.md).

Cause: core rule 2 required a fresh chat per task but did not define stage
boundaries or require each completed stage to start its successor. Existing
validation and integration rules did not explain how validation before merge
must be checked against the resulting merged revision in this workflow.

Effect: [workflow rules 17 and 18](../rules/workflow.md) define the requested
order, one fresh thread per stage, test subagents, successor ownership,
handoff evidence, repair loops, approval continuity, and termination after
push. Core links to that single home; the router loads it only for a staged
workflow. Existing scoped checks, release gates, installer guards, and
integration ownership remain applicable.

## Scope

Lane: `lane/dk-rules-stage-threads`, based on `72b0a2a`.
Only the router, core link, new workflow, new decision, and this audit change.
This task does not start a code audit, firmware build, merge, install, or push.

The activity check found “Align and enforce agent workflow rules”
(`01a15920-3db5-7941-8aec-979e6707dfd0`) working on enforcement and preference
formatting. Authorized context messages established separate ownership.
Its reply confirmed it does not edit core, and its rollout will consume
maintained docs while preserving this task's additions. The router addition
was also identified in the handoff.

## Checks

- `python3 tools/prefs_lint.py docs/USER_PREFERENCES.md`: PASS, 11 entries
  in this lane.
- `python3 tests/host/test_rules_rollout.py`: PASS using scratch repositories;
  covers new rule discovery, backups, exclusions, and repeat behavior.
- `git diff --check`: PASS.
- Manual changed-link check: PASS.
- Historical preference text after its divider: byte-identical to lane base.
- Independent review by `/root/stage_rules_review`: PASS after one repair.
  Review found that public-release package rules could block stage 4 before
  device testing. Stage 4 now explicitly validates candidate/install packages;
  public-release packaging still requires the same-build owner device PASS
  and a fresh requested thread. The reviewer confirmed the repair.

Reading size (whitespace-separated words / UTF-8 bytes):

| File | Before | After |
| --- | --- | --- |
| Router | 185 / 1273 | 197 / 1350 |
| Core | 342 / 2179 | 359 / 2299 |
| Workflow, loaded on demand | 0 / 0 | 762 / 4974 |

Only the short router row and core link add general discovery text. Detailed
stage instructions have one home. No enforcement code changed.

## Rollout

The exact additions are applied to maintained `x0x37/docs` and the workspace
router, core, workflow, and preference mirrors. Their contents match; affected
links resolve. The maintained preference check passes with 14 entries.
Pre-edit backups and before/after hashes are saved in
`repo-bak/rules-stage-threads-20261020T120209987800Z/manifest.json`.
All unrelated live preference and rule content was preserved.

The broader stable-pointer rollout remains owned by the active enforcement
thread named above. A dry run showed 25 checkout entry files to refresh; it
received the applied-file handoff so that rollout can use these maintained
rules. This task does not overwrite its tooling or run a competing rollout.
