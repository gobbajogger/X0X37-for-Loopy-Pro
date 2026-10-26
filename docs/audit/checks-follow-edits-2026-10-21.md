# Checks follow committed edits

Trigger: The owner rejected a repeated complete loop-slot readback during the latest installation in chat `01a15b38-73dd-7363-9646-99f95a8f770c` and required tests related to committed edits.

Cause: Check selection already used changed inputs, but the install reference required every slot CRC after every install. That unconditional step overrode scoped evidence reuse even when the package, controller, loop code and slot map were unchanged.

Effect: Rule 9 requires a changed path and affected failure for each test. Rule 5 now defines when full-slot readback is relevant, preserves backups and native installer verification, records reused evidence honestly, and permits reuse of an already verified byte-identical install. Older decisions remain in the preference history and are explicitly superseded. No firmware, installer code, test selector, budget or release guard changed.

Checks: Review representative changed/unchanged storage, installer, package and host-tool scenarios; validate skill links and owner-decision pointers; obtain one independent review. Source text counts and review results are in `evidence/related-checks/`. No firmware build or device operation is needed for this prose correction. Rollout backs up changed discovery sources before synchronizing them.

Result: Independent review passed with no findings. `tools/check_project_skills.py` passed for ten skills; `tools/prefs_lint.py` passed for 35 entries; `git diff --check` passed. The existing rollout applied 45 managed preference/discovery updates with replacement backups and then reported zero drift. Its source remains `workspace/x0x37`. The four changed maintained source files have separate pre-sync backups; unrelated dirty primary files were verified unchanged. The native-worker task confirmed no overlap with its code, tests or running checks.

Evidence: [checks](../../evidence/related-checks/checks.json), [independent review](../../evidence/related-checks/independent-review.json), [review inputs](../../evidence/related-checks/review-inputs.json), [source backup manifest](../../evidence/related-checks/source-backup.json), [rollout backups](../../evidence/related-checks/rollout-apply.json), and [zero-drift check](../../evidence/related-checks/rollout-check.json). Generated discovery links and the local skill-status block use the unchanged rollout implementation. Backups, firmware packages and device data remain local.
