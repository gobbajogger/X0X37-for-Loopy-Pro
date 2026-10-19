# Rule reading and validation audit — 2026-10-19

Base: `82d0f49`. Lane: `lane/dk-rules-token-opt`.
Source: owner request in "Optimize project rules" to check the other rules chat
and minimize rule tokens while improving development speed and build reliability.

Read "Audit and fix project rules" and its final review/rollout evidence first.
Keep its supported-environment, honest-evidence, ownership, and completion fixes.

## Changes and reasons

| Cause | Change | Expected effect |
| --- | --- | --- |
| Entry, core, style, and skill recovery repeat instructions | One startup route; concise core/style/status | Less required context |
| Every task can load whole rule/history sets | Read step rules only when needed; search relevant product decisions | Less repeated reading; product decisions still apply |
| Skill search history burdens every task | Preserve full old status in writing-skill-recovery.md; link from short status | Evidence stays accessible outside startup |
| Rules repeat validation without naming reuse inputs | Reuse only matching source, config, tools, dependencies; retain required coverage | Avoid redundant checks without accepting stale evidence |
| Long wrapper/checkout instructions repeat maintained docs | Short generated pointers, same backup and discovery logic | Future rollout stays compact |

Sizes are in [rules-reading-size.json](rules-reading-size.json): startup words
1465 -> 675 (54% less); complete active rule set 4017 -> 2371 (41% less).
Characters: startup 9842 -> 4941; complete set 26095 -> 16704.
These measure reading size, not exact model tokens or measured build speed.
History, archives, enforcement behavior, and firmware source remain unchanged.

## Guard review

Core retains owner data, local-only code, proprietary exclusions, licensing,
model/flash/RAM limits, own-PID cleanup, and separate evidence levels. Commit
and integration rules retain exact references, supported environments, required
checks, dev/release coverage, static RAM >= 0 B and pool >= 8192 B. Install rules
retain whole queue, exact gate tree/artifact/configuration/CRC, zero skips/failures,
backup/readback, model/ranges, device lock, watched writes, and no bootloader writes.
Release still needs owner device PASS. Local test packages do not gain that gate.

## Checks and limits

- Native Mac rollout scratch test PASS: backups, discovery, third-party exclusion,
  repeat behavior. Preference lint PASS: 5 current entries.
- Native docs folder quickcheck PASS; 62 advisory grammar hints. No strict-skill
  compliance or exact missing-skill use is claimed.
- Linux x86-64 `x0x37-validation:local`, network disabled, workspace mounted at
  its host path: portable tests 5/5; rule quickcheck 10/10. Log:
  `rules-token-linux-checks.log` beside this record.
- Initial native portability run failed 2/5 because macOS lacks `flock`.
  The supported Linux rerun above passed; native failures are not hidden.
- Independent reviewer checked 31 links, unchanged historical preferences,
  preserved recovery evidence, whitespace, and representative Mac/Linux,
  missing reference/device, changed build input, and helper workflows.
- Reviewer found broadened "release/package" scope. Fixed to "public release or
  release package"; reviewer confirmed closure. No remaining review blockers.

No target build, formal gate, install, or device check was needed or run for
these docs/template edits. There is no new firmware or device PASS.

## Ownership and rollout

Use canonical `x0x37/docs` after local fast-forward. Apply the existing rollout
with backups; verify mirrors, pointers, and a zero-change second dry run.
Never push externally or edit the active firmware integration checkout's code.
The separate skill installation chat owns public-skill installation/provenance
and will rebase on this completed merge. This lane installs no skills.
Applied from canonical `x0x37` at `ecb1708`: 50 entry/mirror files updated.
Every changed path is a rule mirror or AGENTS.md. All replaced bytes match
backups; all mirrors match canonical docs; live integration/UX entry links
resolve; repeat dry run reports zero changes. Full manifest:
[rules-token-rollout-2026-10-19.json](rules-token-rollout-2026-10-19.json).
Committed native docs quickcheck also PASS (94 advisory hints after audit text
was included). No firmware scripts/source or hooks changed in this rollout.
