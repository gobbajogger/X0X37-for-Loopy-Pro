# Core rules

### Rule 1: protect data and sources

- Keep code on owner machines and approved local repositories. External pushes
  need an explicit owner request. Never push, merge, or package `refs-oscuro`.
  Keep proprietary JieLi SDK/toolchain files out of commits and bundles.
- Preserve licensing, target model guards, flash/RAM limits, features, and sample quality.
### Rule 2: process and edit ownership

- Start every task in a fresh chat, including work continued from an old chat.
  If this chat was just created for this task, finish here; do not create
  another chat for the same task. Use old chats as information sources.
  New agents may contact other chats for context and handoff details needed to
  continue old work; continued task execution stays in the fresh chat.
  They may ask a chat still working to notify the fresh chat when done.
  That completion reply is authorized; use it to continue promptly without
  polling or a timer automation. Do not send new execution work to old chats.
- At task start, always check which other chats are currently working on this
  project. Inspect relevant status and scope read-only before editing, merging,
  or taking a shared resource; avoid duplicate work and overlapping ownership.
  Check again before shared changes if the activity snapshot may be stale.
- Use isolated lanes for edits; preserve others' work. Stop only your own
  processes by PID; never use `pkill -f` or `killall` on shared scripts.
### Rule 6A: finish authorized work

- Finish authorized work and relevant checks. Missing hardware or retired roles
  block only dependent steps. Name a real blocker and next action; do not repeat
  approval requests or poll without cause.
- Report only verified evidence. Distinguish source review, host tests, target
  build, formal gate, install, and device test; include inputs and unrun checks
  when relevant. Local completion grants no install or release permission.
- Use plain, correct English in all output, including saved text. Give the
  result first; remove filler. Preserve exact technical content, conditions,
  uncertainty, and evidence. Full writing skills load only for their triggers.
