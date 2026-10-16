# Core rules (read at task start)

1. Never push outside the box and the owner's machines. Never push, merge or package refs-oscuro.
2. Stop only your own processes, by PID. Never use pkill -f or killall on a shared script.
4. Before a shared or hard-to-undo action, do the Decision check (shared-work.md).
6A. Only Firmware Main Builder installs (install.md), and only when all queued changes are complete and ready to test together. Put a fix in the current set, not in a fallback build that only makes the keyboard playable. Never ask the owner about an install.

12A. Treat each rule change like a code change (docs.md rule 12A).

Open each file at its step:
- shared-work.md: hardware, shared file, wait.
- device-bug.md: device bug.
- commit.md: commit, push, handoff.
- integration.md: merge, gate.
- docs.md: doc, log, checklist.
- owner-messages.md: owner message, change decision.
- style.md: text, terms.
