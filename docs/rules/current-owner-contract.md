# Current DnBOX owner contract

Checked against owner instructions through 2026-11-05 (UTC). Read this with the
actual target checkout's `AGENTS.md`, selected skill and exact candidate evidence.
This index reconciles process and ownership; it does not qualify a build or move
an integration ref. Historical plans, exports, rosters and status dates are evidence.

## Ownership and execution

The owner's 2026-11-05 assignment gives **Grok Bot development and delivery from
start to finish**, including arranging source pushes, merges and publication
through the assigned integrator, independent QA, Build and publisher. After the
2026-11-05 handoff (~07:22Z) and owner immediate-start (~08:19Z), Grok Bot is
the primary coordinator; DnBOX Docs writes rules/docs; DnBOX QA reviews exact
trees; no one waits on dot. Do not restart dot's development workers or
take over Grok's source work. Current accepted specialist scopes and original
reviewers remain; preserve unknown review budgets.

Work in the **cloud**, using a qualified supported executor and its actual tools.
The owner's later 2026-11-03 cloud/Codex and 2026-11-04/05 Grok directions
supersede classic-only, no-Codex/Work/bot-execution and fixed-orchestrator routing.
Do not use or transfer this work to the owner's Mac. Platform names do not prove
build capability, and a workflow mode grants no device access.
Heavy work (builds, full test runs, large clones) runs on Cursor cloud agents,
each on its own VM, not on the shared box; the shared box holds only light files.
Each agent works in its own checkout or scratch. Shared state is GitHub non-main
branches and tracking issue #832; handoffs go directly between agents. (Relayed
by the coordinator; owner confirmation pending.)

Use **meaningful stages**, with parallel subagents for disjoint ready work within
verified capacity. The owner's 2026-11-03 clarification requires new stage threads
and an end-of-stage handoff, not a fresh thread for every task. Docs-only and rules
typo fixes may finish inside the current Docs or QA stage; open a new stage thread
only when the stage boundary or owner changes (the documentation review rules
below still apply). Complete the
current stage's necessary work and proof together. At its boundary, preserve exact
inputs, results, owned paths, next owner/action and supported result reader; release
the old writer before the receiver writes. Verify actual receiver consumption.
Send each handoff directly to the receiving agent as a priority message carrying
the exact SHA/tree and the next action; a parent wake is informational only.
(Relayed by the coordinator; owner confirmation pending.)
Unavailable thread creation remains a precise blocked handoff, not a fabricated
launch or permission to duplicate work. Continue independent authorized work.
Follow the [workflow routing](workflow.md) for the target-specific handoff procedure.

## Source and release boundaries

Grok and the assigned sole integrator must identify the actual staging/delivery
branch and exact candidate. `wip/one-build` is an existing staging lane; its
presence does not replace firmware `nightly` development delivery or authorize a
production `main` promotion. The 2026-11-05 start-to-finish push/merge/publish
instruction supersedes the blanket ban on NIGHTLY delivery. It does not permit a
regular worker to merge shared refs or bypass current checks, denials or protections.
This documentation change does not choose or move Grok's integration branch.

Deliver reviewed source on owned non-main branches through the authorized GitHub
API route and verify remote head, parent, tree and changed bytes. Do not retain
authorized completed source only in a local commit. Preserve the candidate's
no-network-Git and no-`.github/` changes constraints. Never force push.

Firmware integration and packaging qualification retain the full host suite and
both Elite and Pro native builds with **zero failures and zero skips**. Required
missing or NOT_RUN evidence blocks that qualification. A box-status string or PR
comment does not replace actual exact-input proof. Docs-only work needs content,
link and policy checks; it needs no firmware build. Scoped intermediate checks
are useful evidence without becoming full qualification.

