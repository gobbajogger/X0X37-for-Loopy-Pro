# Full gate admission audit — 2026-10-20

The maintained runner now requires a declared current purpose and a concrete
reason before setup, result writes, clones, locks, or builds. Routine development
uses scoped checks. This audit did not run a full validation gate.

## Trigger and cause

The owner requested an audit to prevent full gates without a concrete reason.
Rule 11 already restricted full gates to required install/release work or explicit
owner validation, but `tools/gate.sh <commit>` could start expensive validation.
Only `--force` required a reason. `--trial` always bypassed PASS reuse.

The maintained pre-push and server hooks call quick checks, not the full gate.
Four dated workspace `run-formal-gate.sh` wrappers invoke the runner with only a
commit. They are historical run inputs, not continuing authorization. They now
fail closed if used with the maintained runner; no reason was invented for them.
A separate install gate was already running from `smc_install_integration` during
the audit. Its process, source, and evidence were left intact.

## Repair and scope

`--purpose` accepts release-install, release-package, or owner-validation.
`--reason` must contain nonblank meaningful text identifying the current step or
request. The script checks for an alphanumeric character and records the text;
it cannot prove that a caller's declared reason or authority is true. Rule 11
requires that judgment before invocation. Development, a changed commit, an old
failure, trial mode, or force mode alone does not justify a full gate.

Trial requires owner-validation. Force also requires a rerun reason. Matching
PASS reuse remains available. The reason is recorded in gate logs, completed
results and index rows, including reuse rows. Existing result identity and
validation coverage were not changed. Toolchain/dependency matching still needs
the existing caller check; the old cache is keyed by non-Markdown source tree.

The admission check is registered in `tests/run_tests.sh` and SCRIPT-CHECKS.
The usage example and signal-test fixture supply explicit purposes. Full gates,
firmware builds, installs, and device tests were not needed for this repair.

## Checks

Input: isolated lane `lane/dk-rules-gate-reason`, based on `5d7997d`, with the
changes in this commit.

- macOS `python3 tests/host/test_gate_reason.py`: two tests passed; Linux reuse
  test skipped on that platform.
- Linux container `x0x37-validation:local`, same absolute workspace mount:
  `python3 tests/host/test_gate_reason.py`: all three tests passed. Log:
  `build/gate-reason-audit/admission.log`. Covers missing/blank/invalid admission,
  force and trial restrictions, no early writes, permitted purposes, PASS reuse,
  and explicit force routing. Fixtures use scratch repositories, never live results.
- Linux `bash tests/host/test_gate_trap.sh`: five checks passed with container
  user 501:20. The first root-user attempt failed on Git ownership checks in the
  bind-mounted fixture; using the actual workspace UID resolved it. This is a
  bounded signal fixture, not a full firmware gate.
- `bash -n tools/gate.sh`, `git diff --check`: passed.
- Maintained `python3 tools/prefs_lint.py docs/USER_PREFERENCES.md`: nine entries
  passed after preserving concurrent decision edits.
- One independent read-only review: no blocking findings. Its two minor notes
  (usage guidance and test registration) were repaired.

The lane integration rule grew from 227 to 311 words. The added 84 words define
admission, force/trial treatment, and the limit of mechanical enforcement.

## Rollout and handoff

Applied the runner, affected tests, BUILDING example, and additive rule/decision
changes to the maintained `x0x37` checkout. Updated the affected root rule and
preference mirrors. Existing unrelated install-validation edits were preserved.
Backups and an exact changed-file manifest are in
`../repo-bak/gate-reason-20261020T204408/` relative to this checkout.

Firmware integration remains with the assigned integration chat, the single
receiving owner for this lane. It can include this commit with its ongoing
validation changes. The main checkout already contains the repair as working
tree changes; avoid applying it twice. Existing running gates and historical
snapshots are not retroactively changed. Before a future run from a separate
checkout or installed copy, use the updated maintained runner and supply the
actual current purpose/reason. No external push or cross-chat message was sent.
