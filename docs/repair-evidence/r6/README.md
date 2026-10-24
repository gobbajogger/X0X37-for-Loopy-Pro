# PR8 R6 — local candidate output-parent revalidation

Bounded response to [publisher finding6385802554](https://github.com/gobbajogger/x0x37-for-Loopy-Pro_nightly/pull/8#issuecomment-6385802554). Parent/base: accepted R5 `3b626af4d0b0314c1349094699be14fc781c9bcc`, root `40d4faab200df125d901f58b08cbece9b7697c4b`. Independent R5 [source-only acceptance5638712179](https://github.com/gobbajogger/x0x37-for-Loopy-Pro_nightly/pull/8#pullrequestreview-5638712179) remains scoped to its exact root and listed findings; it does not independently approve this additional renderer change.

## Reproduction and correction

The registered qualification test executes the actual CLI from an isolated scratch copy of the source. The only stub is fetch, returning the existing synthetic package bytes. During its awaited Promise, the fixture renames a previously external parent and replaces it with a symlink into the scratch source's review-preview directory. This faithfully reproduces the publisher's approval-before-await/write-after-await case without writing into the real checkout.

RED on unchanged R5: stable-parent control exits0; rebound-parent case incorrectly exits0 and writes versions.json inside the scratch source. The new assertion fails. GREEN after repair: stable control still creates the exact catalog and installer; rebound case exits1 with outside-repository refusal, creates no destination, and preserves the source sentinel. Full original static boundary/projection/download tests still pass. See qualified-red.log and qualified-green.log.

The renderer now retains the canonical approved destination and revalidates the requested destination after download verification and HTML rendering, immediately before filesystem writes. It creates a temporary sibling output only after the await, generates the complete site there, revalidates again, and atomically renames the completed directory to the final destination. Generation errors remove only staging, never the user destination. Staging beside the destination keeps the rename on the same filesystem and preserves legitimate nested external outputs. No partially generated final destination is exposed.

Unlike holding an output/staging path across download awaits, this keeps the destination creation and publication in one synchronous section. It addresses the demonstrated local async window in trusted preparation tooling. It does **not** claim a sandbox against an arbitrary hostile same-user OS process mutating filesystem ancestors during individual synchronous syscalls. No new firmware eligibility, device identity or browser admission architecture is introduced.

## Validation and next stage

Affected qualification suite, full npm build/test and genuine default Chromium are executed; exact command exits/timings and logs accompany receipt.json. Browser/library/generated source and117 history records remain unchanged; the renderer's own installerFingerprint intentionally changes, so a later qualification must bind to the newly selected renderer root. Existing synthetic positive fixtures recompute it normally; no real qualification is manufactured or weakened.

Only renderer, existing registered qualification tests and this evidence change. Separate integration worker owns its pinned122-record four-file history composition; this branch does not import that moving data or overwrite its work. Parent obtains independent review on this exact new root before integration/sole merger/sole Pages publisher proceed. No merge, publication, firmware/controller/vendor bytes, device, network/security or scheduler action is performed.
