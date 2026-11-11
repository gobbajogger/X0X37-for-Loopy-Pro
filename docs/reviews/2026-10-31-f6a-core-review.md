# F6a core code review

Scope: six frozen web source/test files against bb1907494b8d1b52d52ac220dfeb9a0663ea3ee2. Mode: agent report-only. Source pins are in the run receipt. No source edits or commits were made by reviewers.

The change preserves all 512 archived sample-header bytes on Samples-page restore and restricts project imports, recovery, rollback and legacy adoption to the revision-13 policy. The install helper still defaults to 480 header bytes.

Reviewers: correctness; project standards for AGENTS.md requirements; testing for changed regressions; security for file input; reliability for recovery/error paths; adversarial for mutation sequences and mock fidelity. All six completed. The separate font proposal review is outside this source receipt.

## P2 -- Moderate

**#1: Test rejection of a legacy rollback source.** `web/x0x_projects.js:472`; reviewer testing; confidence75; independent validator confirmed.

A valid revision-13 import must be refused when the keyboard's live project is historical because that snapshot cannot serve as a loadable rollback. Existing tests pair historical live projects with historical incoming bundles and reject at the earlier input check. Add a valid revision-13 incoming bundle with a historical live project. Assert version rejection, no project/sample mutations, unchanged bytes, and idle cleanup after read-only export.

## Requirements completeness

- Task1: sample restoration is implemented and has focused synthetic readback proof, including reserved header bytes and empty slots.
- Task2: revision-13 import policy is implemented. Finding1 covers the missing live rollback-source regression.
- Task3: parent continues font/site acceptance, final checks and open PR delivery. This intentional ongoing work is outside the frozen core receipt.
- Row55: original historical owner archive and matching-device readback remain unavailable. Synthetic tests cannot prove its completeness.

## Actionable Findings

#1 is a localized test addition, routed to downstream-resolver, with targeted verification required. Also add the separate apply guard regression by replacing prepared.backupBytes with a valid legacy portable bundle and asserting rejection before mutation (`web/x0x_projects.js:502`).

## Coverage

Six independent local personas completed; no production defect found. One validator batch confirmed finding1. No findings were suppressed, dropped or malformed. No review failed. Cross-model review did not run because this task prohibited external messages; the local adversarial reviewer covered that lens. AGENTS.md was the applicable fallback standard.

Inspected evidence: focused closure69 pass/0 fail/0 skip; Samples page mocked-device green log; three existing host registrations; firmware header-length acceptance; diff whitespace check; six matching SHA-256 source pins during review. Reviewers did not rerun tests or builds. Full host/native/browser acceptance remains separately owned by the parent.

Untracked docs/superpowers/ was excluded from the diff and used only as explicit plan context. Font proposal and forthcoming CSS/font/site-generation/registration edits require a follow-up review. Any parent fixes after persona completion are not covered by these original pins.

---

## Verdict

Ready with fixes for the six-file core scope. Add and verify finding1 before final review. This receipt does not establish whole-task acceptance, merge readiness, historical archive completeness or permission to publish.
