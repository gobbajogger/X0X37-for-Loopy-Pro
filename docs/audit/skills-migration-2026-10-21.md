# Initialize project skills when needed

## Trigger and cause

The owner requested this change in chat `01a15a76-4cff-7370-890e-f80672e744a2`.
The rule index routed tasks to Markdown rules. Those files did not expose workflow descriptions to skill discovery.

## Result

Ten project skills now contain the procedures. Each skill has a task description and a `SKILL.md` entry point.
The source files live in `skills/`. Discovery uses `.agents/skills/` links.
Automatic selection remains enabled. Running threads can initialize a skill through the exact path in the index.

The task and writing skills retain their existing startup scope.
Other skills load when their task step begins. A link does not require all linked skills.

The old rule paths preserve decision IDs. Each path points to its maintained skill.
The preference checker follows those pointers for local files and saved Git revisions.

Project skill edits retain the existing lane and commit checks.
The rollout adds discovery links to each project checkout. It preserves unrelated instructions and old evidence.

It saves replaced files and links. It rejects blocked discovery directories before any write.
No public skill source, pinned hash, device procedure, or firmware file changes.
The maintained quickcheck retains its storage test selection.

## Policy review

The independent review compared the procedures with commit `36cf774`.
It covered SMC diagnosis, development validation and installation, stage handoff, docs edits, existing permission, and unrelated questions.

The review found a missing instruction to search product decisions and behavior. The task skill now includes it.
The review also found an incomplete rollout when a file blocked a discovery directory.
The rollout now checks those parents before any write. The scratch tests cover refusal and unchanged entry files.

The reviewer verified the repairs. No actionable finding remains.

Development remains the default. Release builds still require an explicit request.
The full gate remains excluded from development work. Standing install permission, data checks, model guards, and firmware ownership remain intact.

## Checks and evidence

The affected checks cover discovery, pointer resolution, missing sources, backup safety, repeated rollout, and commit enforcement.
Eight preference tests pass. The rollout test passes.
All ten skills pass the bundled skill validator and the project skill checker.
The two writing skills retain their ten pinned source hashes.

All 18 hook cases pass. All 11 quickcheck cases pass in the existing Linux container.
Five portable test cases also pass in Linux.
The host quickcheck attempt stopped because macOS lacks `flock`. The Linux run supplies the required result.

The ten procedure files change from 4079 words to 4721 words, including skill metadata and initialization instructions.
The writing skill also contains guidance moved from the old source index.
This migration changes how instructions load. It does not claim fewer total instruction words.

[Size measurements and deployment evidence](../../../rules_skills_evidence_20261021/) record the checked files, results, and backups.
No firmware build, full gate, installation, physical-device test, external push, or publication ran for this task.
Device evidence remains unchanged.
