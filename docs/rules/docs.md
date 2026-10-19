# Docs and rules

### Rule 12: current facts and history

Update only affected docs: intended behavior in `BEHAVIOR.md`, changed plan
status, actual results in the relevant log, and owner decisions with source
and active rule in `docs/USER_PREFERENCES.md`. Never invent data or force unrelated
log edits. The integration checkout owns intended firmware behavior.

Keep history intact; supersede conflicting decisions explicitly. Maintained
instructions are `docs/agent-rules.md` and `docs/rules/`; workspace copies are
mirrors. Device PASS covers only owner-tested items on the exact build; mark
others NOT TESTED.

Owner checklists: at most ten tasks, each with goal and starting state. Include
Build, Replaces, and Checked against decisions headers; link technical evidence.

### Rule 12A: rule repair

Use `lane/dk-rules-<topic>`. Record trigger, cause, effect, and checks in one audit.
Keep each requirement in one home with short links elsewhere; load step rules
on demand. Measure reading size before/after, but preserve meaning and guards.
Use relevant [rule checks](SCRIPT-CHECKS.md) and one bounded independent review
against representative tasks. Commit body needs `Trigger: <when>` and
`Check: <script path or manual: reason>`.

Finish authorized repair and rollout; no retired-role handoff loop. Coordinate
actual overlap. Sync maintained entry points/rules with backups, preserve
unrelated edits/history, and leave firmware integration with its owner. Give
active agents a short path/change notice only through an authorized channel;
otherwise save the handoff. Do not restart completed work. Log future material
rule breaks in `docs/rules-breaks.md`.
