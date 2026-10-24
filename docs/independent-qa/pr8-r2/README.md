# PR8 R2 independent website QA — PASSED (source only)

Independently reviewed and tested candidate `b0d3c2b88587948a428305210d8b76ebd4d40535`, full website root `5b5305ae79d109c87c5f5004fb81b2d3780f65cf`, PR8 producer branch `lane/browser-admission-repair-20261024`, public repository `gobbajogger/x0x37-for-Loopy-Pro_nightly`. Website main/base was `1a176e156b049f476074b77d7e6d54be03e8c9ad`. Both fresh remote clone and tested complete checkout match the exact head/root; the tested worktree remains clean. This reviewer did not author or modify the repair. No competing R2 independent full-root review/run receipt was present in the refreshed PR discussion.

The prior exact-head NEEDS CHANGES review remains historical: source `363b128793c3aecf49a6eca293d6ab74a1cd7f0b`, review [5638336932](https://github.com/gobbajogger/x0x37-for-Loopy-Pro_nightly/pull/8#pullrequestreview-5638336932), evidence `76e059e17db12265bf2062144eb7feee1df7e5f8`. Both prior findings are resolved on R2.

## Findings resolved

**F1:** The actual Download handler now cancels the original default action before any mutable catalog lookup. Successful validation alone creates a separate anchor using the current verified Blob. Malformed shape, duplicate version identity, throwing properties and cyclic/unserializable identity refuse admission. Invalidating a click clears cached image/product/link, revokes the old Blob URL and advances the load generation. Registered Chromium covers nine malformed cases with actual clicks. Additional independent focused checks cover the three original malformed mutations, duplicate version, replacement package-object identity and cyclic metadata. All six have zero downloads, zero page errors, no cached image/href, disabled Install, and the prior Blob URL actually fails to fetch. Each case then accepts a restored valid catalog in the same page, re-enables Install only after acknowledgement, and produces the exact 1,056-byte synthetic Pro download through keyboard Enter. SHA256 `060f2f9eaf36b4e740d5cc7dea9a1122101d80996d1fdf0de343a30f2adef790`. Valid nonempty Elite/Pro controls/downloads remain covered by the independent unchanged expanded suite.

**F2:** The real complete history retains 117 rows and two historical builds, with unchanged source/generated status bytes. At 320/375/768/1280, document scrollWidth equals viewport width and card/title/paragraph/definition/badge content has no measured overflow. Wrapping and badge width rules make long hashes/status tokens visible without deleting, clipping or hiding evidence.

## Actual execution on the final unchanged candidate

Linux x86_64, Node v24.19.0, npm 11.9.0, Playwright 1.62.0, Chromium 154.0.8037.57.

| Command | Exit | Result |
| --- | --- | --- |
| `npm test` before build | 0 | All seven suites |
| `npm run build` | 0 | Source/generated output matches; no tracked changes |
| `npm test` after build | 0 | 58 admission cases, 72 qualification negatives, 117 pinned history rows |
| `npm run test:browser` | 0 | 208/208 assertions |
| `PR8_QA_SOURCE=CHECKOUT python3 independent_browser.py` | 0 | Unchanged pinned independent harness, 82/82 |
| `PR8_QA_SOURCE=CHECKOUT node backup_boundary_checks.mjs` | 0 | Unchanged independent boundary script, 4/4 |
| `PR8_QA_SOURCE=CHECKOUT python3 r2_focused_checks.py` | 0 | New independent click/revocation/retry/keyboard/card checks, 22/22 |

The two original independent scripts are byte-identical to evidence commit `76e059e17db12265bf2062144eb7feee1df7e5f8` (SHA256 `c14d79b78fb865111eee2ed4b5c6bf47a50e4d4755295f282f79d022f0785026` and `701bb08d4b1ca31c8d80ae73244d950580da1ffeb27a47921d12c19418bb417e`). They were run by this reviewer, not accepted from producer claims. Logs, every expanded assertion, focused geometry/state and SHA256 identities accompany this file in `receipt.json`.

Coverage retains desktop/mobile 320/375/390/768/1280 as applicable, actual EN/JA controls, Elite/Pro model controls, real browser fetch/digest/product parsing and exact downloaded Blob bytes, premature/bad hash/size/product/controller/revocation/unqualified admissions, empty/malformed/stale catalogs, delayed network/body/digest/scan/lookup/controller/permission callbacks, repeat and concurrent click paths, interruption/denial/retry, acknowledgement, keyboard focus, manual-local recovery and invalid/stale local files. Scoped backup tests run the actual installer/restore function bodies with mocked boundary functions: required backup failure and archive corruption prevent install, mismatched restore readback rejects and equal readback succeeds.

Reviewed current AGENTS/GPL policy, the R2 executable/source/generated diff and original combined PR3/6/7/8 scope. Eleven R2 producer tested blob pins and all nine log hashes/lengths independently match. Fifteen policy/license/status/catalog/library/renderer/test-registry files remain exact bytes from the previously reviewed source. All seven suite registrations and prepare:candidate remain; source/generated/embedded parity passes; test hooks do not enter production output. The renderer's malformed-input negatives and valid nonempty positive remain green. No firmware/controller/vendor binary was added.

## Acceptance boundary and next owner

This is **independent browser-source QA PASSED on exactly R2**, not independent human GitHub approval, merger execution, firmware qualification, public hosting verification or physical-device evidence. Package/controller fixtures are synthetic; MIDI and install boundaries are mocked where described. Real Elite/Pro picker remains empty: NO INSTALLABLE YET. Actual per-model firmware/controller/OTA/backup/rollback/legal/immutable-host/served-byte gates remain separately pending.

Request the existing PR8 producer to transition its own source-complete PR from draft to ready after this acceptance. Reviewer did not change readiness. Parent alone invokes the existing sole NIGHTLY Website Main Merger using `next-stage-packet.md`; no successor was created here. Merger must re-read current head/base, effective rules, reviews and dependencies and run fresh actual combined-tree checks before any eligible normal expected-head merge. New source changes invalidate affected checks. Sole Pages publisher remains the following owner for deployment/HTTP validation. No main/nightly/gh-pages, merge, release, scheduler or device action occurred.
