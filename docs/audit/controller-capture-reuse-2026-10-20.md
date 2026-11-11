# Controller capture reuse rule repair — 2026-10-20

The maintained device-bug rule now requires existing controller captures and
simulated sends instead of another hands-on capture. This is stage-2 rule work;
it changes no firmware and gives no target-build, install, or device evidence.

- Base: `2592a00`; lane: `lane/dk-rules-capture-reuse`.
- Trigger: the owner's capture clarification and "Make it a rule too", preserved
  in [the source handoff](../../cloud-handoff/2026-10-20/smc_receive_repair_20261020/cloud-transition.txt).
- Cause: rule 8 still allowed a request for INLOG without the controller-input
  exception. The earlier idle capture did not demonstrate a receive failure.
- Effect: rule 8 owns the requirement and evidence labels. The preference entry
  records the exact quotes and narrow supersession; historical text is unchanged.
- Scope: `docs/rules/device-bug.md`, `docs/USER_PREFERENCES.md`, and this audit.
  Other device-bug evidence and all install, model, memory, and fallback guards stay.

Reading size, UTF-8 bytes and whitespace words:

| File | Before bytes / words | After bytes / words |
| --- | --- | --- |
| Device-bug rule | 1031 / 156 | 1508 / 223 |
| Preference record | 32913 / 5428 | 33657 / 5529 |

The added rule costs 67 words; startup entry points are unchanged. Only device
investigation loads this rule, and historical preferences do not set new gates.

Checks on the dirty rule tree, based on `2592a00`, in this Linux cloud environment:

| Command | Result | Log |
| --- | --- | --- |
| `python3 tools/prefs_lint.py docs/USER_PREFERENCES.md` | PASS, 17 current entries | `build/capture-rule/prefs-lint.log` |
| `bash tests/host/test_quickcheck_rules.sh` | PASS, 10/10 | `build/capture-rule/quickcheck-rules.log` |
| `bash tests/host/test_quickcheck_devbug.sh` | PASS, 9/9 | `build/capture-rule/quickcheck-devbug.log` |
| `git diff --check` | PASS | command output empty |

Independent read-only review: `/root/capture_rule/rule_review` found no issues.
It checked controller capture reuse; exact payload replay with simulated HCI;
resegmented supported-MTU input; idle zero-packet evidence; and an unrelated bug
without evidence. It confirmed evidence labels, no inferred physical PASS, narrow
supersession, and preserved regression, feature, connection, and fallback guards.

The stage-2 continuation owner receives this checked lane commit and applies it
once to the candidate. That integration is the rollout into the active cloud
continuation; no old chat or unrelated checkout is restarted or overwritten.
