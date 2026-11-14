# SMC retained capture bounds repair

State: reviewed source and affected capture PASS; pending isolated Git delivery.
This report belongs to the repair stage, not qualifier 6af8078c-4fd8-8191-bf80-8b75dce4c0e1.

## Inputs and limits

Consumed exact GitHub API archive 94197fc839b00aff47b98ca02f3c6396e4a6eabf,
root tree 72d28fbc741244d18fa3d18495da16ab0fcb1d24. Original fixture Git blob:
60bca638fd7ccd8ef51739c1d35d9b190f0aae09. API blob identity checks matched
capture driver, shared UI harness/runner, pattern.h and ui.c before execution.
Tracked archive comparison after the fix identifies only the retained fixture
as changed; firmware sources are byte-identical to this base.

NOT CONSUMED: qualifier REPORT.md, NEXT_STAGE.json, and
/workspace/f4-qualification/f4-host-qualification-evidence.zip. The directory
is absent here, exact-title Library search returned no matching packet, visible
history search returned no qualifier messages, and supported thread reader/create
capabilities are not exposed. The creator's preservation worker retains original
packet ownership. Nothing was deleted or replaced. Prior five selectors, 302
assertions, 16 inspected renders and 5,345 verified blobs are creator-reported
history, not newly verified qualifier results. This repair uses the explicitly
authorized independent source-inspection fallback.

## Repair and sensitivity

NPAT is 32. capture_scenes() used invalid 63/62 indices for last-pattern/cue
scenes and wrote proj.pat[63]. Replace them with NPAT-1 / NPAT-2. Preserve
64-step bass length, page 3 and all input/render sequences. No production code,
DAW/USER compatibility, one-peer policy or held-FX behavior changed.

New standalone runner tests/host/smc_remainder_capture_bounds_test.sh executes
the real retained capture driver through the unchanged shared harness, adding
-fsanitize=bounds -fno-sanitize-recover=bounds. Invoke it with sh; no shared
runner/selector registry changes. This regression is intentionally independent
of the previously passing assertion main, which does not exercise this write.

Checks executed with GCC 14.2.0 on Linux:

- RED, unchanged fixture plus new runner: exit 1; exact diagnostic at line 478:
  index 63 out of bounds for type 'pattern_t [32]'. Raw RED.log retained.
- GREEN, repaired fixture plus identical runner: exit 0, 57 actual PNG captures,
  no bounds diagnostic. Raw GREEN.log retained. Existing compiler warnings occur
  in both logs and are not represented as clean compilation.
- Shell syntax: sh -n tests/host/smc_remainder_capture_bounds_test.sh, exit 0.
- Four affected actual frames inspected at 240x240: last pattern 32, cue 31,
  steps 49–64, CHN indicator. focused-frames.json records their hashes.
- Independent read-only review APPROVED: archive diff and runner inspected;
  built instrumented driver separately executed, exit 0 / 57 frames; range and
  chain actual frames inspected. See REVIEW.md for exact reviewed identities.

The full F4/firmware suite, previous supplementary assertions, native firmware,
SDK, emulator, controller, device, install and publication checks were NOT_RUN.
They are outside this fixture-only repair. No release qualification is claimed.

## Changed source identities

- tests/host/smc_remainder_test.c: Git blob ce62193ddb7cb9bc0a82511e7eb6ff1c683c9c4d;
  SHA-256 40c88c2e4b266bd9651e9d4f592f96b0e386f2ff31bc4ac1c686b74345dd2e94.
- tests/host/smc_remainder_capture_bounds_test.sh: Git blob b9e7493f861cdba97432621272aab43c0fc24cd0;
  SHA-256 469006df5e84ad6e17a7d2b05a685040dcef61c78595d21fd55b3e5fa713861f.

## Ownership and next action

Owned branch chat/smc-capture-bounds-20261114, direct child of the stated base.
No matching branch/PR or competing fixture repair was found in current scoped
GitHub searches. The coordination/merge-queue ref returned 404; no complete
cross-thread worker inventory is claimed. Existing Firmware Main Builder / #869
retains all integration authority; #887 remains the sole roadmap writer and
#891 retains firmware UX ownership. No incumbent branch is modified.

After verified isolated delivery, creator / incumbent integrator consumes the
exact PR diff and repair receipt alongside the preserved qualifier packet.
Any #40 no-User wording decision remains with the owner. No merge, release,
publication, device action or new SMC audit follows from this repair.
No supported thread_management.create_thread/read_thread exists here; return
structured completion to the creator instead of claiming successor admission.
