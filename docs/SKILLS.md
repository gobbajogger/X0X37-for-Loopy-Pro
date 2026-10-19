# Required writing skills

Every project agent and helper must read both files once at task start, before
its first work output. Apply them to replies, prompts, docs, logs, commits,
and handoffs. Reread only after a skill or project override changes.

- [Caveman](../skills/caveman/SKILL.md): result first, brief wording, no filler.
- [Simplified Technical English](../skills/asd-ste100/SKILL.md): plain words,
  correct grammar, and exact meaning. Use its default 80% STE style.

The owner's all-output scope overrides Caveman's exception for saved text.
Keep grammar, uncertainty, conditions, negation, and exact technical content.
These take priority over word limits, dropped articles, or “no hedging”.
Higher-priority instructions still control tool use, approvals, and status updates.
Do not add approval steps for already authorized work because of a style skill.
The writing rule is [rule 15](rules/style.md).

Pass this page and both resolved skill paths to each helper. Before sending,
check the result comes first, filler is removed, and meaning and evidence remain.
If a file is missing, restore it from the pinned source below. Report that gap
and continue independent work in plain, correct English. Never claim an unread
or missing skill was used. Optional Caveman modes and companion skills are not
installed or required.

## Installed sources

The owner chose public skills on 2026-10-19. These replace the missing box copies
for current use. They are not claimed as recovered originals. This supersedes
the earlier ban on public substitutes. [Recovery evidence](audit/writing-skill-recovery.md)
remains historical.

| Skill | Public source | Pinned revision |
| --- | --- | --- |
| caveman | [JuliusBrussee/caveman](https://github.com/JuliusBrussee/caveman) | `bad48a0e1df55e05e3f52249afa35ecc1909ee76` |
| asd-ste100 | [prithivrajmu/asd-ste100](https://github.com/prithivrajmu/asd-ste100) | `d93237679681a74873bdee4f1a7869f728968dbd` |

[Source hashes](../skills/sources.json) cover unchanged skill text, bundled
references, README files, licenses, and notices. `.agents/skills/` links to
these project copies. No global install or background updater is needed.
Run `python3 tools/check_writing_skills.py` after an install or update.
File checks and a fresh-agent review do not guarantee every future reply or
certify ASD-STE100 compliance. Correct any observed breach using rule 15.
