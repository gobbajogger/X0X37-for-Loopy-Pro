# Rule checks

Read for rule/enforcement changes. Run affected checks, not this whole list by default.

This table includes content validators and checker regression tests. Select
by the changed content, implementation, or contract using [rule 9](commit.md). For prose
clarification alone, review the affected scenarios and links; do not run unrelated
checker suites. A changed decision entry still needs its pointer check.

[Old check numbers](../archive/rules-2026-10-17/rules/SCRIPT-CHECKS.md) are historical.

| Run when this changes | Check | Evidence |
| --- | --- | --- |
| Installed writing skill sources, pins, or discovery | `python3 tools/check_writing_skills.py` | Pinned source hashes, frontmatter, references, and discovery |
| Rule entry/discovery or rollout copy behavior | `python3 tests/host/test_rules_rollout.py` | Entry files, copy safety and archive preservation |
| Current owner decision entries or their rule pointers | `python3 tools/prefs_lint.py docs/USER_PREFERENCES.md` | Current entries point to rules; history does not set new gates |
| Rule-commit enforcement or its accepted evidence contract | `bash tests/host/test_quickcheck_rules.sh` | Trigger/check evidence, valid pointers, advisory grammar |
| Owner-message checker or its accepted input contract | `sh tests/host/test_ste_owner_msg.sh` | Necessary paths, hashes, fragments and questions remain usable |
| Grammar checker, configuration, or strict-mode contract | `bash tests/host/test_quickcheck_ste.sh` | Warnings by default; explicit strict mode remains available |
| Device-bug evidence enforcement or its contract | `bash tests/host/test_quickcheck_devbug.sh` | Regression fails before fix and passes after fix; valid source evidence accepted |
| Path/environment enforcement or supported configuration | `python3 tests/host/test_portable_rules.py` | Caller reference paths, local repo selection and configured hook runner |
| Install-evidence guard or its contract | `sh tests/host/test_install_gate_guard.sh` | Exact gate result, no invented device result |
| Gate admission, purpose/reason, or reuse routing | `python3 tests/host/test_gate_reason.py` | Refusal before side effects; explicit purpose, PASS reuse and force routing |
| Shared gate process cleanup or change detection | `bash tests/host/test_gate_trap.sh` | Owned child cleanup, signal report, script change detection |

Run Linux-specific checks in a supported Linux environment. Record the command,
commit or dirty tree, result and missing dependencies. A focused test pass is
local evidence. It is not a release gate or a device PASS.
