# Hourly Build — integrated-source preflight, 2026-10-23 (WIP / NO-GO)

**Role:** sole native Elite/Pro DEV firmware + matching controller builder. **Consumer:** Independent QA, sole Safe Merge integrator, NIGHTLY Catalog Sync. This is truthful **source-only / preflight** evidence, NOT firmware build completion, device proof, an installable candidate, or permission to merge/publish/flash.

## Exact source and channel

- Source repository: `gobbajogger/X0X37-for-Loopy-Pro`.
- Consumed Safe Merge integration ref: `lane/integration-ble-clock-q16-data-20261023-1838`, HEAD `93af3b53f19330086e38b99d08ef3d834a75061b`, complete Git tree identifier `daa5e6185d4e30331b3e578d5ec1cef5ad5bd5d0`.
- Immediate parent `8557b698156f4d5ac6e19d6ad7f58c4d296bcebe`. Connected GitHub read reports integration three commits ahead of firmware `nightly` at `c9718ae833eb64a5191c350922593df8a36017c9`; integration is NOT merged into firmware `nightly` or production `main`.
- Integrated source includes clock handover #48, high-rate sample slot #45, BLE mailbox #50, controller atomic writes #42/#49, generator output failure #51 and prior SMC/fastmath/host runner repairs. This records **Git source state**, NOT completed target tests. The separate browser/status NIGHTLY `main` was `7757a4b78cc5bcb415c794efe7f1e89dbe521a7e`, Pages `9c5d3b6b0846f89436492851fef89ed3829b7ad7` (read-only).
- Firmware semantic version: **VERSION UNASSIGNED**. Version-label proposal #53 at `253b5101a9e3ebcc1e8dcca48764322909820b26` is a separate draft, not in this exact integration. Neither a date-based build key nor the NIGHTLY browser npm `0.0.0-dev` is a firmware version.

## Native Linux execution — only tests genuinely executed in this run

- `pwd`: `/`; `uname -sm`: `Linux x86_64`. Host `gcc`, `clang`, `python3`, `node`, `git`, `unzip` and `sha256sum` resolved.
- Privately materialized `x0x37-private-build-inputs-20261021.zip`: 3,886,974 bytes; SHA-256 `2570c8f9472968afb16ea94372a05edda2e1f9c72d22b2431688c87d906c4805`. Kit `python3 kit.py verify`: **exit 0**, checksums and **all 11 pinned AC79NN_SDK_V1.2.1 input files** verified. This kit is a required *subset*, NOT a complete SDK tree.
- Independent **SDK kit fail-closed negative**: copy kit in disposable isolated directory, append byte `ff` to copied `sdk-pinned-inputs/apps/common/net/wifi_conf.c`, run `python3 kit.py verify`: **exit 1 (expected rejection)** with `Kit checksum mismatch: sdk-pinned-inputs/apps/common/net/wifi_conf.c`. Corrupted copy deleted. These are SDK integrity tests, **not firmware code or package regressions**.
- `git ls-remote https://github.com/gobbajogger/X0X37-for-Loopy-Pro.git refs/heads/lane/integration-ble-clock-q16-data-20261023-1838`: **exit 128**, `Could not resolve host: github.com`. The connected GitHub ref/tree/file API still works; shell DNS failure is a **narrow exact-checkout transport gap**.
- Library `files__materialize` attempts on historical source bootstrap and `jieli-linux-toolchains-20260730.1.zip` each refused `This Project file does not have an authorized raw-byte materialization path.` Refusal respected (no alternate raw-byte workaround). Target `pi32v2` compiler was not on PATH or mounted in this Linux runner. Past runs' Clang 4.0.1 verification is NOT a new toolchain execution.
- Four required pinned DSP source checkouts were NOT mounted/revalidated: 9W9 `10cbe5c1e1c9e65b0818bb34644ab1a13092082c`, 8W8 `94aa2716d4c2e5851fc7c5dbb4c17adfd133c997`, 303 `7680578cdecd91e512cfddb9c9eb07b287fb64f4`, TB3PO `f49d81d05d1e29ae37728a08e30f193892be3bc9`.

## Exact integrated Git-source inspection (NOT a native firmware test)

