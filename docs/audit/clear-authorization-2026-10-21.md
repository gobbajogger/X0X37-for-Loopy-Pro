# Clear development install permission and plain rules

The complete, validated development set installs without another approval question.
The installer sends a notice and retains every artifact, model, backup, write, and readback check.

## Trigger and cause

The owner corrected a stage5 report that said installation approval remained pending.
Source: direct owner follow-up in chat `01a15a2d-10dd-78a2-a345-fec17f62b453`, 2026-10-21.
The owner also requested an ASD-STE100 rewrite of the rules.

The preserved 2026-10-16 owner instruction permits a verified, complete build to install without another question.
Newer text excluded installation from automatic permission. The SMC handoffs then repeated that exclusion.
That conflict created a redundant approval requirement.

## Change

Rule 14 now states the standing permission directly, including its application to the current SMC workflow.
It supersedes the handoffs' separate-install-approval requirement. Rules 5–7 and 17–18 link to that permission.
The notice informs the owner. It does not ask for another decision.

All ten behavioral rule files use short, complete sentences and explicit conditions.
The writer read the installed ASD-STE100 skill and its strict notes.
The full standard and dictionary are not bundled. This is an STE-style rewrite, not certified compliance.

Preserve existing scope limits for unspecified external destinations, release builds, publication, fallback installs, and new features.
Preserve complete-candidate validation, exact artifacts and controllers, budgets, device locks, backups, bootloader limits, readback, and loop-slot checks.
Preserve duplicate checks, actual successor startup evidence, bounded recovery, and stage order.

The maintained checkout lacked the later controller-capture rule and its owner record.
Preserve that newer instruction from base `4b6a4c7`: reuse captures and simulated sends without another request for owner movement.
No firmware source, artifact, installer, enforcement code, skill source, or tool policy changes in this task.
This rules-only task does not launch another firmware workflow.

## Checks

The evidence folder is `/Users/mac/Documents/x0x37/rules_clear_authorization_evidence_20261021`.
It contains exact maintained text before the rewrite, checks, review, reading size, rollout backups, and final identities.

- `python3 tools/prefs_lint.py docs/USER_PREFERENCES.md` checks current decision pointers.
- `X0X_QUICK_ONLY=ste X0X_QC_BASE=4b6a4c7 bash tools/quickcheck.sh <lane>` checks documentation and commit requirements without a build.
- `python3 tools/ste_check.py` reports zero hard categories across the ten rewritten rule files.
- Manual review accepts soft verb-lexicon warnings for complete sentences such as “Preserve facts” and “Reuse valid evidence”.
- `git diff --check` checks whitespace.
- One independent reviewer checked permission, install, validation reuse, controller evidence, and successor cases.
  Its one finding restored the explicit condition that stage work and checks pass before successor creation.
  The reviewer confirmed the correction and found no further issue.

Record final committed checks and rollout results in the external evidence folder.
Use backups and compare current file hashes before replacing maintained text. Preserve unrelated dirty files and all historical preference text.
