# M12 / #822: acceptance gaps reproduced; isolated proposal

The assigned combined source has not adopted historical #822's completeness gate. Current strict evidence tooling passes its own structural checks but lacks three independently reproduced release-acceptance boundaries. The isolated repair proposal is PROPOSAL.md; test_m12_boundaries.py is an executable, deliberately failing acceptance fixture. No firmware, ROADMAP, shared tooling, publisher or release policy was changed.

## Exact inputs and ownership

Repository: `gobbajogger/X0X37-for-Loopy-Pro`. Assigned source `94197fc839b00aff47b98ca02f3c6396e4a6eabf`, tree `72d28fbc741244d18fa3d18495da16ab0fcb1d24`. Canonical ROADMAP `12e72b586c31e7ed412b6f4315bdf408c6f712b0`, tree `d7036a7882dec35eecf6158297d264eecbbb0c2e`, ROADMAP blob `923cf5a243ab4d6abe59ed493b506c3a928ca661`. The source's own older ROADMAP blob is `8f6e368e2f3a36e606ab516bd1ce9af6325e39cd`; it is not substituted for the supplied canonical document.

Historical #822 is closed and unmerged, head `46d3e95251f145eed6d6d8a67e66d4942d03ad26`, tree `a7bbf33fecc2552d081344513c91997ff044057a`. Its [closure](https://github.com/gobbajogger/X0X37-for-Loopy-Pro/pull/822#issuecomment-6813747715) says it was stale against 0.0.24.5 and superseded by #864. It is provenance, not a current merge request. The checked-in historical selection still says 0.0.24.5 and ROADMAP `70b5a0332ad008d6c0bd7323ada2e94a36e0980a` / `4bfb141f569058b1407d5d4684e5f8ada3eb4067`.

Parent thread `01a1af1b-2207-7017-b4fb-4e7f6c2c90ea` assigned this isolated acceptance stage and remains routing/integration consumer. Shared firmware, ROADMAP and publisher ownership stays with the existing owners. Independent read-only review was performed by `/root/m12_independent_review`; see REVIEW.md. Live development policy was read at `nightly@66073c68dd80d51a7053432c2c161a8b720a1341`; the connected API returned 404 for `coordination/merge-queue`. That read failure does not establish absent roles or grant shared-ledger ownership.

## Actual adoption

Complete recursive GitHub trees (`truncated=false`) show these paths absent from both assigned source and canonical ROADMAP revision: `tools/roadmap_completeness.py`, `docs/roadmap-source-evidence.json`, `docs/release-selection.json`, `tests/host/test_roadmap_completeness.py`, `tests/host/test_roadmap_release_scope.py`. All exist at historical #822. See evidence/adoption.json and evidence/input-manifest.json.

Current `tools/package_nightly.py` (blob `5f5fdb9c6cbccc8d7715c15c0c16cb9dcf10cca2`) has no completeness/selection arguments or hook and reads the package at line 244. Current `tools/publish_nightly.py` (blob `3f749aae41182546988a38a504d65922ea83f4dd`) preserves its validation/local-preview guards and refuses legacy push, but contains no ROADMAP completeness hook. No live uploader enforcement is established by these files. Its owner must prove the actual external boundary separately.

Current `tools/publish_pages.sh` delegates `--nightly` to Python but retains an unguarded legacy non-nightly force-push/Pages path. Historical #822 retired that path; the current tree did not adopt the retirement. This is source inspection only: the shell path was NOT_RUN and is not authorized to execute. Disposition remains with the existing publisher owner.

The combined source does adopt a separate `tests/host/roadmap_evidence_check.sh` strict mode (blob `17da00da9320cb875da9ea811fb531196673d8a8`) and self-test (blob `b97ac6b975aae5cffe2de8ed343616f4a1733658`). `tests/run_tests.sh:387` registers the self-test as `roadmap-evidence-check`; that is not release-candidate evidence acceptance. Strict mode checks local REQUIRED_ROWS inventory, statuses, pin agreement, paths, claimed source_sha and literal runner names. It does not read candidate Git objects, independently expected ROADMAP pins or independently accepted release selection. A literal registered test and caller-authored PASS do not establish execution.

Historical strict-v2 module blob `ceaba973b14b6d20b2805a6a62cf867019a2ee15` requires historical Done and Master tracker tables; direct parsing of the current canonical ROADMAP raises `ROADMAP must contain both the historical Done and complete Master tracker tables`. This is an input-format incompatibility, not a parser false pass. Its old DONE-prefix scope rules also cannot serve as current release policy.

## Recovered evidence and new execution

