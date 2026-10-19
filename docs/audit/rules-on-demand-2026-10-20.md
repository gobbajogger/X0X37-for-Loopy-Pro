# On-demand rules — 2026-10-20

Trigger: the owner requested fewer rules, initialized only when needed, to
reduce wasted usage, stalls, errors, and regressions.

Cause: startup recursively required style, skill status, and two full skills.
Helper instructions repeated that requirement. Core also repeated install and
docs procedures, and docs edits unnecessarily loaded build setup guidance.

Change: startup loads the entry and core only. The routing table names each
step trigger, including hard-to-undo actions. Full writing skills remain pinned
and discoverable, but load only for their stated triggers. The concise default
still applies to all output. Move the fallback-install ban to install rules;
retain data, source, ownership, validation, and install guards. Preserve rule
IDs referenced by decision records. No firmware or enforcement code changed.

Reading size: whitespace-separated words across AGENTS.md and its required
startup reads fall from 2,700 to 401 (85.1%). This is a reading-size proxy, not
a measured token or usage saving. Step rules and optional skills are additional
only when relevant; automatic skill catalog descriptions are outside this count.

Checks on the candidate tree based on cf83a97:

- Rule rollout scratch test: PASS, including backups and repeat behavior.
- Installed skills: PASS, two skills and ten pinned source hashes unchanged.
- Preference lint: PASS, six entries. Initial missing style rule ID repaired.
- Linux rule quickcheck: PASS, 10/10 using x0x37-validation:local.
  The native commit-mode attempt lacked flock; Linux resolved that dependency.
- Docs-folder quickcheck: PASS; grammar hints remain advisory.
- Independent review covered read-only questions, docs edits, firmware fixes,
  installs, and delegated review. Repaired the missing hard-to-undo trigger,
  fallback-install ban, and style rule ID. Limited BUILDING.md to needed setup.
- git diff --check: PASS.

Rollout uses the maintained x0x37/docs source and tools/rollout_rules.py with
backups. Preserve the pre-existing FM-1 preference edit. Firmware integration
remains with its assigned chat. Existing chats use changed rules next time they
read the entry; no messages or restarts are authorized by this task.

## Applied result

Commit `f9666ea` was fast-forwarded locally to `smk37`. Rollout updated
13 workspace mirrors and checkout entries; a repeat proposed zero changes.
Backups and the complete hash manifest are in `/Users/mac/Documents/x0x37/repo-bak/rules-rollout-20261019T144450251996Z`.
The existing FM-1 preference edit remains uncommitted and byte-preserved apart
from this task's inserted decision. Final preference lint and committed-tree
docs quickcheck passed. Independent follow-up found no unresolved findings.
