# Independent M12 artifact review

Disposition: ACCEPTED for isolated proposal/regression handoff only. No unresolved material defect in the reviewed proposal or probes. This is local independent review, not firmware qualification, accepted release-policy selection, adopted gate, live uploader enforcement or a fresh Work/Codex successor.

Reviewed final artifact SHA-256 identities:

| Artifact | SHA-256 |
|---|---|
| PROPOSAL.md | `3abc79124fbe96499b95a818d87c5a3edae5a81082633efeaa67339236b148a1` |
| test_m12_boundaries.py | `a3d0d77a4ecf500c1245dffeea3d8c7e19c4d627a1e5cdbbe406e40ceceb61f4` |
| evidence/boundary-results.json | `c29b850d360617739afdb180d994eb47878772f02782527f89461af5656c070a` |
| evidence/input-manifest.json | `cfd1aef0fcb9b75fe6bd19e7a6cdb0c40f924ecc5e29aff378865932a006fb7f` |

Independently recomputed Git blob identity and SHA-256 for all 33 materialized source inputs: PASS, including later-added candidate tests/run_tests.sh, tools/publish_pages.sh and docs/rules/local-proof.md. Complete stored recursive trees (truncated=false) show the historical module, focused suites, source map and selection manifest present in #822 but absent from the assigned combined tree and canonical ROADMAP tree. Current package/preview source contains no completeness hook. tests/run_tests.sh:387 registers only the structural self-test, not selected-release acceptance. This verifies scoped source identity and adoption absence, not full checkout closure or the separate live publisher.

The added shell source distinguishes its --nightly delegation to the Python preview tool from its retained legacy non-nightly path, which contains an unguarded force push and Pages POST. Historical #822 retired that path; the assigned combined tree did not adopt that retirement. This is read-only adoption evidence only: the shell was not executed, force pushing remains prohibited, and any correction belongs to the existing publisher owner. REPORT.md's acceptance/ownership claims were inspected with no material overclaim found. These added source reads did not invalidate the unchanged boundary probe or require another test run.

Independent disposable fixtures reproduced strict mode returning exit 1 for a required UNPROVEN row, then exit 0 after removing that row's JSON and shrinking REQUIRED_ROWS.txt, with the same source digest and claimed candidate. Replacing the remaining feature source with a removal comment still returned exit 0. The fixture contains no Git repository. These are gaps if structural strict PASS is used as release acceptance, not evidence that a real release passed or that structural reporting violated its own contract.

Executed the final supplied probe independently. Equivalent reproducible command (the review used a disposable temporary directory for the report):

```sh
python3 test_m12_boundaries.py --checker inputs/candidate/tests/host/roadmap_evidence_check.sh --historical-gate inputs/historical/tools/roadmap_completeness.py --roadmap inputs/roadmap/docs/ROADMAP.md --report /tmp/independent-m12-report.json
```

Probe exit 1 as intended: structural positive and missing-JSON control PASS; shrunk inventory, changed source bytes and self-consistent wrong ROADMAP pins expose three missing acceptance boundaries. The independently generated JSON exactly equals boundary-results.json at the hash above. Historical parser rejects canonical ROADMAP with `ROADMAP must contain both the historical Done and complete Master tracker tables`. Its rejection is a compatibility result, not current release failure counting.

The proposal correctly retains existing structural/v2 coverage, supplies independent scope and source identity before consuming evidence, separates source/behavior/native/device results, and leaves package/preview/publisher adoption with their owners. Requested wording clarification was applied: a release policy cannot revive deferred #119 or declined 26.7 without newer applicable owner authority, and must not define or re-ask M5. Aspiration does not become an automatic release obligation. No shared source, ROADMAP, firmware, package, website, GitHub or device mutation was performed by this reviewer.

Next owner: parent/current integration owner selects an independently accepted applicable release/stage policy, then assigns the bounded adapter implementation and current-layout/negative coverage. The supplied fixture is deliberately red; no green repaired implementation or consumer adoption is claimed. Full historical suites were recovered/executed by the producer; this review independently checked artifact identities and boundary behavior, not those suite runs again.
