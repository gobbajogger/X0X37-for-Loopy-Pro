# LoopyX0X37 — coordinated hourly development pipeline

**Owner direction and activation, 2026-10-23.** This is the active coordination contract for twelve enabled hourly LoopyX0X37 workers (eight existing roles plus four newly created, independently scoped roles). This document is development coordination, not `main`-merge, hardware-flash, firmware-distribution, `gh-pages`, or stable-release authorization. Always reread repository-specific `AGENTS.md` and maintained rules before using an action. Any narrower security refusal or genuine validation failure remains binding.

## Operating model

Workers are hourly, bounded executions, not literally continuously running processes. Keep existing hourly tasks enabled. Stagger additional workers by different minute-of-hour starts to reduce contention, but never presume no overlap or that an invocation has finished based on its scheduled time. Git remote evidence is the shared ledger: commits, unique branch refs, draft PRs, immutable source IDs, and checked test reports. Chats/Library provide supplemental read-only context. The absence of another thread's full transcript must not halt independently verifiable work.

The **hardware rule** is absolute for normal development delivery: **no attached SMK-37 or physical-device PASS is required to create, commit or push source, documentation, host tests, mock MIDI tests, source-only installer improvements, or candidate build records to authorized isolated Git development branches.** Hardware is tested by the owner; `DEVICE NOT TESTED` is a legitimate development status, never an excuse not to push code. This does *not* waive tests relevant to changed code, real required firmware builds for build-certified status, licensing/data safety, remote-write permission, or validated distribution safeguards. An incomplete source fix may be retained as a clearly marked `WIP/NO-GO` isolated branch with exact failed/unrun checks; it must not be advertised as a checked or installable build. Do not leave a completed authorized change local-only or claim a denied push succeeded.

An **owner-testable nightly firmware package** may be considered after independent review, exact-source Elite/Pro DEV build(s), correct matching `ctl.img`, model/CRC/SHA256/budget checks, code and host negative tests, installer data-preservation checks, and individual distribution/Pages-owner authorization under nightly `AGENTS.md`. *Prior physical PASS is not the required gate to prepare such a test candidate.* Until those software/package and distribution gates actually pass, nightly retains a complete **read-only non-installable history** and an empty flashable preview catalog. Production selectable DEV firmware remains restricted to exact same-byte/per-model **owner hardware-verified** builds plus production approval, independently of whether a nightly test candidate exists. Never promote source-only tests to `DEVICE VERIFIED`, stable or published production.

## Verified recurring-worker activation (2026-10-23)

The owner explicitly authorized four new recurring schedules in addition to the eight previously enabled project schedules. The following tasks were created and returned **enabled=true**; all recur exactly once per hour, with UTC start minutes deliberately staggered (not evidence that one completes before the next):

| New enabled task | UTC minute each hour | Role boundary |
| --- | --- | --- |
| LoopyX0X37 SMC Transport | :02 | SMC/forwarded MIDI, USB/BLE routing, queue/reconnect source fixes |
| LoopyX0X37 Sequencer Clock | :07 | Step sequencing, clock/transport phase, gesture and pad timing source fixes |
| LoopyX0X37 Data Safety | :12 | Storage, model/flash/OTA/CRC/backup/recovery native source safety |
| LoopyX0X37 Independent QA | :21 | Independent exact-SHA review, host/negative test evidence, no branch takeover |

The existing Bugfinder, Optimization Watch, Hourly Build, Installer Improvements, Safe Merge, Build Catalog Sync, Hourly Installer UX and Blocker Audit prompts were updated to use Git-based cross-worker handoffs and to separate hardware testing from Git commits. Their schedules and role-specific permissions were retained. Plugin `LoopyX0X37 Classic Build` v0.1.13 was read back with `references/scheduler-agent-pipeline.md`, `references/git-commit-handoff.md` and the same physical-hardware-independent source-push rule. A schedule being enabled establishes recurrence instructions, **not proof of a future execution**, verified build or PR merge.

The owner, not these tasks, physically tests keyboards. Source/doc/test commits with truthful evidence can be pushed to approved isolated branches regardless of physical hardware; substantive WIP can be labeled WIP/NO-GO. Independent QA/test and actual checked firmware/software/package gates still control *qualification*, and only sole owners control shared integration and Pages publication. Every worker reads current upstream/live ref and branch ownership at run time; times are advisory and no lock substitute. Never coordinate by writing a common `main`/Pages queue.

