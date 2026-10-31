# F6a site and data evidence — 1 November 2026

The F6a source repairs are implemented and independently reviewed. Full feature acceptance remains blocked by the cross-application atomicity limitation, the absent historical owner archive/device readback, and the failing full host gate. No merge, deployment, install or device write is part of this delivery.

## Source and reuse

Base: `build/0.0.23@bb1907494b8d1b52d52ac220dfeb9a0663ea3ee2`, tree `e340d89d180879b8d8a2fea07c56800a2feaac5c`. All 4,788 base blobs were verified against the GitHub recursive tree. The isolated lane is `wip/f6a-site-data`.

[Provenance](provenance.json) records all 12 PR #690 commits and per-path blob comparisons. Its history diverges from the target, but 26 of its 29 changed paths already match the base byte for byte. They were reused. The newer `web/fm1ota.js`, `web/x0x_installer.html` and `web/test_install_slots.mjs` remain unchanged. No mixed engine/site snapshot was cherry-picked, no firmware was reintroduced, and no B1 connection/install sequence or F6b editor file was edited.

The font is an exact component port from `5b7e7b7ab76df1b7a2ca7ea27b94690dd915743b`: `web/assets/GolosText.ttf`, Git blob `05ee5ecc570b97818e24e58693662cd8294fdfc9`, 184,292 bytes. The already-bundled SIL OFL is unchanged. The old font distribution hold predates this brief's explicit shared-font scope; this PR restores source assets and does not publish the site. CSS now declares the actual variable weight range 400–900, and documentation headings inherit the shared family.

## Changed behavior

- Samples-page writes retain all 512 original header bytes, including the reserved area. The shared write helper keeps its 480-byte default for the existing installer flow.
- Every project import boundary admits revision 13 only, including direct staging, prepared-file revalidation, recovery and rollback. Earlier files remain readable for inspection and exact backup. Even explicit legacy engine choices cannot send adoption opcode 75.
- Generated pages actually load the existing licensed Golos typeface. A positive shell-generation regression now runs as host selector `site-generation-web`.

## Requirement disposition

| Row | Source and measured result | Acceptance limit |
| --- | --- | --- |
| 51 | Existing all-slot archives reused; reserved-header restore defect fixed. Mixed slot sizes, empty slots, encoded bytes and rates covered by host checks. | Real-device restore not run. |
| 52 | Existing same-origin exclusive Web Locks and project transaction holds retained; competing-origin callback rejection passes. | Arbitrary external MIDI applications cannot be bound to the browser's snapshot/write decision. Closing this guarantee requires a firmware protocol change outside F6a's web ownership. |
| 53 | Existing session invalidation, stale preview rejection, retained recovery downloads and reconnect tests pass, including reused port objects. | No physical reconnect test. |
| 54 | Revision-13-only loading enforced at every API path; no migration. Historical inspection/export preserved. | Historical projects require their original firmware. |
| 55 | Synthetic archives restore every original selected slot byte, including empty slots and reserved header bytes. | The owner's complete historical archive and matching-device readback are unavailable. Historical completeness is NOT VERIFIED. |
| 75 | Existing single Install/Samples/Projects workspace reused and browser tested. | No deployment. |
| 76 | Revision-13 project-inclusive song/pattern/sound/sample transfer and rollback pass stateful host transactions. | No separate song format and no physical-device acceptance. |
| 77 | Existing site reused; exact original Golos font restored under its OFL. | Owner visual approval remains required before merge. |
| 78 | Real generated Manual, What's new and History load; catalog retention/version-order checks pass. | Shell fixture intentionally has no published package catalog. |
| 79 | Existing B1 connection controls remain unchanged; unsupported-browser and denied-permission states pass browser checks. | No real MIDI permission or hardware operation. |
| 80 | Existing local-file chooser and inline error handling pass browser checks, including Russian. | An existing isolated installer test fails on untouched base files because its VM harness lacks `t`; see below. |
| 89 | Existing offline speed-up pipeline reused; browser verifies 50% encoded size saving at 2× import speed and 22050 Hz playback, then exact transfer into Samples. | No perceptual quality or hardware playback claim. |

## Tests and review

