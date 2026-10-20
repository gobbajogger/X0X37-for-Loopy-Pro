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