Existing proof was recovered before new tests. The [historical review](https://github.com/gobbajogger/X0X37-for-Loopy-Pro/pull/822#issuecomment-6559760035) records core 9/9 and release-scope 15/15, resolving earlier limited-packet 13/15 dependency failures. The later [exact-source review](https://github.com/gobbajogger/X0X37-for-Loopy-Pro/pull/822#issuecomment-6651830062) executes the repaired external source/live kernel on `46d3e952`. Those are scoped historical receipts, not current candidate acceptance. Historical 202/203-row reports, the 2 PRESENT / 144 MISSING / 62 QUEUED audit and current tree's older 146-row PASS2 report do not prove this release complete. The stored PASS2 self-test log only had five checks; the actual current self-test is larger.

Scoped Linux/Python 3.12.14 execution used 33 text inputs whose Git blob and SHA256 identities were verified. This is a recovered dependency-complete set for the focused tests, not a full firmware checkout. Existing fixture author identities were preserved only in disposable test repositories; no custom author was set for canonical source delivery.

| Check | Result | Limit |
|---|---|---|
| Historical `test_roadmap_completeness` + `test_roadmap_release_scope` | PASS 24/24, exit 0 | Exact historical tooling and restored import closure; synthetic Git/package/external-read fixtures |
| Assigned-source strict checker self-test | PASS 42 checks, exit 0 | Structural/checker fixtures only |
| New M12 acceptance probes | FAIL, exit 1: 2 controls pass, 3 gaps | Desired boundaries deliberately remain unimplemented |
| Historical parser on current canonical document | REJECTED with explicit GateError | Safe refusal, incompatible layout |
| Current selected-release acceptance, actual uploader enforcement | NOT_RUN / NOT_ESTABLISHED | No accepted current policy supplied or adopted adapter |
| Firmware suite/builds, package reads/downloads, live publication, devices | NOT_RUN | Outside this assignment |

The new acceptance fixture holds an independent synthetic selected set of `#1/#2` fixed. This does not claim those IDs are the actual current release selection.

| Probe | Actual exit | Required acceptance exit |
|---|---:|---:|
| Complete structural fixture | 0 | 0 |
| Remove #2 JSON, retain inventory | 1 | 1 |
| Remove #2 JSON and shrink mutable inventory | 0 | 1 |
| Remove feature bytes but retain file path and claimed candidate | 0 | 1 |
| Replace all evidence ROADMAP pins with another consistent pair | 0 | 1 |

The shrink case keeps source SHA256, candidate string and independent synthetic policy SHA256 unchanged. Every fixture has no `.git` directory, confirming that strict mode alone does not authenticate candidate objects. Independent review repeated the probe and obtained identical result JSON. These are false acceptance risks if structural PASS is used as a selected-release gate, not claims that the structural tool violates its limited documented contract.

## Repair and next action

PROPOSAL.md specifies a separate adapter that consumes independently expected immutable scope/source pins, maps current request tables, binds source/test receipt closure and distinguishes source presence from release-stage acceptance. Keep the strict checker and its tests. Do not resurrect old six-fix scope or make all unfinished rows mandatory. Preserve #119's deferral, 26.7's decline, undefined M5 and aspiration. No policy file can revive them without applicable newer owner authority; M5 must not be defined or re-asked.

The parent/current integration owner should select the applicable already-authorized release-stage obligations from the current canonical ROADMAP and assign a bounded adapter implementation. Then the affected package/preview owners and sole publisher adopt and prove their actual boundaries. This stage supplies an independently reviewed proposal and red fixture, not an implemented fix or adoption approval. No owner question is needed to consume this evidence.

There is no supported fresh Work/Codex thread-creation or parent-thread-send tool in this thread's exposed registry. Local collaboration supplied independent review only; it is not a fresh Work-thread successor. Return the exact artifacts through this delegation's parent handoff. Parent consumption and later integration are PENDING until a real scoped action on these inputs is observed. No successor, uninterrupted continuation or release acceptance is claimed.

## Delivery boundary

Source evidence branch creation was attempted once at `wip/m12-acceptance-20261114` in the canonical repository from the exact assigned source. Automatic approval review denied `github_create_branch`: "Creating a remote GitHub branch is an external repository mutation/commitment, but the user did not explicitly authorize this branch creation or destination." The branch was read back as 404 after the denial. Latest trusted user history and branch-authority search supplied no new authorization for this exact operation; no retry or alternate write route was used. Branch/HEAD delivery: NOT PUSHED; PR: none. Existing local checkout and shared source remain unchanged.

The denied optional remote evidence delivery does not invalidate the completed investigation, reviewed proposal or fixtures. Local files and a source-evidence archive supply the parent handoff. Explicit authorization for this exact branch/destination is needed only if remote repository delivery is still wanted. No user action is required to read or consume the local findings. See STOP-REASON.json for the denied action, actual reason and recovery condition.
