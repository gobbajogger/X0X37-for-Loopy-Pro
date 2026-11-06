# Unselected combined integration proposal, revision 2

The isolated complete source and patch reconstruct tree
`4ebbabf6e6c13a51555c616935b80ec29d4ccab6`.
There are 5,054 tracked leaf objects and 149 changed paths relative to PR698
`48e92b76c2f476cf40771e00a04e1838136bb55d` (base tree
`00370c8cf385577395f616e9df3cbc45a0d23041`).
State: **UNSELECTED / NOT COMMITTED / NOT PUSHED**. No shared branch, ledger,
review assignment, merge, firmware publication or device was changed.

## Inputs and reuse

1. Prior four-lane proposal `499169d1eb12b7b8a90090e474cb9b391b3f8f66`:
   base above, PR850 `c055f15c350aa259a24e63db6df4bc0bc063415b`,
   PR854 `8a3820019257c1f4428f11945fa7f9c30b8ae984`,
   PR860 `c97587c8cea40a8f6e534ec13d9690feb555486d`,
   PR861 `ae42441ac352d942f211907289b6df3a795803ee`.
2. Independently reviewed NIGHTLY-version revision 2 tree
   `9fa2d0f8129f8b330d53e903799af67c23ea5e9f`. Its seven-path patch is pinned
   to evidence commit `a2462be9886308e0108c2f07b7485804534ff136`, path
   `evidence/nightly-version-proposal-20261106/revision2/integration.patch`,
   Git blob `4a63c44bdcc424e92e7809eb19ca1972e83f89e3`.
   Existing QA continuation 6678022112 resolves its P1/P2 findings; no new review
   round or independent approval is claimed by this integration lane.
3. PR850 ROADMAP proposal tree `0020995601b23b6631c23f98b78496bd3df6fbbf`,
   based on c055. Its 61-path patch is pinned to evidence commit
   `ba46268554809207f4128be5e8ebf3990ce63287`, path
   `evidence/pr850-composition-proposal-20261106/composition.patch`,
   Git blob `a307fc2ae09b7e29f93c72440e41030aa289d9e8`.
   Existing PR850 producer/assigned-QA selection remains pending; no acceptance
   is inferred from this mechanical application.

Latest PR698 handoffs were reconciled before execution. The live PR698 head
remained 48e92b76. Prior four-root source reconstruction/package checks and
BUILD_ONLY receipts are retained for their exact original inputs, not relabeled
as versioned combined-tree results. The prior 11 scheduler checks and two killed
dependency mutants are reused because the entire extracted scheduler-function
region is byte-identical (SHA-256
`1e5ad257cd94b8cd9578f62c3fda80066f2581fd953230eeaad3066cf785daaf`).
The accepted #843 segment-log and #860 actual-render evidence remain scoped;
#860 design provenance is still open. Preserve #845/#860 duplicate handling,
#858 already-in-base disposition, and the full 18-lane accounting.

## Mechanical composition and exact verification

The full base was obtained from the GitHub API archive endpoint, with selected
donor paths from the three source-lane API archives. Every materialized tracked
file/symlink was checked against its authenticated Git blob and mode. This is
now a complete verified source tree, not only the earlier metadata proposal.
No network Git transport was used.

Both supplied proposal trees were independently reproduced from their patches
before combination. The four-root source was reconstructed and verified exactly.
The version and ROADMAP patches then both passed `git apply --check
--whitespace=error` and applied directly, with no edited production hunk or fuzz
resolution. The version patch's two runner additions have distinct context. The
ROADMAP patch intentionally replaces the original three PR850 paths and adds
58 paths; it does not change the runner. No additional source lane was selected.

Combined runner Git blob: `aa61919719073fc8265e2f98224efad0edb2695e`.
It has **245 unique literal column-zero registrations**: all prior 243 retain
their exact commands, plus `nightly-version` and `nightly-package`.
Both G-class arms, part-OFF `needs_gen()` membership and
`PART_OFF_REQUIRE_BASE=1 PART_OFF_BASE_REF=a06e20189141ff5fb8766e2c6152be460f4356c3`
are retained. Dynamic/conditional registrations are not included in this count.
The allocator, allocation ledger and schema-2 DEV verifier remain byte-identical
to PR698 base. No firmware/product binary path occurs in the patch.

