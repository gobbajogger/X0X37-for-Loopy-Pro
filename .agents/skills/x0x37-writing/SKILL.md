---
name: x0x37-writing
description: "Write x0x37 output in plain, concise English; use STE rules and dictionary only for an explicitly requested strict review."
---

# Write clearly

### Rule 15: plain English by default

Use plain, correct, concise English by default for output and saved text.
Use complete sentences and direct verbs. State conditions clearly.
Preserve facts, uncertainty, negation, exceptions, technical terms, and literal commands.
Use "behavior" and "colour" in prose; keep code spelling.
Consult [project terms](references/terms.md) when needed.

Only when the owner explicitly requests a strict ASD-STE100 review, read [asd-ste100](../asd-ste100/SKILL.md), [strict notes](../asd-ste100/references/strict.md), and [official rules and dictionary](references/strict.md).
Check only that requested review against those sources. Claim compliance only for text actually checked.
Return to plain writing after the review.

Use [caveman](../caveman/SKILL.md) when requested. Preserve grammar, facts, and conditions.
For skill installation or repair, use [source status](../../docs/SKILLS.md).

### Rule 15A: reason for a change

State a clear reason, its purpose, and how it affects the end user for every change.
Put that reason, purpose, and end-user effect in the commit message and in the pull request.
A later reader must be able to tell an owner decision from a regression.
The end user is the person installing or playing the device, not another agent.