- Focused final closure: **71 PASS, 0 FAIL, 0 SKIP**, exit 0 — [log](focused-final.log).
- Sample restore TDD: [red](sample-restore-red.log) loses reserved bytes and fails two assertions; [green](sample-restore-green.log) verifies complete restore.
- Revision-policy TDD: [red](revision13-red.log) has five expected failures. Final guard cases pass; [isolated removal of the two rollback guards](revision13-guard-mutation.log) causes the two expected failures.
- Font TDD: [red](font-red.log) fails because the generated font is absent; [green](font-green.log) verifies exact copied bytes, license and catalog guards. The registered selector passes.
- Generated-site Chromium acceptance: **PASS** at 320, 390 and 1440 pixels, plus denied permission and Manual/New/History checks — [receipt](browser-acceptance.json).
- Font/render validation: **14 PASS**, covering seven routes in light/dark at 320 pixels, actual font load, shared headings and no horizontal overflow — [receipt](font-browser-validation.json).
- Syntax and whitespace: JavaScript parsing, `sh -n tests/run_tests.sh`, and `git diff --check` pass.
- Independent Compound Engineering core review completed with no production defects. Its regression-coverage finding and additional backup-guard test gap were fixed and independently rechecked. [Core review](../../reviews/2026-10-31-f6a-core-review.md), [follow-up](../../reviews/20261031-f6a-followup.md). Font proposal independently passed before implementation: [design review](font-proposal-review.md).
- Full host suite: **RUNNING / FAILURES ALREADY OBSERVED**. The final count and full selector list will be recorded in `full-host-summary.json` after collection. The runner began before the new additive site selector; that selector was independently run through the updated host registry.
- Broader existing web regressions exposed an installer-local-file test failure. It reproduces on both untouched base files with `ReferenceError: t is not defined`; [base reproduction](b1-base-reproduction.log). The actual generated-page chooser/error browser checks pass. B1 files and checks were not weakened or changed.

Test changes intentionally replace positive legacy-import/adoption expectations with rejection, as required by the current revision policy. All historical inspection/export, corrupt-container, stale-session, rollback and acknowledgement checks remain. The Samples peer now accepts either 480 or 512 header bytes to match the verified current firmware contract; the new full-byte assertion detects header truncation.

## Native builds and RAM

Both native commands actually exited 0 using JieLi clang 4.0.1 and all three SDK package inputs matching the repository's V1.2.1 SHA-256 pins:

```sh
X0X_DEV_PADCAL=0 JIELI_TOOLCHAIN=<verified-toolchain> AC79_SDK=<verified-sdk> python3 tools/build.py --board smk37 --model elite
X0X_DEV_PADCAL=0 JIELI_TOOLCHAIN=<verified-toolchain> AC79_SDK=<verified-sdk> python3 tools/build.py --board smk37 --model pro
```

| Model | Static RAM used / total | Static RAM free | Pool free | Exit |
| --- | --- | --- | --- | --- |
| Elite | 95,952 / 98,304 B | **2,352 B** | 8,536 B | 0 |
| Pro | 95,952 / 98,304 B | **2,352 B** | 8,536 B | 0 |

Free static RAM matches the brief's 2,352 B baseline. Firmware inputs are unchanged by F6a. The existing compiler warning about braces in `miami_decimator_adapter.c` remains; it did not fail either build. [Elite log](elite-build.log), [Pro log](pro-build.log), [hashes and metrics](native-builds.json).

These are BUILD_ONLY development packages, with package identities `SMK-37 Elite_900` and `SMK-37 Pro_900`. No FM-1 target was built. No controller qualification, installability, release, publication or hardware PASS is asserted. SDK/compiler bytes and firmware/product binaries are not committed.

## Actual website screenshots and proposal paths

These PNGs are real Chromium captures of generated pages, with Web MIDI absent. They are not drawings or 240×240 device framebuffer dumps; no device UI was changed.

| Page | Before mobile | After mobile | Before desktop | After desktop |
| --- | --- | --- | --- | --- |
| Install | [390](before-install-390.png) | [390](after-install-390.png) | [1440](before-install-1440.png) | [1440](after-install-1440.png) |
| Samples | [390](before-samples-390.png) | [390](after-samples-390.png) | [1440](before-samples-1440.png) | [1440](after-samples-1440.png) |
| Projects | [390](before-projects-390.png) | [390](after-projects-390.png) | [1440](before-projects-1440.png) | [1440](after-projects-1440.png) |

The existing [workspace proposal](../../../web/workspace-proposal.html) was reused. The font proposal was independently reviewed before final implementation. Actual AFTER captures were regenerated from the final source.

## Remaining owners and next actions

F6a source owner: this lane. Firmware integration and B1/F6b owners remain unchanged. The integration owner must resolve the full host gate and the cross-client firmware protocol dependency; the owner must supply historical-archive/device acceptance and visual approval before merge. The PR remains open. No automatic merge or publication is enabled.
