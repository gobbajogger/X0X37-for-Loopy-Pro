# Work routing correction

Build: documentation only; no firmware input changes.
Replaces: classic-only task placement and per-stage launch rules from PR #720.
Checked against decisions: latest task direction to use fresh Work threads for
distinct tasks, avoid the owner's Safari and complete synthetic checks before
owner-operated hardware checks.

## Trigger, cause and effect

The integration entrypoint still prohibited Work builds and tests while selecting
a maintained workflow that permits capable current execution. Its routing adapter
also repeated the classic-only policy and offered a different firmware merge bar.
The source-retrieval guide described Work as excluded even though its native
Linux/Python/Git requirements can be satisfied there.

Task placement, synthetic testing and browser/device boundaries now have one home
in `skills/x0x37-task/references/routing.md`; `AGENTS.md` links to it. The firmware
qualification bar remains in `AGENTS.md`, with full host tests and Elite/Pro
builds at 0 fail / 0 skip. Documentation uses its existing content, link and policy
checks. The retrieval guide permits an authorized Work executor without changing
source verification or remote-write behavior.

This lets agents prepare and test development changes without requiring hardware
checks first. The owner still performs hardware checks. Source, independent
review, merge and publication remain separate actions. This correction changes
no firmware, integration destination, automation, private rule, `.github/` file,
access setting or protection. Rule 1m, data and licensing safeguards and actual
denials remain in force. Historical evidence is retained.

## Source pins and checks

- Integration source: `02cc7d104254b4427f257c310fb366bfc16a8e02` on
  `wip/one-build`; the same old routing is present in
  `build/0.0.24.5@8be16486e33f65b31c76710e1d825b6aa701c2c9`.
- Maintained policy: `nightly@66073c68dd80d51a7053432c2c161a8b720a1341`.
- Coordination evidence: `4a35a6cdaa0f112db6b3f577542138afd170c20d`.
- Check the exact diff, relative links and unchanged safeguards; run
  `git diff --check` and obtain one independent review of representative tasks
  before source delivery. Measure changed text before and after.
- No `SKILL.md` changes; skill frontmatter validation does not apply.
- Firmware builds, host suite, browser/device operations and publication are
  outside this documentation correction. Documentation checks certify none of them.
