# NIGHTLY publication runbook — DRAFT-for-QA

> **DRAFT-for-QA.** Not adopted. Does not grant development, build, integration,
> publication, device or access authority. Where this draft and
> [the current owner contract](rules/current-owner-contract.md),
> [workflow](rules/workflow.md) or the NIGHTLY repository's own rules disagree,
> those rules win. Items marked **OPEN** need an answer before adoption.

## Purpose

Describe how an exact firmware build reaches the NIGHTLY installer so the owner can
run the **first hardware test**. NIGHTLY DEV candidates do not need a prior physical
device PASS, a production `main` merge or a separate per-build owner approval; they
do need the software, package and distribution checks below. Production promotion to
firmware `main` has separate requirements (exact same-byte, affected-model owner
hardware PASS and separate production approval; see the owner contract).

## Repositories and where things live

| What | Where |
|---|---|
| Firmware source | `gobbajogger/X0X37-for-Loopy-Pro` (tracking issue #832) |
| Hosted firmware packages | firmware repo branch `artifacts/nightly-dev`, append-only, layout `artifacts/nightly/dev-<YYYYMMDD>-<tag>/<sha256>/x0x37-{elite,pro}-dev.fwsc` |
| Catalog / status / history source | `gobbajogger/x0x37-for-Loopy-Pro_nightly`, owned lane → PR → nightly-repo `main` |
| Live installer and catalog | nightly-repo branch `gh-pages` (`versions.json`, `package-version-assignments.json`, `build-status.json`, `install/`) served by GitHub Pages |

Nightly-repo main docs/versions.json and root preview-catalog.json are an empty preview.
They are not the live catalog. The nightly repository never contains `.fwsc`,
`ctl.img`, SDK/toolchain bytes or credentials; the catalog only references immutable
HTTPS package URLs.

## Role chain

| Step | Role | Does | Does not |
|---|---|---|---|
| Prepare / freeze | sole firmware integrator (#698, `wip/one-build`) | selects one exact candidate commit/tree | — |
| Build | `native_build` (Build owner) | real Elite/Pro DEV builds and matching controller with pinned JieLi 4.0.1 + AC79 V1.2.1 | substitute AC79 V1.2.0 |
| QA | `independent_qa` | exact-input review and gate receipts | author the change it reviews |
| Git verification | **OPEN-1** | — | — |
| Catalog source | `catalog_sync` | catalog/history/status source on its own lane, PR to nightly repo | write `gh-pages` |
| Nightly main merge | `website_main_merger` | merges eligible reviewed PRs into nightly-repo `main` | Pages publication |
| Publish | current sole publisher named by the owner map | prepares the candidate site, non-forced fast-forward of gh-pages, verifies live HTTPS bytes | does not resume historical website_publisher |
| Install | owner | installs on hardware | — (agents never install or flash) |

`safe_merge_intake` supports but does not replace the integrator or publisher.
Having repository push access is not permission to bypass these roles.

## Steps and preconditions

0. **Freeze the candidate.** Precondition: the integrator names one exact full commit
   and tree. Do not mix in unfrozen composition.
1. **Build (`native_build`).** Precondition: authorized access to the pinned inputs
   (see [BUILDING.md](../BUILDING.md)). Build Elite and Pro and the matching
   controller; record sizes, SHA-256 and memory/flash budgets; run host regressions.
   For full qualification the 0 fail / 0 skip bar applies; NOT_RUN does not meet it.
2. **Host packages (`artifacts/nightly-dev`).** Add only new files under the
   digest-addressed layout; never change earlier artifacts. Record the full hosting
   commit SHA. Re-download and check size and SHA-256 (no redirects). **OPEN-2**
   (which role writes this branch).
3. **QA (`independent_qa`).** Exact-input acceptance of the same source and packages;
   PASS receipt with immutable `evidence_url` for each required gate (as listed in
   the nightly repo's `docs/qualified-nightly-candidate.md`). Clear any publication
   HOLD only through its owner.
4. **Catalog source (`catalog_sync`).** On its own lane: append-only catalog/history/
   status update (keep earlier versions); run the nightly repo's catalog checks; open
   a PR to nightly-repo `main`. Catalog entries are written only in this authorized
   step.
5. **Merge (`website_main_merger`).** Merge the eligible reviewed PR; read back the
   remote commit/tree; hand off to the publisher. A merge is not publication.
6. **Publish (current sole publisher named by the owner map).** Pin the reviewed source commit/tree; prepare
   and verify the candidate with the nightly repo's documented scripts
   (`npm run prepare:candidate`, `npm run verify:pages`), output outside the repo;
   non-forced fast-forward `gh-pages` through the approved route; verify the live
   installer HTML, `versions.json`, downloaded package SHA-256 and build status over
   HTTPS. A source merge or a Pages Git update alone does not prove served bytes.
7. **Owner install.** The owner opens NIGHTLY Pages, chooses the model, accepts the
   DEV / DEVICE NOT TESTED notice and installs.

## Evidence to record at each handoff

- full 40-character source commit and tree; hosting commit; nightly-repo merge
  commit/tree; `gh-pages` commit
- per model: product identity, package size and SHA-256, controller SHA-256 and
  strategy, budgets
- every check as PASS, FAIL or NOT_RUN with command and exit status; a check that
  runs and fails stays FAIL
- QA receipts with evidence URLs; served URLs and hashes; next role/action

## Stop conditions

Stop and report (do not work around) when: the candidate is not frozen; a required
build, test or gate is FAIL or NOT_RUN (record history only and leave the
installable picker empty for that candidate); downloaded bytes do not match the
recorded SHA-256; an input needs access that has not been authorized; a tool or
route is denied; or a step's owner is unknown or another writer holds the scope.

## Never

- change catalogs outside the `catalog_sync` → merger → publisher steps
- commit secrets, credentials, raw captures, private samples, proprietary SDK or
  toolchain bytes, or firmware binaries to source branches or the nightly repo
- force-push, rewrite `artifacts/nightly-dev` history, or write `gh-pages` except as
  the sole publisher
- publish firmware without the documented steps, or use GitHub-hosted Actions or
  change `.github/` for firmware builds/tests or publication
- install or flash devices, or present NIGHTLY as production

## OPEN questions

- **OPEN-1:** Is there a "Git Verifier" role, and which registry role/automation is it?
  The sources list `independent_qa`, `catalog_sync`, `website_main_merger`,
  `website_publisher` (historical; not resumed), `native_build` and the sole integrator only.
- **OPEN-2:** Which role commits new packages to `artifacts/nightly-dev`, and through
  which approved route?
- **OPEN-3:** Who builds the private qualification object input for
  `prepare:candidate`, and where is it kept (it is not published)?
- **OPEN-4:** What are the exact `--catalog`, `--qualification` and
  `--retained-catalog` inputs for the publisher commands?
- **OPEN-5:** What is the approved route for the `gh-pages` fast-forward?
- **OPEN-6:** Is the required-gates list authoritative in the nightly repo's
  `docs/qualified-nightly-candidate.md` or in its script? The sources list it differently.
- **OPEN-7:** Should this runbook live in this repository, or in the nightly
  repository next to `docs/nightly-first-development-gate.md`?
