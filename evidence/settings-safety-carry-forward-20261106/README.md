# Settings and project safety carry-forward

Isolated patch evidence packet. Source integration is not selected. No shared source branch write, merge, firmware publication or device access. This packet is published under the later explicit handoff authorization.

Exact proposal tree: `2e38c67d3dbf1b905085580229e395433aefd92d`. Candidate tree: `d93c60a7b21bb2cfa92ac40bc43cfbe470b5073d`. Apply `settings-safety.patch` to the proposal; fresh-index patch reconstruction matches that candidate. The checkout is detached at PR698 48e92b76c2f476cf40771e00a04e1838136bb55d with proposal reconstruction staged and only this safety patch unstaged. Do not export its entire HEAD diff as this lane's patch.

Donor: PR810 `45a7285e536d8c24e0c57bf512393c804a6aad5c` (project blob `82cc9eba89884e4310b3273941fb9b6335611415`), incorporating PR790 `314614c011c3b8bb119f6d298c558288f99aed11` and production settings fix `16ba9e239d116f99c5ed9428058ce9264fc45c69`. Read PR810 current handoffs, including independently accepted donor closure in issuecomment-6550858606. That review is donor evidence, not independent acceptance of this patch.

Missing protections restored: complete-save fence after failed live load; distinct erased/read-error/damaged/busy storage outcomes; damaged-project lock; per-object older A/B stream combinations with cache invalidation after recovery. Proposal already had read_settings_payload and pending Bluetooth flush: those were preserved, not duplicated. Preserved M7a live/staged OFF, extended settings tail and export word 28. Added tail/live/staged/export assertions to the reused retry cases.

## Actual checks

GCC 14.2.0, Linux, CC=gcc. Commands ran from /workspace/settings-safety.

| Command | Result | Log |
|---|---|---|
| sh tests/host/run_project_save_preflight.sh | PASS exit 0: full-memory refusal, reboot retention, short/exact settings and collision | preflight.log |
| sh tests/host/run_project_stage.sh | PASS exit 0: 26,600 checks | stage.log |
| SAN_FLAGS='-fsanitize=address,undefined -fno-omit-frame-pointer' sh tests/host/run_project_stage.sh | PASS exit 0: 26,600 checks, final restored candidate | stage-san-final.log |
| sh tests/host/project_runtime_test.sh | PASS exit 0: UI/project and SMC runtime | runtime.log |
| python3 tests/host/test_smc_boot_saved_roles.py | PASS exit 0: 28 checks each plain/UBSan | boot-roles.log |
| PART_OFF_BASE_REF=a06e20189141ff5fb8766e2c6152be460f4356c3 PART_OFF_REQUIRE_BASE=1 sh tests/host/run_part_off.sh | PASS exit 0: runtime, all-ON bit identity, cross-firmware storage/files and roundtrips | part-off.log |
| Remove full-save fence, run project-stage | EXPECTED FAIL exit 1: 31 failures | save-fence-negative.log |
| Disable A/B combination loop, run project-stage | EXPECTED FAIL exit 1: 10 failures | recovery-negative.log |
| sh tests/host/run_project_303_stream.sh | FAIL exit 1 on candidate AND untouched proposal: obsolete format-2 migration fixture calls removed helpers | stream.log / stream-baseline.log |
| git diff --check; fresh-index patch reconstruction | PASS | identity.json |

Initial clang invocation was NOT_RUN (clang absent); GCC was available. Initial sanitized linkage failed on missing plat_ms in the storage fixture because instrumentation retains an engine path. Added a fail-on-call clock stub: it is never called by passing storage tests. The final isolated sanitizer execution is stage-san-final.log. Negative controls restored source before final verification.

## Conflicts and receiving action

Five project.c hunks required manual composition around existing read helper, moved state, M7a tail and OFF adoption. Seven original donor paths plus the existing SMC boot-role fixture are changed (8 total, 425 insertions / 34 deletions). The fixture extraction now accepts the reconciled declaration and models the early read-failure path; no SMC implementation changes. The legacy migration test is left failing, not weakened or used to reintroduce pre-rev13 migration.

Sequencer and SMC owners retain their implementations. Shared interfaces: plat.h previous-copy selector, storage return values, project.c load/save/adoption boundaries, and SMC boot fixture. Union these hunks with other lanes; do not transplant whole project.c or overwrite OFF state/export. No tests/run_tests.sh changes. No new UI warning painter is included; existing damaged-state query returns 2 as donor does.

Parent integrator next: consume patch and identity, compose with sequencer/SMC lanes, obtain independent exact-combined-tree QA. Native Elite/Pro SDK builds, full host suite, UI warning presentation, package/resources and devices are NOT_RUN for this source-only lane; no release qualification is claimed.
