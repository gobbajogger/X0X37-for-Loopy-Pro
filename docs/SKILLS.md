# Writing skills

Use strict ASD-STE100 for all output under [rule 15](rules/style.md).
Read the selected skill one time. Read it again only if it changes.

- [Caveman](../skills/caveman/SKILL.md): explicit Caveman mode or a request for
  terse compression that benefits from its detailed guidance.
- [Simplified Technical English](../skills/asd-ste100/SKILL.md): the project default.
  Read its [strict-mode notes](../skills/asd-ste100/references/strict.md).
  The owner's strict-mode instruction replaces the skill's 80% default.

Keep grammar, uncertainty, conditions, negation, and exact technical content.
These outrank compression limits. The owner's all-output scope applies when a
skill is active. Higher-priority tool and approval instructions still apply.
Pass helpers only relevant skill paths. If a selected skill is missing, report
it and continue independent work; restore from its pinned source when needed.
Never claim an unread skill was used. Optional companion skills are not required.

## Strict STE references

Use [ASD-STE100 Issue 9](https://www.asd-ste100.org/assets/files/ASD-STE100_ISSUE9.pdf),
dated 2025-01-15. Part 1 contains the rules. Part 2 contains the dictionary.

On this computer, the PDF and its searchable text are in
`~/.cache/asd-ste100/issue-9/`, with these names:

- `ASD-STE100_ISSUE9.pdf`
- `ASD-STE100_ISSUE9.txt`

If these files are not available, get the PDF from the official link. Extract text locally.
If the web reader fails, try a direct download with `curl -L --fail`.
Use available PDF tools. Do not send project text to an external service for this task.

If extraction changes a table's order, read the applicable PDF page.
Keep the standard and dictionary outside the repository.

Read the rules and dictionary instructions one time per task. Before each output, examine the text.
Use the dictionary to make sure that each general word has the permitted part of speech, form, and meaning.
Use section 1 for technical nouns and technical verbs. Use the same name for each item.
If the rule summary does not give enough information, read the full rule and its examples.

Use these sentence limits: 20 words for instructions and 25 words for descriptions.
Use section 8 for the word count. Keep the meaning and exact technical content.

`tools/ste_check.py` uses approximate checks. A passing result does not prove full compliance.
Do not use the script as a dictionary. Do not give results for checks that did not occur.
If a reference cannot be read, give the limit and continue work that does not depend on it.

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
