# Browser admission repair — source handoff, not publication

This isolated repair consumes website PR #7 `33252473b8725fc124cd20d85f780a85c4f6d4a0`
(including PR #6 `ddbd8d89f7c4dfd5afa432dd51e940b6dd428592`) and selected, unchanged
catalog/history/test files from PR #3 R25 `2d4134b1922079adfd346361758b3ed86b01e638`,
root `727c5cab036769f2cc631cbe196e630c57719033`. Current website main policy is
preserved. The union of all seven original npm suites and `prepare:candidate` is
registered. All 117 history rows and both historical build generations remain;
the source and generated picker still have zero candidates.

The verified-Blob, exact response-size, same-selection and synchronous writer
patterns selectively adapt GPL browser code from firmware PR #102, fetched head
`22d522a4cde4c18f92a846bc40d504b701cf6d17`; attribution and source pins remain in
`SOURCE_PROVENANCE.json`. No firmware, controller, vendor or private bytes were
copied. This is producer testing; independent QA approval is still required.

## Admission contract

A browser download is a Blob made only from the exact response that passes
size, SHA-256 and exact `_900` Elite/Pro product checks. The remote package URL
never becomes the Download link. Download and Install recheck the same version,
model, package object and metadata snapshot. Selection replacement, an empty
catalog, mutation or explicit revocation invalidates cached permission. Serial
and identity checks suppress obsolete fetch, body, digest and scan callbacks.

Both renderer and browser reject `revoked`, `blocked` or `withdrawn` unless
absent or strictly boolean false; `qualified:false`, `installable:false`, failed
model status, Release imports (including string bypasses), and self-declared
physical proof are rejected. Candidate lifecycle must be absent or `current`.
A superseded history record is not a selectable candidate: retain the unchanged
last safe qualified row when a new attempt fails, and put failed/superseded
checkpoint records in read-only history. Relabeling a candidate requires new
review. No live candidate was removed by this source repair.

URLs must be canonical, digest-addressed HTTPS without user credentials, query
or fragment. DEV model/controller SHA and bounded exact size are mandatory.
The separate renderer continues to require complete source/tree, model,
controller, QA, safety, licensing and hosting receipts; schema shape is not
independent verification of those receipts. A hardware-test label supplies no
physical PASS.

DEV and local-file paths share a writer lock acquired before MIDI permission.
DEV selection is checked again after permission and device lookup. Permission
denial, synthetic interruption or controller rejection permits retry. Existing
slot-preservation code is retained. Real OTA/slot/controller safety and lawful
immutable hosting remain separate qualification gates.

## Reproduction

Run `npm run build`, `npm test`, then `npm run test:browser`. Browser tests need
Python Playwright and `/usr/bin/chromium`; this execution environment supplied
both. Chromium tests use the actual candidate renderer and the complete page,
real browser SHA/package parsers and Blob downloads. Test-only instrumentation
exposes module state and mocks fetch/MIDI/installation; it is never emitted by
the production renderer. Synthetic bytes and receipts are not firmware or a
qualified downloadable build. Uninstrumented empty generated pages also run at
320/390/1280 pixels. Candidate flows run at all three widths, EN/JA, Elite/Pro.

For RED reproduction, extract
`git show 33252473b8725fc124cd20d85f780a85c4f6d4a0:web/x0x_installer.html > /tmp/before-installer.html`
and run `python3 tests/browser-admission.py --source /tmp/before-installer.html --red-only`.
That deliberately exits 1 on unsafe admission assertions. Add `--race-only`
to reproduce the original two-permission/two-installer reentrancy failure. For the renderer RED,
extract that same PR #7 tree to a temporary directory, copy the current
`tests/qualified-catalog.test.mjs` into it, and execute that test: the first
revocation negative fails because the old renderer accepts it. Current source
must pass both suites. Checked logs and a machine-readable packet accompany
this document under `docs/repair-evidence/`.

## Next owner and remaining gates

The parent coordinates one existing independent website QA owner. Refresh PR
heads and reviews, independently fetch the repair's exact commit/root, inspect
the combined diff, run the commands above, and record an exact-SHA verdict.
Only the sole website Main Merger handles reviewed integration; only the sole
Pages publisher handles deployment and served-byte verification. Preserve
catalog/installer test registration during integration. Do not treat this draft
as approval of another PR or of firmware packages.

Firmware compilation, actual controller/OTA/backup/restore acceptance, lawful
immutable artifacts, deployed HTTP byte checks and physical devices were not
part of this source stage. Elite and Pro remain NO INSTALLABLE YET. No main,
nightly, gh-pages, firmware PR #121, tag, release, scheduler or device was changed.
Parent owns downstream task creation; this stage creates no competing owner.

R2 continues this repair after independent review found malformed-selection
Download cancellation and actual-history overflow defects. The implementation,
RED/GREEN logs, 208 registered Chromium assertions and unchanged independent
82-case replay are recorded in [the R2 receipt](repair-evidence/r2/README.md).
The PR remains draft pending a new independent exact-head verdict.

R3 responds to supplementary history HTTP-response findings F3/F4 that superseded
R2 overall source acceptance. PR #8 is draft again. Supported-schema/status/type
validation and detached atomic rendering now fail closed; see
[the R3 repair evidence](repair-evidence/r3/README.md). Independent full-head
re-review remains required before readiness or merger dispatch.
