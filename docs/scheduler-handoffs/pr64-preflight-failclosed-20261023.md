# Hourly Build native probe: fail-closed prerequisite regression (2026-10-23)

**Scope:** Producer-owned draft PR #64 source and unit tests only. The tested inputs were exactly two Git blob-identical files from PR #64 @ `bf96bbc631d2029ac7541707687addda19e70654`, tree `d66d64bf56af5b334e752805cec387ab350e8062`. This was **not** a complete native firmware checkout or firmware build.

## Independently reproducible defect

In previous PR #64 code, when any one of `locked-settings`, `slot-backup` or `installer-slots` returned a failing exit, the probe still invoked **both Elite and Pro firmware build stages and the native controller stage** and copied generated output packages. Its overall `native_probe_pass` was correctly false, but failed prerequisites did not prevent binary generation. This is an unsafe, wasteful preflight bypass; copied mock files in the negative regression are *not* firmware or installation evidence.

**Remedy:** Collect failed prerequisites and skip both model builds and controller compilation/copy if any preflight exit is nonzero. Mark each model `NOT RUN — prerequisite failed`, leave `controller:null`, record `blocked_by` and retain `native_probe_pass:false`. Cleanup and symlink fail-closed repairs from the prior commit remain intact.

## Executed Linux x86-64 checks (source-only, disposable Git worktrees and mocked target stages)

- Existing exact two-blob baseline: `python3 -m unittest discover -s tests/host -p 'test_native_nightly_build_probe.py' -v` — **8/8 PASS; exit 0**; log SHA-256 `d90abc6772b134ba3a2e8cac48c41153d816a6682cd53d3ae65e2f3a27ffc549`.
- New test against *unchanged* `tools/native_nightly_build_probe.py` blob `b4c5454d3f50f084ba1720c4b3ec7d3934eed776`: same unittest command **exit 1**, with **3 expected FAIL subtests**, one per failed prerequisite. Each found an exported mock Elite artifact when no build should have been attempted. Log SHA-256 `278ad58bc2e22aeec498fa288913a26a876f7786fda10f63473f3890fad3f711`.
- New test against amended source blob `c2b3a3c8f209f8720ffd2e02058c1a31a010badd` and test blob `3796ae74c24a4175917e67e00aa46188057d9ed8`: **9/9 PASS, exit 0**, including all three prerequisite cases. Log SHA-256 `fcd56a60998238532bc2e3f227463fcc164549e5fc0b667bc78649ea4163d36c`.
- `python3 -W error -m unittest discover -s tests/host -p 'test_native_nightly_build_probe.py' -v`: **9/9 PASS, exit 0**. Log SHA-256 `38371c60e9ec3f5afb2f57feb9ef33eaa0614f3818df0105d1ee8997580c9027`.
- `python3 -m py_compile tools/native_nightly_build_probe.py tests/host/test_native_nightly_build_probe.py` — **exit 0**.
- Source SHA-256 after repair: `14559a3fa9d81650921bf7abf6535f0450a0e12539b0d65311dc33b85f7c255e` (tool); `bafc35abcc7ae8953ad0d617ec7c1e5ab635eeea05c2257b80fe47fa431e2e90` (unit test).

The native test fixture uses real temporary Git init/commit/worktree/cleanup but **mocks all firmware/SDK validation stages and mock binaries**. Passing its nine unit cases does **not** certify any .fwsc or ctl.img, native SDK, DSP references, installer safety, hardware or physical output.

## Current source-transfer and publication limitations

The live firmware `nightly` ref was independently retrieved via the Git data API at `58b5b45a2dd90581de7a674c331ca5cbe8a67c7e` after policy-only PR #60; new integrated firmware drafts (#70/#71) remain unmerged. `/mnt/data` contained no full current source .git worktree; local `git ls-remote https://github.com/gobbajogger/X0X37-for-Loopy-Pro.git refs/heads/nightly` exited **128**, `Could not resolve host: github.com`. This blocks *exact-current full-tree native builds*, not this two-file regression repair. The historic source/build hashes cannot be relabeled current. Native JieLi 4.0.1/AC79 V1.2.1 prior attestations are historical; their inputs were **not remounted or used** for this scoped run.

Elite: **BUILD PENDING / DEVICE NOT TESTED / NO INSTALLABLE**. Pro: **BUILD PENDING / DEVICE NOT TESTED / NO INSTALLABLE**. No new firmware/controller artifacts or SHA/CRC/budgets. Firmware numeric release version: **VERSION UNASSIGNED**. NIGHTLY web picker still has empty `versions:[]`; no HTTP verified packages.

## Consumer

**Independent QA:** retrieve the *new* exact PR #64 HEAD and Git blobs; independently rerun the three injected fail-before/pass-after scenarios and original cleanup and symlink tests, and confirm that failed preflight produces no firmware, controller, or misleading success record. **Safe Merge:** handle only reviewed integration by normal PR. **Catalog Sync:** historical source-only WIP/NO-GO record, not an installable candidate. No hardware flashing, public binary hosting, main/nightly direct writes or Pages publication.
