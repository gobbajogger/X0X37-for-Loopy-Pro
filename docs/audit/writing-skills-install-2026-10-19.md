# Public writing skill install — 2026-10-19

Trigger: the owner requested Caveman and simple English skills for every project
agent, then explicitly chose public skills after the original-source gap was explained.

Cause: current rules required both styles but had no skill files. Original-only
recovery instructions prevented use of public copies. Identical docs could also
route a checkout to missing local skill files.

Change: install pinned public Caveman and ASD-STE100 text with references and
license notices. Keep upstream bytes unchanged. Record every hash and source path
in `skills/sources.json`. Add `.agents/skills` discovery links. Require both files
at task start and in helper handoffs. Project overrides keep grammar, uncertainty,
conditions, exact technical content, and the owner's all-output scope. Rollout
now checks skill files before selecting local instructions.

Consulted “Optimize project rules”; it found no original sources and owned rule
compression. This lane builds on its final `716e760` revision. Firmware work stays
with the integration chat. Public copies supersede the old recovery-only rule;
old recovery records remain intact.

Install used the system skill-installer download workflow with pinned revisions.
The project includes the runtime text, references, README files, and licenses.
ASD-STE100 release workflows, test utilities, and evaluation files are omitted.
Caveman companion modes and runtime programs are not installed. No global skill
install, shell hook, automatic update, or external push was performed.

Checks on the candidate tree:

- All 10 installed source files match bytes fetched from their pinned public revisions.
- `python3 tools/check_writing_skills.py`: PASS. A changed skill file in a scratch copy fails its hash check.
- `python3 tests/host/test_rules_rollout.py`: PASS. Added missing/stale skill cases fail with the old rollout script.
- `python3 tools/prefs_lint.py docs/USER_PREFERENCES.md`: PASS, 6 entries.
- `docker run --rm -v /Users/mac/Documents/x0x37/rules_writing_skills:/work:ro -w /work x0x37-validation:local bash tests/host/test_quickcheck_rules.sh`: PASS, 10/10 cases.
- `git diff --check`: PASS.
- `X0X_QUICK_ONLY=ste X0X_QC_BASE=716e760 bash tools/quickcheck.sh /Users/mac/Documents/x0x37/rules_writing_skills`: PASS. The advisory wording check reports warnings, including unchanged upstream skill text and exact owner quotes. It does not certify STE compliance.
- One independent read-only agent review: PASS, no findings. Fresh startup found and read both skills, checked links and upstream bytes, and preserved uncertainty and install conditions in a sample. Mac work, absent hardware, missing skills, and helper handoffs remain clear.

Required startup rule text grew from 675 to 920 whitespace-separated words.
The two required skill bodies add 1,780 words, read once per task. References
remain on demand. This increase installs the exact skills the owner requested.

These are source and host checks. They do not prove future agent compliance,
strict ASD-STE100 certification, firmware builds, formal gates, or device results.

## Applied rollout

Skill install commit: `95a9e08`, merged locally on `smk37`.
Verified 38 workspace entry points and two existing external worktree entry
points. They reach the current skills through local docs or the workspace
wrapper. Two workspace `.agents/skills` links expose the canonical copies to
new turns. The six changed rule files and their backups match recorded hashes.
A repeat rollout changes zero files. See [rollout evidence](writing-skills-rollout-2026-10-19.json).

Existing chats must read the changed rules on their next turn. No completed
chat was restarted. The instruction and discovery checks cover every current
project checkout, but cannot force an agent to obey or certify every reply.
