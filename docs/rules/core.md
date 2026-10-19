# Core rules

### Rule 1: protect data and sources

- Keep code on owner machines and approved local repositories. External pushes
  need an explicit owner request. Never push, merge, or package `refs-oscuro`.
  Keep proprietary JieLi SDK/toolchain files out of commits and bundles.
- Preserve licensing, target model guards, flash/RAM limits, features, and sample quality.
### Rule 2: process and edit ownership

- Use isolated lanes for edits; preserve others' work. Stop only your own
  processes by PID; never use `pkill -f` or `killall` on shared scripts.
### Rule 6A: finish authorized work

- Finish authorized work and relevant checks. Missing hardware or retired roles
  block only dependent steps. Name a real blocker and next action; do not repeat
  approval requests or poll without cause.
- Report only verified evidence. Distinguish source review, host tests, target
  build, formal gate, install, and device test; include inputs and unrun checks
  when relevant. Local completion grants no install or release permission.
- Use plain, correct English in all output, including saved text. Give the
  result first; remove filler. Preserve exact technical content, conditions,
  uncertainty, and evidence. Full writing skills load only for their triggers.
