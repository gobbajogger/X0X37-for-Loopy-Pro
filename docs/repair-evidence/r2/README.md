# PR #8 R2 producer repair — independent re-review pending

This fast-forward continuation repairs both findings from the independent review
of source `363b128793c3aecf49a6eca293d6ab74a1cd7f0b` (root
`d50b659b84de5859a3a0bd261221c2ed95ccda73`). It does not approve that source or
replace the independent reviewer. The QA scripts and original findings remain
at evidence commit `76e059e17db12265bf2062144eb7feee1df7e5f8`,
[`docs/independent-qa/pr8-r1`](https://github.com/gobbajogger/x0x37-for-Loopy-Pro_nightly/tree/76e059e17db12265bf2062144eb7feee1df7e5f8/docs/independent-qa/pr8-r1).
The review is
[5638336932](https://github.com/gobbajogger/x0x37-for-Loopy-Pro_nightly/pull/8#pullrequestreview-5638336932).

## Changed behavior

The Download handler synchronously cancels its original anchor default action
before any mutable metadata lookup. Only a successful complete validation can
create a separate download anchor for the current verified Blob. A malformed
lookup, exception or changed identity clears the cached Blob, image, product
and link, disables Install and invalidates pending fetch completion. Version
list/selection and admission predicates now return a refusal on malformed
shapes, duplicate version identities, throwing properties and unserializable
metadata. Valid nonempty Elite/Pro Blob downloads and guarded manual local-file
recovery remain tested.

History cards inherit `overflow-wrap:anywhere`, and badges have a container
width bound. No evidence text or record is deleted, clipped or hidden. All 117
history rows, both historical builds, source/generated status and empty real
firmware catalog remain byte-identical to the preceding head. The source
provenance record and generated installer were refreshed.

## Actual RED/GREEN evidence

The unchanged independent expanded script on the original head reproduced
**76/82 pass, six failures**, matching F1/F2. Its focused diagnostic performed
a real stale 1,056-byte synthetic Pro Blob download, SHA-256
`060f2f9eaf36b4e740d5cc7dea9a1122101d80996d1fdf0de343a30f2adef790`.
History viewport/scroll widths were 320/618, 375/618, 768/777, 1280/1280,
including card-level overflow. See `focused-before.log` and `expanded-before.log`.

On repaired source:

- `npm test`, `npm run build`, `npm test`: exit 0; all seven suites,
  58 admission cases, 72 qualification negatives, 117 pinned rows.
- `npm run test:browser`: exit 0, **208/208**. The default registered browser
  suite retains all prior 182 assertions and adds real-click malformed-data
  tests (including throwing getters/cyclic metadata) and all four actual-history
  widths with card-content overflow checks.
- The unchanged independent `independent_browser.py`: exit 0, **82/82**.
- The independent `backup_boundary_checks.mjs`: exit 0, all four checks.
- `focused_after.py`: exit 0. This adapter preserves the independent fixture
  and actual click/geometry checks while expecting cancellation after the fix.
  All three reported malformed mutations now yield zero downloads, zero page
  errors, no cached image/href, disabled Install, and an actually revoked old
  Blob URL. Full history/card contents fit at 320/320, 375/375, 768/768 and
  1280/1280, with 117 history rows and two builds at every width.

Runtime: Linux x86_64, Node 24.19.0, npm 11.9.0, Playwright 1.62.0,
Chromium 154.0.8037.57. Package bytes and device/installation boundaries remain
explicit synthetic/mock fixtures; no real firmware or hardware is certified.

## Reproduce and consume

Use `npm test`, `npm run build`, `npm test`, `npm run test:browser` in the exact
new PR #8 checkout. To run the independent scripts unchanged, extract the pinned
QA evidence directory above, set `PR8_QA_SOURCE` to that checkout, and run its
`independent_browser.py` and `backup_boundary_checks.mjs`. Run the adapter here
with `--qa-evidence PATH/TO/docs/independent-qa/pr8-r1` and the same environment
variable. Its original `focused_repro.py` intentionally expects the stale
Download and is used only for RED reproduction on the original source.

`receipt.json` records exact tested blobs and log SHA-256 identities; the PR
body supplies the final pushed commit/root and complete parent-controlled
re-review packet. The original R1 logs/receipt remain historical evidence and
are not relabeled as current results.

Status remains **REPAIR HANDOFF — PRIOR WEBSITE QA INCOMPLETE**. Keep PR #8
draft until the distinct existing reviewer clears the new exact head. Parent
alone coordinates re-review and any later sole-merger dispatch. No successor,
merge, protected-ref write, Pages publication, firmware/controller upload,
schedule change or device operation occurred. Elite and Pro remain
NO INSTALLABLE YET; real per-model safety/legal/hosting/served-byte gates remain.
