# Check scope repair — 2026-10-20

Trigger: owner requested checks scoped to edits in the current chat.
Cause: “affected checks” lacked selection criteria and the catalog did not
separate content validation from checker regression coverage.
Effect: rule 9 ties checks to changed behavior and dependencies, preserves
applicable gate coverage, and permits reuse of unaffected results. Catalog
entries now state when their checks apply.

Validation on lane/dk-rules-check-scope, dirty tree based on 5d7997d:
- `python3 tools/prefs_lint.py docs/USER_PREFERENCES.md`: PASS, 7 entries.
- `git diff --check`: PASS.
- Manual relative-link existence check of both edited rule files: PASS.
- Bounded independent review: PASS for prose, shared code, checker contracts,
  formal gates, and unrelated later edits. Corrected its minor catalog wording
  finding to include content validators as well as regression tests.
- Scoped rollout: compared rule destinations against base, backed up six files,
  verified written bytes. Added only this owner entry to existing preference
  files, preserving the concurrent FM-1 reference-only decision.

No firmware/build/checker implementation changed; those suites were not needed.
Reading size (whitespace word count):
- docs/rules/commit.md: 293 -> 418 words
- docs/rules/SCRIPT-CHECKS.md: 245 -> 328 words

Backup: /Users/mac/Documents/x0x37/repo-bak/check-scope-20261019T165849Z

Handoff: current workspace rule readers use the updated mirrors; firmware
integration remains with its assigned owner. No cross-chat messages sent.
