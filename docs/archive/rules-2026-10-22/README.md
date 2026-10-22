# Project rules and skills archive — 2026-10-22

The owner requested a temporary archive of all project rules and skills, not deletion.
This directory is historical documentation. Its archived rules and skill text are not active instructions.

## Preserved contents

The root AGENTS.md, all tracked skills and their licenses, skill-discovery symlinks,
legacy rule entries, rule indexes, saved project instruction text and owner-preference
record were moved here without changing their Git blobs or file modes.
SKILL.md filenames have an .archived suffix to avoid skill discovery.
The exact original and archived paths, Git blob IDs and modes are in restore-manifest.json.
The source is commit 3200990e0df914494c6311ad2cc8bc0cb71ec8b0.

## Restoration

For each manifest entry, move archive_path back to original_path, preserving its file
mode and symlink target. Stop if an original path already exists; compare it rather
than overwrite newer work. Verify that each restored file has git_blob_sha using
Git's blob hashing (hash the link target text for a symlink). Restore all entries
together so relative skill-discovery links point to their original targets.
A repository owner can also revert the two archival commits on a separate branch.

## Scope and validation limits

Only project instruction and skill files were moved. Firmware, installers, tests,
validators, build scripts, product manuals, historical audit/evidence and earlier
archives were not changed. Built-in platform skills and application approval rules
are outside this archive. Original branches and Mac working copies are unchanged.

Some documentation links still point to the archived paths. Skill-presence and
rule-rollout checks may report missing active files on this archive branch; these
checks were intentionally not weakened or rewritten to conceal the archival.
No firmware-build or hardware-test result is implied by this documentation change.
