# Port composition and mapped SMC popup to all-in-one

Task C plan, 2026-10-30. Owner-approved isolated development only.

## Pins and ownership
- Source handoff: `lane/mb-aio-port-650-687` at `ae0a62e3774545d4dc7544d481b0d9da10b26055`; keep FMB's work intact and do not write to that lane.
- Requested integration baseline: `integration/all-in-one` at `ea859a8639f7448fb53196f6feb8c8c761b114d9`.
- Observed live integration head at intake: `c54d9f9b4262709bb4ea5852ae4ed008fa100996` (advanced since request).
- Current firmware wip pointer: `wip/one-build` at `401ba15c468b52fd3b26958e0fe4bf335fbda0b5`.
- Isolated writer: `chat/port-ui-popup-and-break`, initially branched from the exact FMB handoff SHA.
- Target: a draft PR into `integration/all-in-one` for review by the assigned integrator, never a direct merge.
- Prior PR sources: #650, #651, #638, plus the SMC UI mapped-page peek/hold/restore stage-6 source on `cursor/smc-mixer-ui-popup-stage6-blocked-8e03`. #687 BLE work is FMB's existing handoff; preserve it. Miami is owned elsewhere.

## Boundaries
No `.github/`, GitHub Actions, zipball source transport, device flash, `main`/`nightly` updates, force-push, new engine, bass303 engine, or `wobble.c` reintroduction. Preserve Chicago/Miami sound and rev-13-only persistence. Do not remove owner-approved private header source from existing branches. Never push the vendor SDK, toolchain, tokens or firmware/product binaries.

## Implementation and evidence
1. Fetch exact GitHub API trees/blobs and compare changed-file SHA identities against #650, #651, #638, the popup branch, FMB handoff, and current integration. Reuse anything already present. Inspect repository rules before editing.
2. Port only still-missing, valid behavior, keeping newer mixer/FMB/engine changes and the current shared UI. Resolve conflicts explicitly. Popup must show the actual mapped mixer/FX screen, hold about 1200 ms from the latest control movement, restore previous view/page/selection, cancel safely on navigation and honour overlay/clip/performance guards.
3. For any changed UI, create and review a clearly labeled before/after screen mockup before implementing, then capture the resulting UI test evidence. Place preview source/assets alongside this plan without binaries or restricted SDK bytes.
4. Run the scoped SMC popup, #650 material UI, #651 strict host linkage, #638 adaptive BREAK/admission checks, then the entire `tests/run_tests.sh` with counts of PASS/FAIL/NOT_RUN and unmodified failure logs.
5. Build both SMK-37 Elite and Pro from an exact-source checkout using pinned JieLi and SDK V1.2.1, verify exit codes, model identity, code/RAM/pool limits, and keep firmware binaries outside Git.
6. Commit each coherent source/check/evidence stage via the authenticated GitHub API default author, verify remote branch SHA after writes, and open one draft PR into `integration/all-in-one`, documenting requested-vs-live base and every measured PASS/FAIL/NOT_RUN.

## Completion criteria
PR has only approved changes, actual readable mockups and logs, a verified remote HEAD, and exact test/build results. A failed or unrun required check is reported as such and prevents a false green or merge-ready claim. No hardware or publication claims.
