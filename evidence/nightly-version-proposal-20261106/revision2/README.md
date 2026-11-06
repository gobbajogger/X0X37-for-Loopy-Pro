# NIGHTLY version proposal: independent-review fixes

Proposed source tree: `9fa2d0f8129f8b330d53e903799af67c23ea5e9f`.
Previous reviewed proposal: `1f02d81a8862166302ff673b3278be1a0fb7beb4`.
Base: `48e92b76c2f476cf40771e00a04e1838136bb55d` / tree `00370c8cf385577395f616e9df3cbc45a0d23041`.
Original donor: `c32c54ac8ec68aaca53adccf7af66f155590f2dc` / tree `ac490f7fae9198748aa54bd914d42f99d45444f9`.

UNSELECTED / NOT COMMITTED / NOT PUSHED refers to the proposed source tree, not any evidence-only delivery. Existing sole integrator and assigned reviewer retain ownership. This update addresses the supplied P1 and P2 findings; it does not declare independent acceptance.

## Reproduced findings and fixes

P1 reproduced: the real package CLI exits 2 without its required --firmware-version. The original composed orchestration tests substituted a fake package script that accepted arbitrary arguments. The runner now requires a canonical composition.nightly_version in the plan, passes it to both package invocations, and supplies the same version and pinned full source commit to the actual build arguments. Missing/noncanonical versions fail before build work. Plans must explicitly provide the intended version and matching fresh artifact pins; old unversioned hashes are not reused or relabeled.

The DEV schema stays unchanged: its firmware_version describes the transport identity, still null for model_900. The NIGHTLY app version is bound by the fingerprinted composition plan and both actual package checks. This does not create or reset an allocation, expand numeric release identity admission, or claim release qualification. tools/dev_validate.py, tools/release_alloc.py and releases/openbeta/ledger.json are byte-identical to the original base.

P2 reproduced: the former restricted numeric regex accepted 0.0.25 alongside a malformed 0.0.24_openbeta ABOUT token by silently skipping the latter. The parser now captures every null-bounded ABOUT token before canonical validation, rejecting suffixes, empty tokens, non-ASCII and newline contamination, even when another valid line exists. Existing stale DEV, ambiguity, model/CRC/layout/controller/ABI and output-preservation checks remain.

Changes relative to previous proposal: only tools/dev_validate_composed.py, tools/package_nightly.py and their two regression tests. Full integration patch contains seven paths relative to PR698; review-fixes.patch contains only this four-path revision.

## Tests and evidence

- reproduction.log: original P1 actual CLI exit 2 and P2 invalid conflicting token accepted as 0.0.25.
- about-regression.log: five malformed conflicting tokens accepted by the original parser and refused by the fixed parser; single valid ABOUT still accepted.
- composed.log: 33 existing/imported orchestration and DEV validation tests PASS before the last added missing-argument assertions.
- composed-final.log: all 13 composed tests PASS on final source. The new test uses the real packer and actual generated package CLI for Elite/Pro compose and check, verifies explicit expected version, verifies missing flags exit 2 and wrong version exits 1. Fake native build/composer remain only in the preexisting orchestration tests; no actual native build is claimed.
- version.log: six version tests PASS.
- allocator.log: eight allocator tests PASS.
- Final full synthetic package regression: Elite and Pro PASS, exit 0 (package-final.log), including all new conflicting-token cases.
- Fresh-index integration.patch application reproduces the exact proposed tree. Base-relative whitespace and runner syntax checks PASS.

Synthetic package fixtures contain no executable firmware. Native firmware/controller build, actual package qualification, installer and device tests NOT_RUN. Full host-suite status is not asserted. No shared owner branch, ledger allocation/reset, merge, firmware publication or device operation.

Next: existing assigned reviewer checks this exact revised tree; sole integrator selects/reconciles it with the intended combined source. Build plans must supply composition.nightly_version and fresh matching versioned artifact hashes. Existing independent QA, version-route reconciliation and downstream delivery gates remain open.
