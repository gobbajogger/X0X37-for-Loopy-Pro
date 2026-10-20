# Strict STE default

Strict ASD-STE100 is the project default for all output, including saved text.
The official Issue 9 reference is available outside the repository.

## Trigger and cause

The owner requested "100%ste" in chat `01a15a3f-e535-7c03-a89a-4d2be5f3fbb7` on 2026-10-21.
The owner then instructed the agent to use available tools and continue.
The installed skill defaults to 80% STE. The project rules previously loaded it only on demand.
The strict notes did not give a local reference path. The agent stopped before it searched for the official reference.

## Change and scope

Rule 15 sets the strict default. Rule 6A links to that instruction.
`docs/SKILLS.md` gives the reference location and the rules for its examination.
`docs/USER_PREFERENCES.md` records the owner instruction and supersedes the previous STE defaults.

The official PDF and its searchable text are in `~/.cache/asd-ste100/issue-9/`.
The PDF has 434 pages. The reference includes the rules and dictionary.

The agent read the summaries for all nine rule sections and the dictionary instructions.
The agent also read selected full rules and dictionary entries.
The saved instructions direct future agents to read the applicable rules and examine words in context.
An approximate script result does not prove full compliance.

The pinned skill files retain their original hashes. The reference stays outside version control.
The standing development-install permission and all other rules retain their previous scope.
This change does not add a software gate.

## Checks and text size

Base: `f1c9ccd9e82a90bde949469a50ae08c6542121b1`.
The previous task agent released the three shared files before edits began.
The evidence folder is `/Users/mac/Documents/x0x37/rules_strict_ste_evidence_20261021/`.
It contains text-size measurements, source hashes, check results, review, and rollout backups.

The core rule changes from 387 to 390 words. The style rule changes from 169 to 213 words.
The skill instructions change from 265 to 504 words. The owner record adds 65 words.
The added instructions give the strict reference and word-examination method in one location.

Use these checks for the completed change:

- `python3 tools/check_writing_skills.py` examines source hashes and discovery.
- `python3 tools/prefs_lint.py docs/USER_PREFERENCES.md` examines decision pointers.
- Manual review examines the changed requirements and relative links.
- One independent review examines strict defaults, reference discovery, meaning, and representative tasks.
- `git diff --check` examines whitespace.
- The documentation-only quickcheck examines commit evidence and decision pointers.
- `python3 tools/rollout_rules.py --workspace /Users/mac/Documents/x0x37 --check` examines mirror agreement after rollout.

Save final results in the evidence folder. Back up maintained files and compare their previous hashes before replacement.
Keep unrelated edits and historical records.
