# Nightly installer preview — repository rules

- This repo contains **public GPL-3.0-only** browser-installer source and a source-only, no-hosted-firmware preview. Retain author notices and license.
- Never add vendor JieLi SDK/toolchains, DSP reference trees, user samples/backups, credentials, firmware `.fwsc` images or `ctl.img` artifacts.
- The preview catalog must stay empty unless the owner separately authorizes an exact validated nightly firmware candidate and its distribution checks.
- After UI changes run `npm run build`, `npm test`, and an applicable responsive browser preview before pushing.
- The production `gobbajogger/X0X37-for-Loopy-Pro` repository, its `main`, and `gh-pages` remain separate destinations; do not modify them from here.
- Never force-push, publish stable firmware or flash hardware. Main updates here require explicit owner approval.

## Remote-first development delivery (owner instruction, 2026-10-23)

- After applicable checks, **every substantive validated commit must be pushed** to an authorized isolated development branch (create a new isolated branch when needed), with a remote branch SHA/read-back verified against the intended commit and diff. A commit existing only in a local checkout or ZIP is **not delivered**. Do not finish a Git-delivery stage or claim 'pushed' until that verification passes.
- To avoid stranded local commits, do not make a final local commit unless it can be delivered through a permitted branch. An unfinished patch can be retained as a clearly labeled **not-yet-committed draft** with tests/evidence; if a committed patch cannot be pushed, mark **GIT DELIVERY PENDING — NOT PUSHED** and preserve the exact error. Never claim an unpushed candidate was integrated or published.
- One safety-rejected GitHub edit is a **specific rejected operation**, not evidence that other authorized Git writes or future scheduled development are unavailable. Do not evade that refusal with a different API, endpoint, disguised payload, or branch. Continue other independently safe native work and permitted Git deliveries. Retry the refused content only after legitimate resolution of the refusal.
- A development-branch push is **not** a merge to `main`, an edit to `gh-pages`, a deployed site, firmware distribution, or hardware verification. Existing review, licensing, empty installable catalog, physical safety, branch-ownership and Pages-publisher gates remain in force.

## Scheduled full-cycle plugin workflow (2026-10-23)

- For this project's scheduled ChatGPT development workers, activate the **current installed LoopyX0X37 Classic Build plugin's `loopyx0x37-full-cycle` workflow** and its `references/full-cycle-execution.md` contract; use the connected **GitHub** plugin plus local native execution to perform the actual work. Reading a skill, announcing a plan, or saving a local ZIP is not a completed cycle.
- Every applicable cycle must inspect the exact live source and relevant Git handoffs, implement/review the scoped change, run appropriate local static/regression/browser checks and firmware/controller builds **only when applicable**, verify provenance/data safety, make a non-force isolated development-branch commit, and independently read back the remote branch commit SHA and changed file content. Persist a discoverable Git handoff for the next scheduled run when another role must act.
- A source branch does not automatically update this site's `gh-pages`. Only the designated NIGHTLY Pages publisher may commit specifically authorized checked site output, preserving existing assets. The installable catalog stays empty until candidate-specific validation and distribution approval; the complete NIGHTLY build/status inventory may remain read-only. Production firmware choices additionally require matching real hardware evidence and owner approval.
- If the installed plugin's newly added workflow is not yet available in this execution's skill listing, obtain the current authorized plugin instructions or use the maintained project workflow **without pretending the plugin executed anything**. Keep independently runnable work moving. Never sidestep an explicit safety-denied GitHub write.
