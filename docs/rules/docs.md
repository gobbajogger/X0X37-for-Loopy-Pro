# Docs and rules

### Rule 12: current facts and history

Update only affected documents. Put intended behavior in `BEHAVIOR.md` and actual results in the relevant log.
Update changed plan status. Record owner decisions, sources, and active rules in `docs/USER_PREFERENCES.md`.

Do not invent data or require unrelated log edits. The integration checkout owns intended firmware behavior.

Preserve history. Explicitly supersede conflicting decisions.
Maintain instructions in `docs/agent-rules.md` and `docs/rules/`. Workspace copies are mirrors.

A device PASS covers only items that the owner tested on the exact build. Mark other items NOT TESTED.

Limit an owner checklist to ten tasks. Give each task a goal and starting state.
Include Build, Replaces, and Checked against decisions headers. Link technical evidence.

### Rule 12A: rule repair

Use `lane/dk-rules-<topic>`. Record the trigger, cause, effect, and checks in one audit.
Keep each requirement in one home. Use short links elsewhere. Load step rules only when needed.

Measure text size before and after the change. Preserve meaning and guards.
Use relevant [rule checks](SCRIPT-CHECKS.md) and one bounded independent review of representative tasks.

Include these lines in the commit body:

- `Trigger: <when>`
- `Check: <script path or manual: reason>`

Complete the authorized repair and rollout. Do not send work through retired roles.
Coordinate actual file overlap. Back up changed files before synchronizing maintained rules and entry points.

Preserve unrelated edits and history. Leave firmware integration with its assigned owner.
Send active agents a short path and change notice only through an authorized channel. Otherwise, save the handoff.

Do not restart completed work. Record future material rule violations in `docs/rules-breaks.md`.
