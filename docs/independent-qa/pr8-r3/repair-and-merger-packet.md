# PR8 R3 independent QA — NEEDS CHANGES; history repair passes

Reviewed unchanged producer head `068f3cd15dd4698cd6d609dee54628d73bd51686`, complete root `c39126aac8fcca9e72e5a7c1157b546ecb5366e3`, PR8 branch `lane/browser-admission-repair-20261024`, public `gobbajogger/x0x37-for-Loopy-Pro_nightly`. Fresh remote clone and tested complete checkout match; candidate worktree remains clean. Reviewer did not author/modify repair, rebase moving data, merge or publish. Current AGENTS/GPL/build rules and live PR3/6/7/8 findings were inspected.

**R3 correctly repairs F3/F4**, including the exact merger-response fixtures; F1/F2 installer/layout fixes remain green. However, the requested review of **all known superseding PR3/6/7 findings** independently reproduces three unresolved boundaries on this same R3 root. Therefore no overall acceptance or merger handoff is granted. A limited history-only pass must not be represented as full-site acceptance.

## Actual checks

Linux x86_64, Node24.19.0/npm11.9.0, Python Playwright1.62.0, Chromium154.0.8037.57.

| Command | Actual exit/result |
| --- | --- |
| `npm test`, `npm run build`, `npm test` | 0/0/0; all seven suites,58 admission,72 qualification negatives,117 history pins |
| `npm run test:browser` | 0;208 original assertions +34 history-response cases |
| Pinned unchanged independent `independent_browser.py` | 0;82/82 |
| Pinned unchanged `backup_boundary_checks.mjs` | 0;4/4 actual-source boundary checks with mocked boundaries |
| Pinned unchanged `r2_focused_checks.py` | 0;22/22 including real clicks/revocation/valid retry/keyboard/layout |
| Pinned unchanged `history_response_negatives.py` | 0;8/8 exact supplementary cases |
| Independent `history_adversarial_extension.py` | 0;31/31 |
| Independent `dependency_boundary_checks.py` | **1;1/8 pass,7 failed expectations, grouped F5/F6/F7 below** |
| Current registered history tests against unchanged R2 HTML | Expected RED exit1;15/34 pass,19 fail |

All11 producer R3 tested blob pins and six log hashes/lengths match. Fifteen retained policy/license/status/catalog/installer/library/renderer/validator/lock files are exact bytes from R2. Real source/generated catalog remains empty,117 rows/two historical builds unchanged, generated-source parity and union of seven suites intact. No production test hooks or firmware/controller/vendor binaries added. Command/exits and SHA256 log/script manifests are in receipt.json.

## Known finding coverage on this exact source

| Source finding | R3 independent disposition |
| --- | --- |
| PR3 historical identity/digest/CRC/allowlist/append-only gates through R25 | Registered native source/provenance negative suites pass;117 immutable records retained |
| PR3 R26 runtime null history/model, schema999, forged DEVICE VERIFIED | **Fixed:** exact8/8 replay and new34/34 registered cases pass |
| PR3 R27 evidence_commit provenance guard | **Unfixed on PR8: F7**, real copied-validator counterexamples below |
| PR6/7 suite-registration collision | Union of all seven suites and prepare:candidate retained, native checks pass |
| PR6 direct HREF/revoked/blocked/withdrawn/model/Release negatives | Fixed,58 native admission cases and actual Chromium byte-verified Blob cases pass |
| PR6 unreviewed external vendor firmware anchors | Removed from current source; guarded manual-local recovery remains functional |
| PR6/7 two install handlers entering during pending MIDI permission | Fixed; registered actual-handler concurrent/repeat/denial/interruption/retry regressions pass |
| PR6 R23/R24 bounded-stream/no-reader source-owner handoffs | **Unfixed on PR8: F6**, independent real Chromium response-boundary tests below |
| PR7 lifecycle/superseded/string-Release/device-claim/model-status renderer negatives | Fixed;72 qualification negatives and actual browser admission checks pass |
| PR7 public manifest/private-field projection finding | **Unfixed on PR8: F5**, actual native candidate CLI output reproduced below |
| Own F1 malformed stale Blob and F2 history overflow | Remain repaired; unchanged82/82 and22/22 focused checks pass |
| Own F3 partial history and F4 schema/status runtime validation | Repaired; exact replay8/8, registered34/34 and independent extension31/31 pass |

The31-case history extension independently checks duplicate IDs across/between lists, malformed model/identity/presentation types, string booleans, unsafe/negative/fractional sizes, unsupported model/product/status, root null/array, late final-card failure and a **second-list commit failure after the first list was committed**. Every refusal leaves both lists empty without uncaught page errors. Actual117-row, empty-list, inert markup and typed future-append controls remain usable. This validates presentation, not authenticity of future evidence.

