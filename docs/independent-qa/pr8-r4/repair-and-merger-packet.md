# PR8 R4 independent QA — NEEDS CHANGES; one existing F7 corner remains

Exact unchanged producer candidate `02a5a05ee0aa9677884aadf1e0bb48e1b31d6cad`, full root `eb903cc7a8ae658e99894b0e7b7b995eb9437c74`, PR8 `lane/browser-admission-repair-20261024`, public `gobbajogger/x0x37-for-Loopy-Pro_nightly`. Reviewed main/base `1a176e156b049f476074b77d7e6d54be03e8c9ad`. Fresh remote clone and complete tested checkout independently match; worktree remains clean. Reviewer did not author/change the repair, merge, publish or flash hardware.

The prepared finite matrix is at evidence commit `8357bea49e1b80c1d6b315dc749692c92424b0eb`. R4 fixes F5/F6, the original three F7 cases, and the actual browser loader/binding/backup paths. All native/default/supplied suites pass. However, the already-reported R28 extension of F7 still accepts a claimed SHA placed later in a URL whose actual Git ref differs. Overall acceptance remains **NEEDS CHANGES**, not a failed installer/model test.

## Actual independently executed checks

Linux x86_64, Node24.19.0/npm11.9.0, Playwright1.62.0, Chromium154.0.8037.57.

| Check | Exit/result |
| --- | --- |
| Native `npm test`, `npm run build`, `npm test` | 0/0/0; all prior seven suites plus new write-boundary suite |
| Registered default Chromium | 0;215 installer assertions +34 history cases |
| Actual browser JS shipping bodies, registered in npm test | 41/41; actual install/write/resume/bind/find/singlePort/slot helpers, synthetic discovery/serve/MIDI timing boundaries |
| Unchanged independent expanded browser | 0;82/82 |
| Unchanged focused real click/keyboard/history | 0;22/22 |
| Unchanged supplied history-response replay | 0;8/8 |
| Unchanged independent adversarial history extension | 0;31/31 |
| Unchanged supplied dependency boundary replay | 0;8/8 |
| Original scoped backup/archive/readback plus labeled mock bind API adapter | 0;4/4; actual binding separately exercised by41-case suite |
| Additional actual Updater wrong-app-family/disconnected-loader cases | 0;3/3; zero UPGRADE/serve on rejection |
| Equivalent PR102 R25 package-size/receipt/actual-byte tests | 0;4/4; legitimate control accepts, altered metadata/bytes refuse |
| `python3 final_matrix_extension.py` | **1;5/6 pass, one existing F7 URL-ref case fails** |
| Existing R28 validator comparison | 0; confirms both controls accept and only current R4 accepts the shadowed URL-ref fixture |

Native qualification suite also exercises the actual preparation CLI: valid exact-byte fixture prepares usable public catalog/installer; unexpected/private fields at every documented schema level refuse; full qualification.json is absent; inside/aliased output paths reject and genuine outside/nested paths work. Browser read budget now refuses no-reader response without arrayBuffer, and cancels oversize streams early (independent fixture:3 pulls/1 cancellation instead of prior5/0). No assertion or source guard was weakened for old mocks; the backup-only interface adapter is explicit and its original file is retained alongside it.

All18 producer tested blob pins and20 producer log hashes/lengths independently match. Twelve retained policy/license/status/catalog/history/old-test/fm1pkg files are byte-identical to R3. Source/generated/embedded parity passes;117 immutable rows/two historical builds and actual empty picker remain unchanged. No test instrumentation or private firmware/controller/vendor bytes enter production output. Full R4 executable diff, current AGENTS/GPL, prior and superseding PR3/6/7/8 findings were inspected.

## Finite matrix disposition

- **F1/F2:** malformed stale-Blob click, exact download, valid retry/keyboard and full-history responsive/card geometry remain repaired.
- **F3/F4:** schema/status/nested shape and atomic partial-render failures remain repaired; exact replay and independent extension pass; these threads were already individually resolved.
- **F5:** strict per-level fields, explicit public projection, actual valid/no-private CLI output and no unrestricted public qualification are verified. Resolve own F5 individually after posting this evidence.
- **F6:** bounded readable streams, missing-reader refusal without full buffering, early oversize/stale cancellation, exact-size/hash/model positive and retry all pass. Equivalent fixed receipt/catalog size equality plus bounded actual bytes rejects the upstream size-inflation example. Resolve own F6 individually.
- **F7:** pinned/mutable/mismatched/object evidence_commit cases are repaired. The existing R28 URL-reference extension below remains open.
- **P1–P4:** prior admission/revocation/Release/model/controller/vendor-link, two-handler pre-MIDI concurrency, interruption/denial/retry and guarded local-file recovery pass the complete suites.
- **B1–B8:** actual `install -> waitFor(loader) -> write` refuses ota-FM-1 for an Elite package and the reverse family before loader UPGRADE/serve. Legitimate Elite/Pro/MKE/Starry/SMK+VA aliases and FM-1 positive remain accepted. Direct write/resume unknown product/family refuse. Real observable find/bind/singlePort handshake/ambiguity/name-change and verified post-install port guards pass. Actual shipping methods were reviewed and executed, not replaced by a stub at the guard under test.
- **S1–S4:** unknown X0X layout/read/send/malformed response and required-backup failure prevent install; narrow known stock FM-1_015/SMK-family016 no-slot path remains usable; valid empty layout proceeds; archive verified/saved before install; observable restoration binding changes and mismatched readback refuse while the exact positive restores successfully.
- **C1:** current GPL/policy/provenance, complete suite union, source/generated/embedded parity, immutable history and private/public separation verified.