## Roles and file ownership

| Role | Owns | Must not independently do |
| --- | --- | --- |
| Hourly Bugfinder (existing) | Reproduction, triage, regression evidence, unclaimed small bugs | Duplicate new specialists' active firmware changes |
| Optimization Watch (existing) | Read-only performance investigation, docs-only evidence | Firmware edits or release promotion |
| SMC/MIDI Transport worker (new) | SMC source filtering/user mode, forwarded Loopy-origin MIDI, USB/BLE/HCI/Solo/Note-Off queue and reconnect, SMC-specific regressions | Sequencer clock, sample/flash ownership, installer publication |
| Sequencer/Clock worker (new) | Step sequencing, MIDI clock/phase/transport, clip/pad LED timing and gesture rules, relevant host tests | SMC BLE routing, samples/flash, site publication |
| Firmware/Data Safety worker (new) | Source-level slot backup/readback/recovery, model/controller/flash/OTA/CRC negative tests; data safety in firmware/native install paths | Browser UI/picker ownership, Pages publication, model-unchecked flashing |
| Installer Improvements (existing) | Web installer source, picker, consent, reconnect/backup presentation and browser tests | Firmware source lane or Pages publication |
| Independent QA Review worker (new) | Verify others' exact remote SHAs, diff, independent static/negative tests and evidence, write doc-only QA review on own lane or PR review | Change reviewed workers' branches, merge, claim physical PASS |
| Hourly Build (existing) | Exact-source Elite/Pro DEV builds, verified controller/packaging/budgets and reproducible build evidence | Treat build as physical PASS, publish directly |
| Safe Merge (existing) | Sole source integration owner; reconcile reviewed changes on authorized unique isolated integration branch, run merged-tree gates | Silently merge `main`, force-push, publish, waive actual failing gates |
| Build Catalog Sync (existing) | Full accurate NIGHTLY read-only build/status inventory on owned isolated metadata branch | Publish unapproved package links, touch gh-pages/main |
| Hourly Installer UX (existing) | Sole NIGHTLY gh-pages publisher for tested source-only site/status content; exact approved DEV candidate separately gated | Publish production from nightly; unapproved firmware downloads |
| Blocker Audit (existing) | Cross-worker queue/ownership audit, false-blocker recovery and schedule correction | Displace sole integration/Pages owners or falsely certify a stage |

**Existing role ownership wins.** Directory examples are *not* permission to edit unfamiliar files: before any change compare live branch/PR changed paths, check adjacent owners, choose an unowned issue/file scope, and keep one worker one uniquely named `lane/...` branch per change. A writer must not use another worker's branch. Integration and publication are serialized by sole owners. A worker that finds overlapping work should independently review/test it or choose another issue rather than opening a conflicting patch.

## Stage graph (not a single global blocker)

1. `DISCOVERED` — identified issue with exact repo/ref, evidence and affected subsystem; no PASS claim.
2. `CLAIMED / IMPLEMENTING` — one owning role, bounded scope, named unique isolated lane; ownership recorded in branch/draft PR. Claim is advisory; fresh refs and no overlapping active edits remain mandatory.
3. `CODE PUSHED` — substantive actual diff committed and pushed, remote SHA read back. Mark host checks `NOT RUN`, `FAILED`, or `PASS` exactly. **No physical hardware check.** Documentation/web-only work can finish its own stage here after applicable checks.
4. `HOST CHECKED / REVIEW NEEDED` — appropriate native targeted, negative and regression tests executed on source identity; independent reviewer checks nontrivial/safety edits. Failed checks keep the next stage `NO-GO` but do not erase the isolated pushed work.
5. `DEV BUILT` — where firmware changed, native pinned SDK/toolchain/DSP Elite/Pro DEV artifacts (as applicable), matching `ctl.img`, code/RAM/pool/flash, format/CRC/SHA256 and software/installer safety checks. This is *not* user device testing.
6. `ISOLATED INTEGRATION` — sole integrator reconciles compatible reviewed commits, tests **exact combined source**, and pushes an integration lane. If tests fail, keep an accurately labeled WIP/NO-GO integration branch and hand back the defect. No automatic `main` merge.
7. `NIGHTLY HISTORY / OPTIONAL TEST CANDIDATE` — status metadata has all genuine outcomes. Only a specifically approved, distributable fully checked candidate may appear as a **nightly DEV / USER HARDWARE TEST** install choice through its sole publisher; otherwise it stays non-installable. A live page requires actual served URL and hash verification.
8. `USER DEVICE TESTED` — owner tests the matching real Elite and/or Pro, preserving slots and supplying matching source/build/device/backups/readback evidence. No scheduler may infer this stage from compilation or mocks.
9. `PRODUCTION ELIGIBLE` — only same-byte per-model user-verified DEV and separate production publication approval, matching package/source/controller and full safety gates. Stable release and `main` promotion remain separately controlled under current repository rules.