## Remaining exact counterexamples

**F5 / P2 — future public candidate outputs copy private/unknown fields.** Known [PR7 publisher finding6384589660](https://github.com/gobbajogger/x0x37-for-Loopy-Pro_nightly/pull/7#issuecomment-6384589660) applies to unchanged `scripts/render-qualified-nightly.mjs`. A legitimate synthetic catalog/qualification control validates; adding synthetic internal-controller, auth-token, Library-locator and controller-vendor-payload fields still validates. Independently executed the **actual native CLI** with a mocked synthetic fetch Response and real size/SHA/product verifier, not a source-window shim. CLI exits0 and prepares files. The exact sentinels appear in:

- `install/index.html`, `versions.json`: synthetic private binary marker and token.
- `qualification.json`: synthetic internal Library locator and controller payload.

No genuine private data, firmware or deployed leak is claimed; no candidate was hosted. Reject unknown fields per schema level and construct an explicit public projection; keep private/full qualification out of public outputs. Preserve legitimate allowed fields and all existing qualification negatives. This is source repair for the existing producer, not an action for the publisher to silently compensate.

**F6 / P2 — browser response reads are unbounded until after buffering.** Known [PR6 bounded-stream handoff6384617303](https://github.com/gobbajogger/x0x37-for-Loopy-Pro_nightly/pull/6#issuecomment-6384617303) and [no-reader handoff6384842378](https://github.com/gobbajogger/x0x37-for-Loopy-Pro_nightly/pull/6#issuecomment-6384842378) apply to current `loadSelectedPackage()`: it awaits `r.arrayBuffer()` before checking approved size. Real Chromium synthetic response without a readable body calls arrayBuffer once; safe fixture returns2MiB+1 bytes despite approved1056. An oversized readable stream emits4KiB, is fully pulled through close(5 pulls), and never cancelled. Both eventually deny image/Download/Install correctly, but fail the resource limit. No actual unlimited allocation or wrong-firmware admission was performed. Enforce approved byte budget while reading, cancel oversize/stale/error streams, and fail closed when no readable stream exists; retain valid bounded-stream positive and guarded local file behavior.

**F7 / P2 — future history append accepts unpinned/mismatched evidence_commit.** R27 `876adf33d3a96accac3838c21f6f8681d8c1ab39` adds a real guard absent from PR8's selected R25 validator. Independent native tests copy the exact PR8 validator, all required manifests/locks/evidence files into temporary directories and append one synthetic source-only record. Matched pinned SHA/evidence URL is correctly accepted(exit0). `evidence_commit='main'`, a mismatched valid40hex SHA, and `{ref:'main'}` are **also accepted(exit0)** with the same immutable URL. Current117 locked rows remain intact and no installability claim is made. Port the narrow guard or equivalent with positive/mutable/mismatched/type regressions in producer source; this does not require importing movingR26/R27 history data. The copied-fixture tests never change the candidate.

## Parent-controlled next action

Overall source QA remains **NEEDS CHANGES / NO MERGE**. Parent returns F5/F6/F7 to the existing PR8 producer, coordinating current Catalog Sync/history owner for the R27 guard. Repair only on an authorized owned isolated lane; preserve R3 history atomic validation, R2 installer/CSS,117 immutable rows, GPL/current main policy, all seven suites, safe empty real catalog and correct model/controller/manual-local safeguards. Do not commandeer PR3/6/7/9 lanes or rebase moving history by assumption. Add these meaningful counterexamples plus positive controls, run native test/build/test/default Chromium, and deliver fresh remote head/root/log evidence. Fresh independent full-root QA must cover **every known finding**, not just the newest repair. Only after that acceptance may producer readiness and the existing sole Website Main Merger resume; parent retains dispatch.

F1/F2 remain resolved. F3/F4 are verified fixed on R3 but left unresolved because the requested **overall-pass condition for closing them was not met**; no unrelated thread was resolved. This records repaired scope truthfully without manufacturing overall acceptance. New findings remain outstanding. Any new source change invalidates affected checks.

No real firmware/controller/OTA/physical/hosting qualification is claimed. Elite/Pro remain NO INSTALLABLE YET. No readiness change, protected main/nightly/gh-pages write, review deletion/protection change, merge, public artifact upload, device action or scheduler mutation occurred. Evidence-only publication is separate from producer source. No successor was created; the parent coordinates repair and later sole-merger dispatch.
