# Internal process notes

This folder holds agent and process scratch that used to sit at the repository root: BLE install-attempt logs, Miami optimisation notes, status snapshots, local stage handoffs, and a small audit folder.

It is not user documentation. For public docs start with [README.md](../../README.md), [CREDITS.md](../../CREDITS.md), and [docs/MANUAL.md](../MANUAL.md).

`cloud-handoff/` stays at the repository root because `tools/stage_handoff.py` and `tests/host/test_stage_handoff.py` read `cloud-handoff/pipeline.json` there. `loopyx0x37-classic-build-skill.zip` also stays at the root: the GitHub file connector used for this tidy cannot rewrite that binary as a rename. `evidence/`, `skills/`, `.agents/`, `.cursor/`, and `experiments/` were left in place after a reference check (test evidence, agent skill paths, Cloud Agent environment, experimental sources).
