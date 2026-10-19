# Rule checks

Read when: change a rule, install a hook, or check completion.

The current rules are in core.md. The old numbered check list is in
../archive/rules-2026-10-17/rules/SCRIPT-CHECKS.md. Its dates, box paths, roster
and unfinished proposals are historical evidence, not current requirements.

| Area | Check | Evidence |
| --- | --- | --- |
| Rule entry and rollout | `python3 tests/host/test_rules_rollout.py` | Entry files, copy safety and archive preservation |
| Current owner pointers | `python3 tools/prefs_lint.py docs/USER_PREFERENCES.md` | Current entries point to rules; history does not set new gates |
| Rule commits | `bash tests/host/test_quickcheck_rules.sh` | Trigger/check evidence, valid pointers, advisory grammar |
| Plain owner messages | `sh tests/host/test_ste_owner_msg.sh` | Necessary paths, hashes, fragments and questions remain usable |
| Grammar review | `bash tests/host/test_quickcheck_ste.sh` | Warnings by default; explicit strict mode remains available |
| Device bug cause | `bash tests/host/test_quickcheck_devbug.sh` | Regression fails before fix and passes after fix; valid source evidence accepted |
| Local path and environment | `python3 tests/host/test_portable_rules.py` | Caller reference paths, local repo selection and configured hook runner |
| Install evidence | `sh tests/host/test_install_gate_guard.sh` | Exact gate result, no invented device result |
| Shared gate safety | `bash tests/host/test_gate_trap.sh` | Owned child cleanup, signal report, script change detection |

Run Linux-specific checks in a supported Linux environment. Record the command,
commit or dirty tree, result and missing dependencies. A focused test pass is
local evidence. It is not a release gate or a device PASS.
