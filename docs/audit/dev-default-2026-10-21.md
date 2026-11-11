# Development default rule repair — 2026-10-21

Trigger: The owner corrected stage-4 release/full-gate scope to development build and installation, then explicitly requested a persistent development default and release-build permission rule in a side conversation.

Cause: Rule 11 permitted generic full-validation requests and workflow handoff wording to launch a gate that includes release builds without a current explicit release-build request.

Effect: Rule 11 owns the development default and release-build prerequisite. Core, build/test, install, stage-4 and BUILDING instructions point to or apply that decision. Trial, force and owner-validation purposes provide no exception. Existing release safety, publication authorization, budgets, model and data guards remain.

Scope: Rules and documentation only. No build runner, firmware, running process, validation record, device or remote ref changed. The parent development workflow remains with its owner.

Checks: `python3 tools/prefs_lint.py docs/USER_PREFERENCES.md` passes all 18 current entries. Local relative links in the seven affected documents resolve. `git diff --check` passes. No build or full gate was run for this documentation repair.

Manual scenario review, performed after writing the change:
- Build/install latest firmware, finish stage 4, merge or push: development configuration and scoped checks; no full gate.
- Ask for full validation without a release-build request: development checks; no release configuration or full gate.
- Trial, force, owner-validation, changed inputs or an old release handoff: no exception to the explicit release-build prerequisite.
- Explicitly request a release build: release configuration becomes authorized; a full gate runs only for the current qualifying release step or explicit full validation of that release build. Publication remains separate.
- Development install: exact DEV CHECKED artifacts, intended controller, budgets, queue/model guards, backup, lock and readback remain required.

Subagents are prohibited in this side conversation. This is a manual review; no independent helper review is claimed.

Reading size (seven affected documents, bytes): before 77897; after 80528; change +2631. The added text records the explicit authorization condition; historical decisions remain intact.

Workspace rollout follow-up (current rule-edit task): The maintained rules already
contained commit `f708dc5`, but workspace discovery mirrors still exposed the old
gate wording. Applied `tools/rollout_rules.py --workspace
/Users/mac/Documents/x0x37 --apply`; replaced files were backed up under
`repo-bak/rules-rollout-20261020T143628354732Z`. The subsequent `--check` reports
zero drift. `python3 tests/host/test_rules_rollout.py` passes its scratch-repository
checks, and `python3 tools/prefs_lint.py docs/USER_PREFERENCES.md` plus the workspace
mirror check pass all 17 entries reported by the current checker. The five affected
rule mirrors match their maintained sources byte for byte; their relative links
resolve.

One independent read-only reviewer in this task reviewed the saved commit and
maintained rules against generic build/install/stage-4/full-validation requests,
explicit release requests, trial/force and historical instructions. Result: PASS,
no gaps. Development remains the default, full gates require a current explicit
release-build request, and existing release/install/evidence/publication guards
remain intact. No firmware build, full gate, device write or remote push was run
for this follow-up. Active agents can discover the correction through the synced
workspace entry points; no cross-chat message was sent.
