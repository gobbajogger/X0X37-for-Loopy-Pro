# Strict STE references

Use this procedure only for the explicitly requested strict review. Return to plain writing afterward.


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

Read the rules and dictionary instructions one time per task. For the requested review, examine the selected text.
Use the dictionary to make sure that each general word has the permitted part of speech, form, and meaning.
Use section 1 for technical nouns and technical verbs. Use the same name for each item.
If the rule summary does not give enough information, read the full rule and its examples.

Use these sentence limits: 20 words for instructions and 25 words for descriptions.
Use section 8 for the word count. Keep the meaning and exact technical content.

`tools/ste_check.py` uses approximate checks. A passing result does not prove full compliance.
Do not use the script as a dictionary. Do not give results for checks that did not occur.
If a reference cannot be read, give the limit and continue work that does not depend on it.

