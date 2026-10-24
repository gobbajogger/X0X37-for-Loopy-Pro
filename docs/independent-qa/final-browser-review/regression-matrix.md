# Final bounded PR8 browser regression matrix — PREPARED, NEW HEAD PENDING

This is an executable review plan, not a new candidate review or PASS. Parent reports the existing PR8 producer is repairing F5/F6/F7 and porting existing browser model/binding/backup guards. Wait for its new immutable head and complete root before executing final candidate QA. Last reviewed source remains `068f3cd15dd4698cd6d609dee54628d73bd51686`, root `c39126aac8fcca9e72e5a7c1157b546ecb5366e3`; overall NEEDS CHANGES. Existing producer, independent QA, sole Website Main Merger and sole Pages publisher ownership remains unchanged. Parent coordinates the next stage.

## Actual route and reference pins

The website executes `web/x0x_installer.html` through inlined `web/fm1ota.js`, `web/fm1pkg.js`, and `web/x0x_breaks.js`. Its two install handlers call browser `run()`: detected OTA uses `Updater.resume`; normal install uses `installKeepingSlots` and `Updater.install`, then `Updater.write`. Sample restore goes through the same browser Updater and slot helpers. This route does not call Python or `physical_id`.

User-supplied existing browser reference was independently read at production-source commit `c2e63feb11385b91a9dbd3b29b20a368ed948a5a`:

- `web/fm1ota.js` Git blob `485a54187e78822847f5cb51088204983caffa71`: `write` model check before UPGRADE, `bind`, `singlePort`, target-name/model checks, verified post-install binding, resume product check.
- `web/x0x_breaks.js` Git blob `bb388f049747acd02b65931968702f1283ad4bc4`: bind before backup, fail closed on unknown sample layout except explicitly recognized stock/no-slot-command response, required backup/archive/readback guards.

These are source references, not certification of a future PR8 port. No reference code was copied into or authored on the producer branch. No native PR114 mocked `physical_id` result is treated as a browser failure. Browser tests must exercise actual JS methods and observable WebMIDI ports/identity; no invented hardware serial or physical-unit proof.

## First actions on receipt of final pin

1. Read exact new PR8 head/root, live AGENTS/GPL/build rules, producer diff/receipts and latest PR3/6/7/8 comments/threads. Independently verify full remote root and a clean complete checkout. Record current base and existing owners; don't consume an unpinned moving branch as a tested candidate.
2. Review complete combined executable/source/generated scope against the last reviewed source. Record which tests changed and preserve meaningful prior assertions, seven npm suites, candidate preparation, source/generated/embedded parity, truthful history locks and empty unqualified real catalog. New intentional test counts must be explained; a raw count alone is not coverage.
3. Execute native `npm test`, `npm run build`, `npm test`, default `npm run test:browser`; capture each actual exit/log and exact final source identity. Run applicable independent pinned fixtures plus the matrix below, extending only affected boundaries.

## Acceptance matrix

All rows are **PENDING ON NEW EXACT HEAD**, regardless of historical passes.

