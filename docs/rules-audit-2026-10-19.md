# Project rules audit — 2026-10-19

Scope: project instructions, workflow docs, skill references, enforcement, and
workspace discovery. No firmware source changed in this repair. The existing
firmware integration task keeps firmware ownership.

## Causes and repairs

| File or entry | Cause before repair | Behavior after repair |
|---|---|---|
| Workspace `AGENTS.md` | Missing. A task in the wrapper had no reliable rule entry. | Names current rules, style, evidence levels, and task ownership. |
| Checkout `AGENTS.md` | Some copies had box-only rules. Others pointed at missing `docs/agent-rules.md`. | Current lineage has self-contained repo entry links. Older local checkouts use a relative workspace entry. |
| `docs/agent-rules.md`, `docs/rules/core.md` | Active integration lineage lacked these files. The wrapper still held old versions. | Canonical repo docs restored. Workspace copies are discovery mirrors. |
| `rules/commit.md` rule 9 | Required all work on the box, even with local Linux tools. Required a new test for every change. | Supported macOS host tests and local Linux target checks can finish work. Tests match the change. |
| `rules/commit.md` rule 10 | Required fetch/rebase and named-role transfer regardless of network or current task. | Isolated local commits and current ownership work without retired roles. |
| `rules/core.md` rule 16, `rules/shared-work.md` | Required a new helper per edit, fixed five-minute waits, and role notifications. | Bounded helpers, useful parallel work, and concrete dependency reports. |
| `rules/docs.md` rules 12/12A | Required four unrelated doc updates and a Docs Keeper/Orchestrator/smk37 cycle. | Update affected docs; one independent review; authorized task agent completes repair and rollout. |
| `rules/integration.md` | Hard-coded gate path and old role-only gate request. | Same formal checks can run in supported Linux with real configured paths. |
| `rules/install.md` | Mixed completed local work with install readiness and box transfer requirements. | Local work, formal gate, install, device test, and release are separate evidence levels. |
| `rules/device-bug.md` | Made owner INLOG and role handoffs prerequisites for every fix. | Use actual evidence and relevant regression checks; request missing device evidence only when needed. |
| `rules/owner-messages.md`, `rules/style.md` | Split skill scope and banned needed paths, IDs, and evidence. Required routing questions through an old role. | Plain correct English and concise caveman style apply to all agents and outputs. Preserve exact evidence. Use existing authorization. |
| `AGENTS_AND_ROOMS.md` | Old agent IDs, paths, and uninstalled proposals looked live. | Current task assignment wins. Original roster is archived unchanged. |
| `USER_PREFERENCES.md` | Chronology mixed old workflow commands with live instructions. | Current authority and superseding request come first. Original chronology remains intact. |
| `SCRIPT-CHECKS.md` | Proposed and live checks were mixed. | Current index names actual scripts and their validation scope. |
| `tools/quickcheck.sh` | Box-only defaults, forced grammar heuristics, overwritten caller reference paths, mandatory INLOG, and unguarded missing Mac lock tool. | Local repo and hook paths, caller reference variables, advisory grammar, proven source regressions, and fail-closed bounded locking. Native folder mode can check docs without shared cache writes. |
| `tools/gate.sh` | Box-only wording and clone hardlinks failed on the mounted local container filesystem. | Supported Linux/container path, explicit setup error on macOS, and private `--no-local` clones. Gate evidence requirements stay intact. |
| `tools/prefs_lint.py` | Re-read superseded historical sections as live owner rules. | Checks current entries and stops at the explicit historical boundary. Broken current rule links still fail. |
| `tools/ste_check.py` | Rejected necessary paths, hashes, questions, and detailed evidence in owner messages. | Empty content still fails. Style is reviewed without hiding evidence or blocking an authorized task on word heuristics. |
| `tools/pre-push.hook`, `tools/release_tree_check.sh` | Fixed hook runner and repo assumptions blocked valid local paths. | Configured local runner and repo work; missing PASS and protected source checks still fail closed. |
| Existing installed Mac hooks | Repair text pointed only to unavailable box notes. | Rollout repairs recognized notice text with a backup. Clone and proprietary-ref guard behavior stays intact. No new Linux runner is imposed on Mac pushes. |
| Workspace `BUDGET.md`, `LIVE_BRANCHES`, `ROADMAP.md`, task plans | Dated status and plan text could revive old role or machine requirements. | Authority notice preserves original facts and budget limits; live task owns current revision. |
| Writing skill paths | Only box paths were recorded; exact skills were absent from the Mac catalog. | `SKILLS.md` records recovery evidence and exact availability. No substitute is called the original. |

The old rules and roster are preserved byte for byte under
[archive/rules-2026-10-17](archive/rules-2026-10-17/). Preference history remains
in the maintained file too. Workspace backup files, repository history, logs,
dated design plans, and past checklists remain evidence. They do not become live
instructions when a new task reads them.

## Guards retained

Keep owner loop data, backup and read-back CRC checks, bootloader and write range
protection, serialized hardware access, model guards, licensing, and proprietary
source restrictions. Keep exact gate pins and validation tree/artifact checks.
Keep static RAM free at zero or more and pool headroom at 8192 bytes or more.
Release needs the required formal gate and exact-build device evidence. A local
test cannot supply that evidence.

## Validation and rollout

The [independent final review](audit/x0x37-rules-final-review.md) found no remaining
blocking defect. It checked Mac work, Linux/container work, missing optional
references, unavailable hardware, and helper handoffs. All 14 archived originals
matched their source bytes. Current rule, entry, and skill-status links resolve.

Native Mac rollout, preference lint, docs folder mode, missing-lock fail-closed
behavior, and gate setup reporting passed. Local Linux checks passed: rule
quickcheck 10/10, grammar 13/13, regression evidence 9/9, portable rules 5/5,
install evidence 21/21, and gate process safety 5/5. Independent checks also
passed for release-tree protection 14/14 and remote allowlist 13/13. See the
[enforcement record](audit/enforcement-2026-10-19.md) for scope and evidence.
These are development checks. They do not certify firmware for release or claim
an install or device PASS.

The [inventory](audit/inventory.md) covers 35 checkouts and 485 instruction and
workflow records at its snapshot. The [enforcement inventory](audit/enforcement-inventory.json)
records source script hashes and the existing installed hooks. Scripts with
legacy default paths remain usable through their documented local parameters.
`BUILDING.md` gives concrete gate, hook, status, and install-guard paths. Old status
roster labels are diagnostics; they do not assign work or prevent completion.

Exact `asd-ste` and `caveman` originals remain unavailable. All 46 readable source
archives and the local validation container were checked. No configured original
box endpoint or authenticated upstream was found. [SKILLS.md](SKILLS.md) records
the recovery evidence and the exact files needed. No substitute was installed.

`tools/rollout_rules.py` uses a dry run by default. Apply saves replaced bytes and
a hash manifest under the workspace `repo-bak/rules-rollout-*`. It updates only
instruction entry points, rule mirrors, authority notices, and recognized stale
Mac hook repair wording. It excludes
third-party references and `refs-oscuro`, and never writes firmware source.
The scratch test checks dry run, backup, repeat behavior, and discovery links.
