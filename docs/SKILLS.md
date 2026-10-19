# Writing skills: load on demand

The short default in [core](rules/core.md) applies to all output. Read only the
selected skill once when its trigger applies; reread after relevant updates.

- [Caveman](../skills/caveman/SKILL.md): explicit Caveman mode or a request for
  terse compression that benefits from its detailed guidance.
- [Simplified Technical English](../skills/asd-ste100/SKILL.md): explicit STE,
  technical rewriting/explanation needing its guidance, or strict STE work.
  Load strict-mode references only for strict mode.

Keep grammar, uncertainty, conditions, negation, and exact technical content.
These outrank compression limits. The owner's all-output scope applies when a
skill is active. Higher-priority tool and approval instructions still apply.
Pass helpers only relevant skill paths. If a selected skill is missing, report
it and continue independent work; restore from its pinned source when needed.
Never claim an unread skill was used. Optional companion skills are not required.

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
certify ASD-STE100 compliance. Correct observed wording problems without adding a new workflow gate.
