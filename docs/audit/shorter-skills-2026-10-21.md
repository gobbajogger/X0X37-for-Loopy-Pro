# Short skill entry points

## Trigger and cause

The owner requested shorter, clearer rules in chat `01a15a99-75d7-7921-9d65-f4b120566904` on 2026-10-21.
The owner then requested simple instructions with clear paths and solutions.
A small first draft removed repeated prefaces but left long procedures in entry files.
That draft was not applied. The current change uses short entry points and required step references.

## Result

- Rule 6A defines skill use and instruction priority one time.
- Follow-ups use loaded instructions. Changed sources require their changed sections.
- Task startup searches relevant decisions instead of loading the full history by default.
- The trigger index uses short step descriptions. Catalog descriptions and their manifest are unchanged.
- Ownership, check execution, commits, build setup, release gates, stage execution, handoffs, install artifacts, install evidence, and terminology have explicit reference paths.
- Each reference has a read condition in its entry skill.
- Long technical procedures are still required at their applicable step. They are not startup reading for unrelated work.
- Strict STE and its required official sources remain applicable before output.

The four device-bug, docs, owner-decision, and shared-work procedure bodies are unchanged after removal of the repeated preface.
The new environment, ownership, commit, validation, terminology, and install references preserve their prior instructions.
Workflow stage and handoff procedures were shortened without removal of their conditions.
The release reference preserves gate admission, input identity, purpose/reason, validation, budgets, and exact-artifact evidence.
No firmware script, automatic test selector, validation record format, gate implementation, or device state changed.
The earlier automatic test-selection and independent controller-record recommendations remain unimplemented.

## Size

The comparison uses base commit `c5ed5da`.
It includes ten skill entries, the trigger index, and every new step reference.
It excludes existing unchanged references, public writing skills, decision history, and this audit.

- Skill entries plus trigger index: 5168 words before; 2644 after (48.8% less).
- New step references: 2112 words, required only at their stated steps.
- Total including those references: 4756 words (8.0% less).
- Task and writing entry points together: 526 words after.
- Trigger index: 319 words before; 171 after.

The larger reduction is in the text loaded for a step. The total reduction is smaller because technical safeguards remain available.
No runtime savings are measured.

## Validation

`python3 tools/check_project_skills.py`: PASS for all ten skills, descriptions, discovery, IDs, and local references.
`python3 tools/prefs_lint.py docs/USER_PREFERENCES.md`: PASS for 27 current entries.
`python3 tests/host/test_rules_rollout.py`: PASS in scratch repositories.
`git diff --check`: PASS. A separate local-link and source comparison passed.
The final independent review found two missing read/exit conditions.
The workflow now starts a successor only when a next required stage exists.
Install evidence is required explicitly for release packaging and publication.
The reviewer confirmed both fixes and found no other material safeguard loss.
The shared-source update uses exact baseline checks and backups. It changes only these policy sources and their workspace mirrors.
No firmware build or device test is required for these text and routing changes.
