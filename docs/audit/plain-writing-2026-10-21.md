# Plain writing and on-demand instructions

Trigger: The owner selected plain English by default, strict STE review only on explicit request, and instructions for the current step only in chat `01a15a99-75d7-7921-9d65-f4b120566904`.

Cause: Rule 15 required official rules and dictionary checks before every output. This loaded detailed writing sources for routine coding and messages.

Change: Rule 15 now applies concise, correct English to all output and saved text. It loads STE sources only for an explicit strict review. Rule 6A states current-step loading and reuse of unchanged sources. The skill manifest and source index use the same triggers. The preference record explicitly supersedes the earlier universal strict default. The pinned public skills and dictionary stay unchanged.

Scope: Project instructions only. No firmware build or device check is needed. Build-speed implementations retain their existing owners.

Representative checks: A routine status reply, code summary, documentation edit, or commit uses the short writing entry without the dictionary. An explicit strict ASD-STE100 review loads the strict skill, notes and official reference. A linked later-stage procedure stays unloaded until its step starts. Facts, uncertainty, conditions, project terms and literal commands remain intact.

Validation: Project skill source/link/manifest checks passed (10 skills). Preference pointer checks passed (28 entries). One independent review found a lingering per-output instruction in the strict reference. It now applies only to the requested review; the writing entry explicitly returns to plain writing afterward. The reviewer confirmed closure with PASS. The shared rollout uses backups and exact baseline checks; post-rollout source and pointer verification remains required.

Text size (whitespace-separated words):
- `skills/x0x37-writing/SKILL.md`: 192 → 137.
- `skills/x0x37-task/SKILL.md`: 334 → 341.
- `skills/x0x37-writing/references/strict.md`: 245 → 262.
