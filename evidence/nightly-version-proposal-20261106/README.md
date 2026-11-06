# NIGHTLY version integration proposal — LX-REQ-016 / LX-REQ-023

Completed isolated source/tooling preparation and scoped tests. UNSELECTED, NOT COMMITTED, NOT PUSHED. No new PR. Existing sole PR698 integrator and assigned QA retain selection/review ownership.

## Authenticated inputs

- PR698 / wip/one-build: 48e92b76c2f476cf40771e00a04e1838136bb55d; base tree 00370c8cf385577395f616e9df3cbc45a0d23041. Remote head rechecked unchanged at completion.
- Preserved rc1 donor: c32c54ac8ec68aaca53adccf7af66f155590f2dc; tree ac490f7fae9198748aa54bd914d42f99d45444f9.
- Maintained nightly policy: 66073c68dd80d51a7053432c2c161a8b720a1341. Main AGENTS read at 1a6bc3fe0245277b5ae31ab554c73d74b9ab4d83.
- Full base archive obtained via GitHub API, not network Git. All 4935 blobs verified against the authenticated recursive tree; complete base root independently reconstructed. Relevant donor blobs verified exactly (see donor-tree.json).
- Historical #806 QA: https://github.com/gobbajogger/X0X37-for-Loopy-Pro/pull/806#issuecomment-6550284001 accepts scoped b46fff14 version/discovery work but records an inherited USB host failure. That acceptance is NOT transferred to this proposal. Latest PR698 handoffs 6677634925 and 6677645083 preserve version-route reconciliation and BUILD_ONLY limits.

## Proposed changes and effect

Five paths only: tools/build.py, tools/package_nightly.py, tests/nightly_package_test.py, tests/host/test_nightly_version.py, tests/run_tests.sh.

The build gains --nightly-version X.Y.Z with a strict full Git SHA input and application-only version defines; Elite/Pro loader/package identities remain model_900. The checker requires a CLI expected version, reads the decrypted application ABOUT line, rejects missing/ambiguous/noncanonical versions and stale DEV strings (ASCII and both UTF-16 byte orders), and matches header/ABOUT/expected versions when applicable. Existing model, integrity/CRC, layout, sample-map, controller pin/header/ABI and atomic output preservation checks remain in place.

Only version hunks were extracted from the donor build script. No DSP optimization, stock-loop, USB discovery, UI rendering or firmware source changes were imported. X0X_GIT_SHA is supplied as a diagnostic define; this patch does not add a displayed Git SHA or new USB names. Existing ABOUT and editor version consumers use X0X_VERSION.

Allocator and checked-in ledger are byte-identical to base (release_alloc.py blob 3926f1d409c8ae41e3d2e969d3a8a0e812505fc2; ledger blob 5573f98159237028142636d9c3499ac42b66c42b). Numeric 9XXYYZZ package identities still require an existing allocation. The model_900 route retains its existing exemption from that numeric-identity allocator, while now validating a real app version. This does not allocate 0.0.25 or select an authoritative release policy; the existing version owner must reconcile that route before delivery. Tests use temporary synthetic ledgers only.

## Executed evidence (Linux, Python 3.12.14)

- python3 tests/host/test_nightly_version.py: PASS, 6 tests, exit 0 (version-final.log).
- Deliberate SHA regex relaxation from exactly 40 to 4–40: real version regression FAIL, exit 1 as expected; restored source rerun PASS (negative-sha.log).
- python3 tests/nightly_package_test.py: PASS, Elite and Pro, exit 0 (package-final.log). Uses synthetic non-executable packages only. Includes 0.0.25 positive acceptance, wrong/malformed versions, missing/ambiguous ABOUT, stale DEV strings, USB-only non-evidence, allocated-header/ABOUT disagreement, unallocated numeric identity, wrong model/controller pins, corruption/layout/ABI and output preservation.
- python3 tests/host/test_release_alloc.py: PASS, 8 tests, exit 0 (allocator.log); allocation behavior unchanged.
- sh -n tests/run_tests.sh and base-relative git diff --cached --check: PASS.
- Apply integration.patch to a fresh index initialized with the complete base tree: PASS; reproduces the exact proposed tree below.
- Full host suite, native firmware/controller builds, actual versioned package bytes, UI/USB renders, installer and device checks: NOT_RUN in this bounded tooling lane. No build/release/install qualification claim.

## Handoff

Proposed tree: 1f02d81a8862166302ff673b3278be1a0fb7beb4
Patch SHA-256: 0b319e6f334d4fab52dd9f2947ad67bf0b4b00e68d8e51101e850f36a82fd733

The sole integrator can apply integration.patch to the pinned base, or reconcile these five paths with its eventual selected combined root. Preserve all concurrent runner registrations; this patch adds only nightly-version and nightly-package. The previously proposed 499169d1 four-root tree is not this proposal and has not been modified here. Assigned QA must assess the resulting selected root. After version-route reconciliation, Build must produce actual versioned outputs and rerun affected package checks; existing unversioned artifact hashes cannot become 0.0.25 by renaming.

No shared branch/ref or real release ledger mutation, merge, publication, scheduler or device operation occurred. Remaining downstream blockers: source selection, authoritative version policy/allocation route, independent exact-input QA and versioned native/package qualification, plus the existing ROADMAP/design/installer/rights/publication gates recorded by the parent.