For 0.0.24.5/0.0.25, read the current [single ROADMAP worklist](https://github.com/gobbajogger/X0X37-for-Loopy-Pro/pull/812)
at its freshly verified full head and keep existing item owners. Preserve M10/M11/M12,
selected fixes, all already-built features, both-model measured free pool of at
least 8,192 B at required steps, `X0X_DEV_PADCAL=0`, SDK V1.2.1, controller/package
identities and existing safety/performance gates. The [November 1 public relay](https://github.com/gobbajogger/X0X37-for-Loopy-Pro/pull/812#issuecomment-6557286905)
is provenance for that task's inputs, not independent authorization, proof of a
gate PASS or a later repeal of cloud/Grok ownership. Keep unresolved M12 final
candidate/ROADMAP and actual uploader enforcement pending; do not repeat resolved
source-only reviews or invent an updated final build status.

Qualified NIGHTLY is for the owner's **first physical hardware test**. It requires
every applicable pre-publication gate and lawful immutable artifacts; prior device
PASS and another per-candidate chat reply are not required. The assigned publisher
then verifies actual served page/catalog/package hashes before recommending an
installation. Keep prior safe versions. Agents never install or flash. Production
firmware/installer behaviour still requires exact same-byte, affected-model owner
hardware PASS and separate production approval. Follow the [NIGHTLY gate](https://github.com/gobbajogger/x0x37-for-Loopy-Pro_nightly/blob/main/docs/nightly-first-development-gate.md)
at the verified remote SHA of the nightly repository's default branch `main` (or
another ref the owner names); record that SHA in the evidence (the earlier reviewed
pin was `a8205aff483dce09414093defdc19cf79d57b719`)
and current target rules; source push, merge, publication, live bytes and device
acceptance remain separate.
Keep existing NIGHTLY catalog entries and history unchanged, including the current
0.0.24.5 entry. A new entry is added only through the documented catalog_sync and
website_main_merger steps and the current sole publisher (role ids as recorded in
coordination/automation/roles.json on branch coordination/merge-queue at
`4a35a6cdaa0f112db6b3f577542138afd170c20d`), and only
when every required check is PASS (0 fail / 0 skip; NOT_RUN is reported honestly and
is not PASS). The current sole publisher is whoever the owner map names; that file
records website_publisher as historical, and this sentence does not resume that
identity. The docs role never edits catalogs or publishes. (Relayed by the coordinator.)

Preserve feature, model, sample-quality, data, licensing and binary protections.
Existing synth `*_private.h` filenames are not a deletion instruction. Keep the
approved headers and notices; new evidence about actual contents or rights needs
component-specific review. Exclude secrets, raw captures, unrelated private data
and proprietary SDK/toolchain/DSP bytes from public delivery.

## Documentation maintenance

Grok's dedicated **DnBOX Docs** bot maintains docs and rules after this cleanup.
Its creation/role confirmation was reported by the project coordinator; this file
does not claim an independent runtime or subscription check. DnBOX Docs is the
current docs writer after the 2026-11-05 handoff (~07:22Z) and owner
immediate-start (~08:19Z). Transfer maintenance with the exact reviewed
patches, source pins, checks, unresolved conflicts and ownership receipt. Reconcile any
existing docs-consistency role and accepted scopes once, at docs-role takeover,
and record the result; later edits check only the ledger for the touched scope; re-reconcile if a new
docs role appears. Do not create a competing writer or scheduler.

For each later owner decision, DnBOX Docs records its source and chronology,
updates the affected rule in its single home, fixes entrypoint links and explicitly
supersedes conflicting text while preserving history. Use the maintained docs
skill. Rules, process or ownership edits, and any edit that changes the meaning of
a rule, need exact diff/link/policy checks and independent QA review on the exact
tree. Pure typo, link or wrapping fixes that change no rule meaning need diff/link
checks; independent review is optional unless requested. If in doubt, treat the edit as a
meaning change; record the classification and the exact diff; QA may reclassify it. Documentation does
not grant development, integration, publication, device or access authority.
