# Docs, checklists and logs

Read when: before an owner test checklist, a doc, a log or a device result log, and at the end of each task.

### Rule 12
Step: P7 checklist, P11 log result. Rule owner: Docs Keeper.

Do not invent data. Docs and logs hold only development information. Update BEHAVIOR.md (reference copy on integration), ROADMAP.md, USER_PREFERENCES.md and the log (x0x37-progress-log.md) in the same task. Keep the box and the Mac copies in sync (git commits and docs). Log a device PASS only for the items and build that the owner names. Mark the others NOT TESTED.

Start each owner test checklist with these header lines:
- Build: the batch number and the gated hash.
- Replaces: the earlier checklist, or "none".
- Checked against decisions: the time of the latest USER_PREFERENCES.md entry that you compared with the checklist.

Put 10 tasks or fewer in an owner test checklist. Start each task with its goal, in the words of the owner. Each task starts from any screen.