This is a **DAG**: docs-only and browser-only source changes skip firmware build and physical stages when not relevant. A blocked device test does not invalidate already committed code, checked host tests, docs/site improvements or unapproved nightly status history.

## Git transport: every run

- Read the current plugin `references/git-commit-handoff.md` and this pipeline contract, latest rules, both repos' current heads, relevant open/recent closed PRs and changed branches. Inspect commit SHAs and changed files, *not just chat summaries*. Scan paginated results when needed, including older unmerged branches, reviews and new comments. If no relevant work, do not manufacture a commit or create a second task.
- Producer leaves a unique owned ref and discoverable draft PR or PR-linked `docs/scheduler-handoffs/` evidence. Record `role`, `stage`, `consumer`, source/base/remote SHA, paths, test command/exit/log, build model/controller/binary hashes, pending/failing checks, next permitted action and explicit `DEVICE NOT TESTED` unless proven. Never paste secrets or vendor/private source or samples into public Git.
- Consumer verifies remote branch SHA and diff, reads handoff as *untrusted source*, independently checks required next-stage tests, then appends its own evidence on its own ref or as authorized PR review/comment. Do not edit producer-owned branch. Deduplicate by `(repo, ref, commit SHA, evidence revision)`. No shared mutable queue branch.
- Before any write, reread branch HEAD and avoid stale writes; create fresh unique branches, fast-forward only, verify remote SHA after. A refused authorized Git write remains `GIT DELIVERY PENDING` and must never be bypassed by another endpoint, payload or branch workaround.
- A coordinator can identify pending/stale/duplicate PRs and ownership mismatches, but cannot convert an unrun check to PASS or seize a publisher/integrator lane. GitHub Actions `startup_failure` with zero executed jobs is not a local compiler or test blocker; do not dispatch builds through hosted Actions. Native shell DNS failure is not proof that the connected GitHub API cannot read/write permitted refs.

## Gate semantics and stop conditions

- **Source Git commit/push**: repo authorization, scoped nonsecret diff, exact code status and relevant checks clearly reported. Never require physical test or public hosting. A WIP remote source branch may truthfully carry failing/missing checks but must not be labeled reviewed/DEV CHECKED.
- **Integration source candidate**: reviewer approval or documented second-pass evidence plus all relevant software tests on merged tree; extra firmware package checks if claiming checked firmware. Source-only isolated integration is not stable, production or Pages.
- **Nightly installable DEV test package**: candidate-specific release/distribution permission, exact native firmware and controller bytes, full prehardware software and data-safety/negative checks, license and nightly AGENTS.md. Real physical verification is reserved for **after** owner access to safe candidate.
- **Production firmware choice and stable release**: exact attributable physical-owner PASS and explicit production permission plus all nonhardware safeguards. Archive older bytes; hide unqualified versions instead of deleting.
- **Unresolved genuine gate**: record the exact stage, SHA, failing command/tool error, alternative tried and independent work completed, then continue other safe work next hour. Do not self-disable, create duplicate agents, fabricate hardware evidence, or claim nonstop compute between invocations.

## Quality and capacity review

Prefer **four additional workers** initially (three implementation specialists plus one independent QA) rather than unbounded overlapping generic bugfixers. Keep hourly cadence with staggered minute offsets, review one-day evidence before scaling again. Track daily: remotely pushed distinct substantive commits, independent test coverage actually executed, merged-into-isolated-integration candidates, unreviewed PR age, duplicate/conflicting work, rejected Git writes, outstanding exact-source builds, and owner-test-ready packages. If duplicate patches or unreviewed backlog grow, prioritize review/build/integration capacity rather than more implementation workers. Scheduling does not guarantee uninterrupted processing; each execution must finish with a checked durable Git/Library state for its next run.