The41-case source suite runs browser JS in Node VM with explicit synthetic I/O/timing fixtures; Chromium executes the rendered-page suites with fixture networking/MIDI boundaries. Neither certifies a real device or durable physical-unit identity. Python physical_id/PR114 native mocked outcomes are not imported as browser failures.

## Remaining F7 — claimed SHA later in a pathname is not the actual Git ref

This is case4 of the already-existing [sole-merger R28 receipt6385168347](https://github.com/gobbajogger/x0x37-for-Loopy-Pro_nightly/pull/8#issuecomment-6385168347), not a new identity architecture or moving-data requirement. On temporary copies of the **actual R4 validator** and its required unchanged manifests/locks, append one synthetic noninstallable source-only row:

- `evidence_commit = 1111111111111111111111111111111111111111`
- `evidence_url = https://github.com/gobbajogger/X0X37-for-Loopy-Pro/blob/ffffffffffffffffffffffffffffffffffffffff/docs/1111111111111111111111111111111111111111/receipt.json`

Expected: reject, because the actual `/blob/<ref>/` ref is all-f, not the claimed all-1 SHA. **Observed R4 exit0**, prints `PASS: 2 nightly builds +118 history; no unapproved installer entries`. Its substring check finds the claimed SHA later in the path. Existing117 locked rows are preserved and no download/installation claim is introduced. Matched actual-ref control exits0; wrong-tree and mutable-ref controls reject.

The existing exact Catalog R28 validator from commit `9d03fe036220c02b224a91a90c0bb09b66fd29fe`, blob `9cd11cf06a1277238d5eafc470ab315177f99264`, was independently compared using the same copied inputs: it accepts the matched-ref control and **rejects the shadowed-ref fixture exit1**. Its narrow guard extracts the40hex immediately after commit/tree/blob and compares that to evidence_commit. R4 ported R27, so this previously reported extension was missed. This comparison is existing reference code in temporary tests, not a reviewer-authored repair or alteration of PR8.

`final-matrix-extension-results.json`, `r28-reference-results.json` and their runnable scripts/logs contain exact observations. Do not describe this as a displayed firmware link, unverified hardware upgrade, secret leak, or corruption of current117 records. It is a future-source-evidence binding defect.

## Parent-controlled bounded repair/merger packet

Parent returns only this remaining F7 URL-reference case to the existing PR8 producer. Port the existing narrow R28 guard/equivalent into the owned producer lane and add matched-ref, wrong-ref-with-claimed-SHA-later-in-path, wrong-tree and mutable-ref regressions. Preserve current117 rows; no moving R28/R29 data rebase is required. Preserve all verified R4 public projection/stream/model/binding/backup/history guards and test union. Deliver fresh remote head/root and applicable native/default/negative logs. Independent QA then verifies the final immutable root and any changed/invalidated tests; unchanged browser evidence may only be mapped forward with actual byte identity and applicable required checks. Never turn this R4 NEEDS CHANGES receipt into another root's acceptance by assumption.

F5/F6 are individually verified addressed and may be resolved with links to this exact evidence; F7 stays unresolved. F1–F4 remain resolved. Only after full acceptance does producer-owned readiness and the existing sole Website Main Merger resume through normal fresh expected-head/protected workflow. No self-approval or publication by this reviewer; parent retains dispatch. PR9/other catalog proposals are separate inputs needing their own fresh combined-tree reconciliation/QA if selected.

Real Elite/Pro remain NO INSTALLABLE YET. Firmware/controller/model/OTA/legal/immutable-host/served-byte/physical gates remain separately pending; browser source qualification is not hardware-tested evidence. No protected main/nightly/gh-pages write, source fix, merge, release/upload, schedule or device action occurred. No successor created. Evidence branch changes only independent QA material.
