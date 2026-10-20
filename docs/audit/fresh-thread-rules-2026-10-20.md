# Fresh task chats and completion handoffs — 2026-10-20

Trigger: the owner's fresh-chat instruction, relayed from
`01a15922-cd41-72f3-8774-692f97f8c359`, and three direct follow-ups in
`01a15923-4d24-73a0-a57d-dfd02e5928da`.
Exact quotes and active pointers are in [the preference record](../USER_PREFERENCES.md).

Cause: rule 2 did not require fresh task chats or an activity check, and rule 16
allowed helper reuse without that boundary. The owner then clarified permission
for context, handoff, and completion messages.

Effect: [rule 2](../rules/core.md) is the single operational home. Each task and
continuation runs in a fresh chat; a chat already created for its task finishes
there. Agents inspect project activity before work and shared changes. They may
request context and ask working chats to notify the fresh chat on completion.
Those replies are authorized. Continued execution stays in the fresh chat,
without polling or a timer automation. Rule 13 links to that message permission;
rule 16 links to chat ownership instead of permitting unrestricted helper reuse.

## Scope and preservation

Lane: `lane/dk-rules-fresh-threads`, based on `5d7997d`.
Only preference/rule text and this audit belong to this lane.
The maintained checkout has unrelated edits; rollout applies only this lane's
four-file documentation patch and syncs its corresponding workspace mirrors.
Before-edit backups are under
`repo-bak/rules-fresh-threads-20261020/` in the workspace.
Preference history is unchanged. No firmware, gate, installation, or external
push is part of this task.

The project activity check found “Align and enforce agent workflow rules”
(`01a15920-3db5-7941-8aec-979e6707dfd0`) active. Its scope was read before shared
edits. A context/handoff message identified this lane and requested preservation
of its additive entries during the other chat's preference-format and rollout
work. It also requested a completion notice under the owner's clarified
authorization. No new execution work was assigned to that chat.

## Checks and review

Commands in this lane:

- `python3 tools/prefs_lint.py docs/USER_PREFERENCES.md`: PASS, 10 current entries.
- `python3 tests/host/test_rules_rollout.py`: PASS, scratch dry run, backups,
  discovery, third-party exclusion, and repeat behavior.
- `git diff --check`: PASS.
- Manual changed-link and byte-comparison checks: all rule links resolve;
  historical preference text is byte-identical to the lane base.
- Independent review by `/root/fresh_thread_rule_review`: PASS, no findings.
  Covered fresh tasks, continuations, same-task follow-ups, active-chat overlap,
  context requests, completion notices, and unrelated messaging.

Reading size, whitespace-separated words and UTF-8 bytes:

| Rule | Words before → after | Bytes before → after |
| --- | --- | --- |
| Core | 191 → 342 | 1267 → 2179 |
| Shared work | 177 → 178 | 1169 → 1184 |
| Owner messages | 144 → 166 | 979 → 1148 |

The extra core text records the new owner requirements. Other files link to that
home. No enforcement code changed, so unrelated checker suites and firmware
builds are outside this documentation check scope.

## Rollout result

PASS: the four affected maintained documents and their workspace mirrors match.
The live preference record retains all prior bytes after removal of only the
four new entries. Its pointer check passes with 13 entries (the lane has 10
because three unrelated current decisions are already present in the maintained
checkout). `git diff --check` also passes there. Backup hashes are recorded in
`repo-bak/rules-fresh-threads-20261020/rollout-manifest.json`.

The existing workspace entry already routes through the mirrored core rules.
The separate workflow chat owns its broader discovery-tool repair and rollout;
this task does not replace that script or rewrite unrelated checkout entries.
This lane's local commit is the durable handoff to the project owner. Firmware
integration remains with its assigned owner; this rule-recording task is complete.
