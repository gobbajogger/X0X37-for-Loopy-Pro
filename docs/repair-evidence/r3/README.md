# PR #8 R3 — history-response repair, independent re-review pending

This continues the same producer branch after supplementary independent
[review 5638417379](https://github.com/gobbajogger/x0x37-for-Loopy-Pro_nightly/pull/8#pullrequestreview-5638417379)
reproduced F3/F4 on R2 `b0d3c2b88587948a428305210d8b76ebd4d40535`, root
`5b5305ae79d109c87c5f5004fb81b2d3780f65cf`. Earlier F1/F2 remain resolved;
the R2 overall acceptance/readiness was superseded. PR #8 was returned to draft
and its merger handoff placed on HOLD before this repair. Parent retains all
reviewer, merger and successor dispatch.

## Repair

The served history page validates schema 1, read-only kind/flags, arrays,
unique record IDs, allowed build/history statuses, source/hash identity shapes,
model/product/size fields and typed presentation strings before constructing
cards. Unauthenticated hardware/installable claims and unsupported statuses are
rejected. Allowed history statuses match the existing source validator's eight
states; a new schema/status requires reviewed runtime/source-validator changes.
These are presentation checks, not cryptographic authentication or firmware QA.

Both sections are constructed in detached document fragments. Visible lists
are replaced only after complete validation and successful construction; every
error path clears both sections. This also handles a synthetic exception on the
100th card, after otherwise valid input, without leaving partial visible data.
The R2 wrapping rules and all installer admission/race source remain unchanged.

Live Catalog source was checked before work at R26
`234cd59437137002565b22376b0d2a7a24e8053e` and again at R27
`876adf33d3a96accac3838c21f6f8681d8c1ab39`. Neither contains a runtime-history
repair to reuse: the runtime module remains the failing original. R27 changes
only its separate provenance validator/tests/receipt. No Catalog branch was
edited or moving R26/R27 data imported. The later merger must reconcile current
dependencies and independently recheck any different combined tree.

## Actual tests

`tests/history-response.py` was registered in the default `npm run test:browser`
command before changing the runtime. Its fixtures use the real response and the
five exact named mutations from independent evidence
`809965b4ddc4e2363e7344227adcf1b93f68005b`, plus the full-two-build schema
negative, type/status/claim/identity boundaries, transport failures, and a forced
card-construction exception. Positive controls cover one-build/117-row history,
actual two-build/117-row history at 320/375/768/1280, empty history, a typed
synthetic future append, and inert markup in a title. The append positive proves
presentation compatibility, not authenticity of a new source receipt.

- RED on unchanged R2 page: exit 1; **15/34 pass, 19 expected failures**,
  including all five named mutations, full-response schema and partial-DOM error.
- Native `npm test`, `npm run build`, `npm test`: all exit 0; all seven
  existing suites, 58 admission cases, 72 qualification negatives, 117 history
  pins remain green.
- Full default `npm run test:browser`: exit 0; **208/208 existing assertions
  plus 34/34 history-response cases**. No forged tag/download link, uncaught
  page error or partial cards in rejected fixtures. Positive records remain
  complete, readable and wrapped.
- The supplied `history_response_negatives.py` from the pinned independent
  evidence ran byte-unchanged: **exit 0, 8/8** (two positives, six negatives).
  Its fixed source path `/workspace/pr8-independent-qa` was supplied as a local
  symlink to this checkout; assertions and fixtures were not modified.

Logs accompany this file; receipt.json pins their SHA256 and tested source
blobs. Reproduce RED with
`git show b0d3c2b88587948a428305210d8b76ebd4d40535:docs/builds/index.html > /tmp/before-history.html`
then `python3 tests/history-response.py --source /tmp/before-history.html`.
Run the full native/default-browser commands on the new exact checkout for GREEN.
The independent script is at
`docs/independent-qa/pr8-history-followup/history_response_negatives.py` in its
immutable evidence commit above. All fixtures are browser-only; no device or
firmware qualification is claimed.

All 117 history rows, both historical builds, source/generated data, real empty
picker, source provenance, GPL/policy and installer/controller/slot libraries are
unchanged bytes. No firmware, controller, vendor assets or new download URLs were
introduced. No protected branch, merge, publication, scheduler or device change
occurred. Elite and Pro remain NO INSTALLABLE YET.

## Receiving stage

**REPAIR HANDOFF — PRIOR WEBSITE QA INCOMPLETE.** Keep PR #8 draft. Parent
must resume the distinct existing independent Website QA owner on the final
SHA/root recorded in the PR body. Independently inspect this runtime diff and
receipts, rerun native test/build/test, default Chromium and supplementary
fixtures; record an exact-head verdict covering F3/F4 and retained F1/F2 guards.
Only after acceptance may producer readiness and existing sole website merger
resume. A historical PASS or a successful schema check is not whole-site QA,
merge execution, firmware installability or publication. No successor was
created by this producer.