- Connected GitHub read the current `93af3b53...` source tree (1717 entries, untruncated) and matching exact file blobs:
  `firmware/src/ble/ble_midi_host.c` `f4415750533c0e43f1a6c45dcd3a54dac1af6605`,
  `tests/ble_mailbox_backpressure_test.c` `c24c65e4e651b2b12958fb37d7bcab7b76ed0199`,
  `tests/run_tests.sh` `5fb5b4b2227ab2d7989883d3afa82686edc33a7a`.
- Source-level verification: `cmd()` checks mailbox send result *before* setting `cmd_wait` (BLE C lines 54-64); `bleh_poll()` preserves queued MIDI on failed `att_send()` (lines 589-605). These are the **proposed BLE fix in this source**, not native proof of working hardware.
- **Unresolved source-level negative risk**: `att_err()` at lines 131-135 and ordinary GATT `att_in()` responses at 259-368 call `att_send()` without checking failed mailbox writes or storing a retryable response. Saturation can therefore drop an ATT reply; the integrated #50 handoff separately acknowledges ATT retries are not covered. Needs explicit mailbox-full ATT response negative and Independent QA before software installability. No executed native failing assertion is claimed for this finding.
- `tests/run_tests.sh` contains terminal worker result fail-closed guard at lines 350-366 and absent requested selector failure at 368-377; **inspected remote script source only**. These guards do NOT imply all tests have run.

## Required checks — status by target

| Gate | Elite DEV | Pro DEV |
| --- | --- | --- |
| Whole approved `93af...` exact local source, pinned compiler, complete SDK and four DSP checkouts | **BUILD PENDING** | **BUILD PENDING** |
| Actual native target build and matching controller `ctl.img` | **NOT RUN** | **NOT RUN** |
| `.fwsc` version/header/model, CRC32, SHA-256 and byte size | **NOT AVAILABLE** | **NOT AVAILABLE** |
| Code, RAM, pool, flash budgets | **NOT RUN** | **NOT RUN** |
| Full SMC/BLE/clock/USB/MIDI/808/909/seq/sanitizer/worker timeout negatives | **NOT RUN ON EXACT SOURCE** | **NOT RUN ON EXACT SOURCE** |
| Wrong-model/stale controller, sample backup/readback/restore/interrupt, installer negatives | **NOT RUN** | **NOT RUN** |
| Independent second QA and licensing/hosted immutable package byte validation | **PENDING** | **PENDING** |
| Owner hardware/device PASS | **DEVICE NOT TESTED** | **DEVICE NOT TESTED** |

**No new target artifacts exist from this run.** Therefore there are no attributable per-model `.fwsc`/controller SHA-256, package CRC/size, build IDs, or DEV CHECKED outcomes. Do not transplant historical binaries or relabel their hashes. No flashable URLs or publication approval.

## NIGHTLY-first status handoff

NIGHTLY Catalog Sync may record both models as **SOURCE-ONLY INTEGRATION WIP / BUILD PENDING / NOT INSTALLABLE**, pointing to exact firmware source HEAD/tree above; keep `version: UNASSIGNED`, `artifact: none`, `controller: none`, `physical: untested`, no flash URLs. Its own isolated source/status PR + independent review and sole NIGHTLY Pages publisher remain exclusive. Current NIGHTLY `AGENTS.md` still asks for per-build approval; draft NIGHTLY policy #2 must reconcile before publication. Standing owner policy permits DEVICE-NOT-TESTED software-qualified test releases after **all** remaining prehardware gates; lack of physical PASS does NOT alone block NIGHTLY, but here automated model/package/QA gates remain missing. Do not copy proprietary source/deps/firmware into the public browser-only NIGHTLY repository.

Production `main`/public picker require exact physical proof on each model, reviewed matching artifacts, a truthful NIGHTLY checkpoint and separate explicit owner production approval. **No production, firmware NIGHTLY, website, Pages, or hardware changes made by this evidence lane.**

**Next consumer:** Independent QA/sole integrator inspect ATT-response full-mailbox rejection and remaining known BF-004/BF-006/BF-007; Catalog Sync records noninstallable build history. Hourly Build resumes real native Elite/Pro DEV and controller tests only on a fully reconstructed and pinned exact accepted integration HEAD. Preserve present WIP/NO-GO state until genuine results exist.