`combined.patch` contains the complete 149-path net delta against PR698 base.
SHA-256: `7a8d2240a356836d1990230edbe7be067502ce7908cfa361c294bfbb44921a16`;
571,487 bytes. A separately constructed complete base index reproduced the base
tree, accepted this patch with whitespace checking, and wrote the exact combined
tree. No commit or ref was created. `expected-tree.json` supplies every expected
mode/type/blob; `integration-verification.json` records the machine checks.
After tests, `git diff --exit-code` passed against the candidate index.

## Executed combined-input checks

Command, from candidate root, with the verified historical ROADMAP fixture at
`ROADMAP.pinned.md`:

```sh
X0X_TEST_JOBS=1 \
X0X_TESTS='nightly-version nightly-package dev-validate-composed roadmap-evidence-check-selftest' \
sh tests/run_tests.sh
```

Exit 0. All four selected checks PASS:

- `dev-validate-composed`: 13 tests; actual synthetic Elite/Pro package CLI
  invocations, version argument propagation and refusal cases included.
- `nightly-version`: six tests.
- `nightly-package`: both synthetic models PASS, including composition,
  version/admission/model/CRC/ABI/output-preservation cases. No executable
  firmware or real build outputs were used.
- `roadmap-evidence-check-selftest`: 67 PASS / 0 FAIL; real pinned-source and
  delete-#8-from-both-inventory-and-record cases executed, no self-test skip.

The runner also prints an unrelated SMK-37 Pro native-package skip notice for
missing build/SDK inputs. This is retained in its log. Do not call this a full
suite or zero-skip native qualification. Native builds in composed orchestration
fixtures remain simulated. No unchanged full host/native suite was repeated.

Actual product evidence remains negative:

```sh
sh tests/host/roadmap_evidence_check.sh .
# exit 0: structural report, 2 PRESENT_OK and 201 UNPROVEN, 203 rows
sh tests/host/roadmap_evidence_check.sh --strict \
  --candidate-sha 48e92b76c2f476cf40771e00a04e1838136bb55d \
  --roadmap ROADMAP.pinned.md .
# exit 1: 203 failing rows, 201 UNPROVEN plus #66/#68 source_sha_mismatch
sh tests/host/roadmap_evidence_check.sh --strict \
  --candidate-sha 48e92b76c2f476cf40771e00a04e1838136bb55d .
# exit 2: current docs/ROADMAP.md is not the trusted pinned blob
```

The diagnostic SHA is the published base, because this proposal has no commit;
it does not assert a committed combined candidate. Trusted ROADMAP blob is
`a204a54c989d6f323d619e70f799427603756541` at
`5f578fc0642a1200a5163802bcd69277c511bfc7`. The fixture is an untracked test input,
excluded from the proposed tree. Report mode does not accept `--roadmap`; an
initial invalid report invocation exited 2 and is retained separately. Strict
mode's pinned-source check and ordinary structural reporting are distinct.
An initial working-directory typo prevented the first runner launch; no test
executed in that attempt. Corrected invocation above produced the retained PASS.

## Reproduction and next owner action

On an isolated complete checkout at PR698 base:

```sh
git apply --check --whitespace=error /path/to/combined.patch
git apply /path/to/combined.patch
# Stage only the patch paths in the isolated candidate, preserving modes.
# The complete index tree must equal 4ebbabf6e6c13a51555c616935b80ec29d4ccab6.
```

Use the unchanged original a06 baseline for actual part-OFF reference tests;
it must exist as a complete local object/source closure. Follow the version
proposal's explicit `composition.nightly_version` plan requirement and provide
fresh versioned artifact pins. Never reuse the unversioned BUILD_ONLY package
hashes as 0.0.25 artifacts. Changes to build/stamping/package inputs require
affected real Elite/Pro/controller build/package/resource checks with the
existing Build and assigned QA owners.

The existing sole integrator still decides final source/all-lane selection.
ROADMAP product proof (201 UNPROVEN and historical mismatches), PR850 adoption/QA,
UI design provenance, missing selected lanes, invalidated combined tests,
native/versioned-package/installer/data/model/license/hosting and served-byte
gates remain open. No test or metadata repair manufactures that evidence.
This deliverable does not authorize or perform merge, publication or devices.

Current evidence links:
- https://github.com/gobbajogger/X0X37-for-Loopy-Pro/pull/698#issuecomment-6678022112
- https://github.com/gobbajogger/X0X37-for-Loopy-Pro/pull/850#issuecomment-6677301507
- https://github.com/gobbajogger/X0X37-for-Loopy-Pro/pull/698#issuecomment-6677645083
- https://github.com/gobbajogger/X0X37-for-Loopy-Pro/pull/698#issuecomment-6677323359
