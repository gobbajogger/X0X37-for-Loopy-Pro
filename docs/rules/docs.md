# Docs, checklists, and logs

Read before docs work, owner checklists, device result logs, and task completion.

### Rule 12: current facts and history

Do not invent data. Update the docs affected by the change. Change `BEHAVIOR.md` when intended behavior changes. Change the plan when its status changes. Record new owner decisions in `docs/USER_PREFERENCES.md` with a pointer to the active rule. Record actual results in the relevant log. Do not force unrelated edits to four files for every task.

The repository `docs/agent-rules.md` and `docs/rules/` are the maintained instruction source. The wrapper mirrors them for discovery. The current integration checkout's `BEHAVIOR.md` describes intended firmware behavior. Old plans, backups, dated rosters, and logs remain historical evidence. A stale installed hash in a log is not a new install instruction.

Keep preference chronology and historical evidence intact. Add a current authority notice and a superseding decision when older instructions conflict. Sync current rules and entry points after a rule commit without changing firmware or active agents' unrelated edits.

Log device PASS only for the items and exact build the owner tested. Mark the other items NOT TESTED. A local build, host test, or selected check is not device evidence.

An owner checklist has no more than ten tasks. Start each task with its goal and a clear starting state. Include Build, Replaces, and Checked against decisions headers. Technical evidence can go in the attached record.

### Rule 12A: rule repairs

Use an isolated `lane/dk-rules-<topic>` branch. Audit the trigger, cause, and effect of each change. Run the relevant doc and enforcement checks. Give commit body lines `Trigger: <when the rule applies>` and `Check: <script path or manual: reason>`. Script check numbers and live status are in [SCRIPT-CHECKS.md](SCRIPT-CHECKS.md).

Use a bounded independent review for the final rules and representative tasks. The assigned task agent can complete the repair and rollout authorized by the owner. A missing Docs Keeper, Orchestrator, box, or retired role does not require a circular handoff. Coordinate only an actual overlap with live work. Keep firmware integration with its current owner.

Roll out the maintained entry point and current rule copies to affected project checkouts. Preserve old records and unrelated edits. Give active agents a concrete rule path and a short change notice through the authorized coordination channel. Do not restart completed tasks. Log causes and checks in the audit record; log a material future rule break in `docs/rules-breaks.md`.