| ID | Operation and fixture | Required observation |
| --- | --- | --- |
| F1 | Verified Blob then malformed/null/duplicate/replaced/cyclic current selection; real click | Original default action cancelled before unsafe lookup, old Blob revoked, image/product/link cleared, no download/errors; restored valid metadata and keyboard Enter still download exact approved bytes |
| F2 | Actual complete read-only history at320/375/768/1280 | Every expected immutable row/build visible; document and card/title/paragraph/badge contents fit; no deletion/clipping to obtain fit |
| F3 | Null history/model, late card-construction error and failure committing second list after first commit | Error visible, both lists empty, no partial cards or uncaught error; valid positive still renders |
| F4 | schema999/unsupported status, forged DEVICE VERIFIED, malformed presentation types, duplicate IDs | Refuse whole response without fake tags or partial records; actual full/empty/inert-text/future-typed-append controls accepted |
| F5 | Legitimate synthetic candidate CLI plus synthetic unexpected fields at catalog/version/model/qualification/package/integration/controller/gate/budget levels | Required legitimate fields preserved; strict input policy enforced; no synthetic private marker in actual installer/versions or other prepared public files; no unrestricted private qualification output; real valid CLI control remains usable |
| F6 | Valid bounded response; oversize/truncated/error/held/stale body; body missing reader; lying/missing Content-Length | Enforce approved length while reading, cancel oversize/stale/error; no unbounded arrayBuffer fallback; reject size/hash/product failure before Blob/MIDI; exact bounded positive and retry still work |
| F7 | Future noninstallable history append with matched SHA/URL, mutable main, mismatched40hex, object-valued evidence_commit | Matched pinned control accepted; three corrupt cases rejected by actual copied exact-source validator; existing immutable rows and legitimate append remain intact; no moving-history rebase required |
| P1 | PR6 direct-HREF six negatives, revoked/blocked/withdrawn/Release/wrong model/size/hash/controller | No unverified remote Download link or MIDI/install admission; byte-verified Blob positive for Elite/Pro still succeeds |
| P2 | PR7 lifecycle/current/superseded/string-Release/device-claim/model-status qualification | Reject all known unsafe states while admitting a complete valid synthetic qualification; include exact streamed download bytes and local-controller hash checks |
| P3 | DEV/local actual handlers repeated or concurrent before MIDI grant, denial, interruption, selection change and retry | One writer reserved before permission; no second installer entry; stale callback cannot authorize new selection; lock released on failure and valid retry works |
| P4 | Guarded manual-local file on empty real catalog, invalid/stale file and controller reads | Valid local recovery remains possible with acknowledgements/model guards; invalid/stale reads clear or cannot replace authorization; no unchecked vendor anchors restored |
| B1 | Actual browser `Updater.install` from supported running app, legitimate Elite/Pro alias combinations, selected loader and final exact product | Accepted aliases complete the synthetic flow; actual `write` runs and correct logical image/product reaches serve; no stub replacing write/model guard |
| B2 | Actual `install -> waitFor(loader) -> write` with selected SMK/Elite package but `ota-FM-1` loader; reverse family | Error before loader UPGRADE or data serve; close rejected link; count app-stage and loader-stage sends separately. This is the reported browser-specific counterexample, independent of Python mocks |
| B3 | Direct actual `write` with valid bound product and same-family loader; foreign loader; absent product | Legitimate same-family write proceeds; cross-family/unknown product refuses before UPGRADE/serve. Preserve actual guard; stub only I/O/timing/protocol boundaries |
| B4 | Actual `resume` with supported loader alias/product; foreign-family loader; missing product | Positive resume works; foreign/unknown identity rejects before UPGRADE/serve; no regression from existing resume model guard |
| B5 | `sameKeyboard` legitimate Pro/Elite/SMK aliases inclMKE-P37,STARRYKEY-37, supported SMK+VA; FM-1 same-family | Preserve existing supported alias semantics; do not require an accidental exact Elite-versus-Pro string equality. Unrelated family must refuse |
| B6 | Actual `bind/singlePort` with one selected compatible port, two compatible ports, selected name changed before upgrade | Sole selected positive accepted; observable ambiguity/name change refuses before UPGRADE or sample backup/write; no swallowed error |
| B7 | Actual `find/waitFor` after binding: sole port's legitimate loader rename, changed family, multiple compatible ports | Supported single-keyboard rename within existing semantics works; wrong family/ambiguity refuses before destructive action. Do not invent a new identity design |
| B8 | After final firmware verification, extra compatible port or changed selected port before slot restore/readback | Reject observable binding change/ambiguity before sample writes/readback from another port; exact bound positive retains successful restoration/result |
| S1 | Actual `installKeepingSlots`: recognized running X0X layout request/read failure | Throw slotbackup/error and never call install or send UPGRADE; link closes. Don't reinterpret an unknown layout as safely empty |
| S2 | Known legacy stock FM-1_015 or supported SMK-family version16 with exact no-answer(slot command15) response | Preserve the narrow supported stock/no-slot path if present; unrelated timeout/CRC/malformed/layout errors still refuse. Keep normal valid layout/empty slots positive |
| S3 | Required at-risk slot backup read fails; generated archive fails verification | No install/UPGRADE; preserve recoverable state, release writer and allow retry. Check actual browser function bodies, not unrelated Python outcome |
| S4 | Successful required backup, logical-image install, restore then mismatched readback; successful matching readback | Save verified archive before installation; mismatch explicitly fails and retains recovery evidence; exact matching readback succeeds |
| C1 | Current-source integrity/privacy/license/provenance and full suite union | GPL/current policy retained; data/source/generated parity; no private SDK/controller/firmware/user bytes or test hooks in production; no hidden download surface; record root/hash/commands independently |

## Test construction boundaries

Model/binding tests use the actual imported/evaluated browser Updater class and actual `install`, `write`, `resume`, `bind`, `singlePort`, and `find` where relevant. Fake WebMIDI inputs/outputs can supply deterministic SysEx identity replies; record handshake, UPGRADE and payload counts by target port. Timer delays and device protocol servicing may be stubbed to keep tests bounded, but never replace the guard under test. For install's wrong-loader case, preserve `install` and `write` and inject the staged loader at the find/wait boundary; separately exercise real find/bind against observable port fixtures.

Slot tests run actual browser helper bodies and count whether installer/UPGRADE/sample writes were reached. A required backup-read failure is distinct from the narrowly identified stock firmware not implementing the slot-layout command. Keep both negative and legitimate recovery positives.

Browser ports and reported model/version are not a cryptographic/durable physical-unit identity. A same-valued simulated replacement cannot be used to manufacture a browser `physical_id` requirement or hardware certification. Only actually observable changes supported by current browser guards belong in this bounded source review. Real per-model firmware/OTA/controller/backup/device qualification, legal distribution, immutable external hosting and live served HTTPS bytes remain separate pending gates. No real flash or private/vendor upload occurs.

## Disposition and routing

Per clarified parent instruction, F3 `PRRT_kwDOVUCQlc6v4rjc` and F4 `PRRT_kwDOVUCQlc6v4rje` were individually resolved against the unchanged tested R3 head above, with links to review5638498157 and independent evidence255e00f5330409eb5bdb834d488ad8ee51602efc. Original F1/F2 remain resolved. F5/F6/F7 remain unresolved. Resolution of repaired scope is not whole-site acceptance.

When the new candidate arrives, give one exact-head whole-source verdict covering this finite matrix and ALL known superseding PR3/6/7 findings. Resolve only individually verified addressed own threads after posting attributable evidence. A failure returns the concrete fixture to the existing producer; do not author a fix then self-approve. Parent controls readiness/sole-merger dispatch; reviewer does not merge or publish. Any source change invalidates affected checks. No new task, scheduler or competing owner is created by this preparation.
