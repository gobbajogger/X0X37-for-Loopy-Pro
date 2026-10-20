# Skill sources and discovery

Use [project skill triggers](agent-rules.md) to initialize workflow skills when needed.
Project procedures live in `skills/x0x37-*/SKILL.md`. `.agents/skills/` exposes them to skill discovery.
Automatic selection is enabled. A running thread can use the exact path from the trigger index.

Initialize [x0x37-writing](../skills/x0x37-writing/SKILL.md) before project output.
It selects strict STE and the relevant writing sources.

- [asd-ste100](../skills/asd-ste100/SKILL.md): strict STE, through `x0x37-writing`.
- [caveman](../skills/caveman/SKILL.md): explicit Caveman mode or a matching compression request.

## Installed writing sources


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
