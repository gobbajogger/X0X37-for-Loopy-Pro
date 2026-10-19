# Gate scope repair — 2026-10-20

Trigger: owner requested fixing always-on gates, explicitly selected focused
checks for development installs and full gates for releases, then asked that
validation depend on specific changes.
Cause: rule 11 required a gate per finished batch and rule 6 plus the installer
accepted only full gate evidence, even for development installs.
Effect: routine edits/commits/merges/batches use scoped checks. Development
installs accept separate DEV CHECKED evidence from a selected-model build,
budgets, justified selected tests, logs, and exact package/controller hashes.
Release configuration rejects development records; release gate behavior remains.
Queue, model, bootloader, locking and data guards remain in the install path.

Validation on lane/dk-rules-gate-scope, dirty tree based on b627b83:
- `python3 tests/host/test_dev_validate.py`: 10 tests PASS, scratch repositories
  and fake builds. Covers success without gate, release refusal, queue, hashes,
  source identity, docs-only reuse, failures, budgets, controller selection,
  old controller scripts, timeout child cleanup, installer wiring.
- `sh tests/host/test_install_gate_guard.sh`: 21/21 PASS; real gate fixture absent.
- `sh tests/host/test_install_guards.sh`: 11/11 PASS, stub device only.
- `python3 tools/prefs_lint.py docs/USER_PREFERENCES.md`: PASS, 8 entries.
- `git diff --check`, shell syntax, affected rule links: PASS.
- Bounded independent review: PASS after repairing cache/temp isolation,
  source identity checks, documentation-only reuse, and owned process cleanup.

Logs: [development](gate-scope-2026-10-20/dev.log),
[gate guard](gate-scope-2026-10-20/install.log),
[install guards](gate-scope-2026-10-20/safety.log).
No target build, formal gate, device write, or hardware validation was run.
Validation execution requires Linux x86-64 (a local container works), uses native
builds, and expects synchronous native plan commands. Verification is portable.
Coverage selection is reviewed against the diff; the runner does not infer it.
A new changed source tree needs new matching build/evidence. Reinstalling identical
artifacts does not. Controller-enabled validation requires isolation fixes in the
selected commit; an old commit is refused before running any checks.

Reading size:
- docs/rules/install.md: 248 -> 342 words
- docs/rules/integration.md: 227 -> 294 words

Rollout: affected maintained files and workspace rule mirrors copied with base
comparison and exact-byte verification; existing owner entries preserved.
Backup: /Users/mac/Documents/x0x37/repo-bak/gate-scope-20261019T170928Z

Receiving owner: assigned firmware integration chat. Adopt this lane's workflow
commit into its checkout before using its local installer. Firmware branch order
and device writes remain with that owner. No cross-chat messages or pushes sent.
