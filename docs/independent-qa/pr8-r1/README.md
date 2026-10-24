# PR8 independent website QA — NEEDS CHANGES

Candidate `363b128793c3aecf49a6eca293d6ab74a1cd7f0b`, complete root `d50b659b84de5859a3a0bd261221c2ed95ccda73`, base `1a176e156b049f476074b77d7e6d54be03e8c9ad` in `gobbajogger/x0x37-for-Loopy-Pro_nightly`. Independently cloned from remote and checked detached; native Git branch readback matched. QA did not author or modify the candidate. Evidence is separate from the unchanged candidate. PR8 had no existing comments/reviews on initial and prepublication reads; prior PR3/6/7 scoped reviews were consumed. Existing producer, sole merger and publisher ownership is preserved.

## Findings requiring producer disposition

**F1 (P2), malformed selected data can leave the previous verified download active.** In `web/x0x_installer.html:642-654`, `currentVersion()` and `v.models[chosenModel]` execute before `event.preventDefault()`. After a valid package has been verified, replace its selected model map with `null`. A real Chromium click throws `Cannot read properties of null (reading 'pro')`, does not cancel anchor default behavior, retains the old Blob href, and downloads the previously verified 1,056-byte synthetic Pro package. SHA-256 `060f2f9eaf36b4e740d5cc7dea9a1122101d80996d1fdf0de343a30f2adef790`, exactly the old fixture. `versions={}` and `versions[0]=null` likewise throw before cancellation. This violates the requested malformed/stale-selection fail-closed behavior. The fixture uses a test-only in-place mutation, as do existing metadata/revocation regressions; it does not demonstrate a remote attack, real firmware download, or an external catalog update path. The preparation validator rejects these malformed *initial* catalog shapes. Guard every event exit, including malformed catalog lookup; clear cached admission on invalid shape; keep a valid nonempty positive fixture.

**F2 (P2), actual history fails responsive layout.** Serve the unchanged `docs` tree and load `/builds/` with its real `build-status.json`, not a reduced synthetic history. Chromium viewport/scroll widths are `320/618`, `375/618`, `768/777`, `1280/1280`. Long SHA strings in description paragraphs (e.g. `integration-pr103-restoration-ota-ack-reviewed-source-20261024`) and `BUILD_SUCCEEDED_UNREVIEWED` titles overflow cards; a desktop card also overflows even though the document fits. In `docs/builds/index.html:17-24`, only `code`/`dd` have wrapping rules. Preserve all 117 immutable rows and fix presentation rather than deleting or changing evidence.

`focused-repro.log` contains the real downloaded-byte result and element geometry. `independent-browser-results.json` records every expanded assertion. Screenshots were captured locally under `/workspace/pr8-qa-evidence` and are not committed.

## Actual verification

Linux x86_64, Node v24.19.0, npm 11.9.0, Playwright 1.62.0, Chromium 154.0.8037.57.

| Command | Exit / result |
| --- | --- |
| `npm test` before build | 0; all seven suites |
| `npm run build` | 0; no tracked generated changes |
| `npm test` after build | 0; 56 admission cases, 72 qualification negatives, 117 locked rows |
| `npm run test:browser` | 0; 182/182 assertions |
| `python3 independent_browser.py` | 1; 76/82 pass, six assertions expose F1/F2 |
| `python3 focused_repro.py` | 0; F1 actual click/download and F2 geometry reproduced |
| `node backup_boundary_checks.mjs` | 0; four scoped checks |

The additional browser tests use uninstrumented `renderInstaller` output for 320/375/768/1280, actual EN/JA language and Elite/Pro radio controls, real fetch with routed HTTPS synthetic package responses, real SHA/product parsing, actual Blob downloads and byte comparisons, acknowledgement, focus and layout. Instrumented extra cases expose only module state or replace MIDI/install boundaries. Actual manual-local file controls work on the uninstrumented empty page; invalid local files clear prior authorization. Exact local target bytes and pre-permission lock, stale local-file completion/rejection, and all 117 real history rows were checked. Test hooks never enter production source or generated output.

Registered Chromium coverage independently replayed the producer's premature/badhash/revoked/unqualified/stale, model change, empty catalog, interrupted network/body/digest/scan/lookup/controller/permission, retry and concurrent-write tests. Scoped backup checks execute the actual `installKeepingSlots` and `restoreMapped` bodies with mocked boundary functions: required-backup failure and archive-verification failure prevent install; mismatched restored readback rejects; matching readback succeeds. These are browser-source boundary tests, not physical data-safety certification.

The PR7 baseline was independently replayed: admission exited 1 with 45 unsafe failures out of 119 assertions; the race exited 1 with two permission requests and two installer entries. These expected RED logs verify the original failures and are separate from repaired-candidate results.

All six producer log hashes/lengths and nine tested blob pins matched the receipt. Seven selected PR3 R25 status/history/validator/test/lock files are unchanged exact bytes. Source/generated status and empty catalog match; native build regenerated installer/libraries without tracked changes. GPL notices and current main AGENTS policy remain; no firmware/controller/vendor binaries are in the tree. Full combined executable diff (PR3/6/7 plus PR8) was reviewed. Test/receipt hashes are in `receipt.json`.

## Reproduction

Clone and detach at the candidate above. In the evidence directory, set `PR8_QA_SOURCE` to the detached checkout. Run `python3 independent_browser.py`, `python3 focused_repro.py`, and `node backup_boundary_checks.mjs`. Python Playwright and `/usr/bin/chromium` are required. The default source path is `/workspace/pr8-independent-qa`. The browser harness serves loopback only and uses synthetic bytes; it neither uploads nor flashes anything.

No real firmware, matching controller qualification, OTA/hardware behavior, lawful immutable public host or deployed HTTP bytes are certified here. Elite and Pro remain NO INSTALLABLE YET. No main/nightly/gh-pages, merge, readiness, release, schedule, hardware or producer-source changes were made. The GitHub review is COMMENT evidence because the connected account is also the PR author; it is not independent human GitHub approval.

The parent owns repair dispatch and later sole-merger dispatch. This failed stage must return to the existing producer and then fresh exact-head independent QA. Do not mark ready or invoke merger on this verdict.
