# W3 #901 workflow review: proposed UI, pending owner approval

**Status: DESIGN / REPRODUCTION ONLY. Do not treat this as implementation or authorization to merge/deploy.**

- PR: https://github.com/gobbajogger/X0X37-for-Loopy-Pro/pull/901 (draft, existing W3 lane)
- Reproduced application head: `0ecc8502a0ee71dc086397627c809d6b35839c42`
- Exact app blob: `web/presets/app.mjs@2e7e0c507ad773a3a70eea4de9c2bc77cb4b6972`
- Exact page blob: `web/presets/index.html@b59a553e597171dc267967a9ea202b45be7bb2ad`
- Executed in isolated DnBOX cloud scratch with Node v24.22.1, Chromium 156.0.8078.79, Puppeteer, HTTP bound to localhost. Scratch probe: `/workspace/scratch/w3-901-warning-3324770/diagnose-next.mjs`.
- Existing baseline **reused, not rerun**: Node 18/18 and Chromium 19/19, source evidence in `web/presets/evidence/2026-11-13-multi-record-preview.md`.

## Current exact-head reproduction (read-only, no product changes)

1. Mixed user/factory ZIP with three valid DNPR entries can be explicitly Kept in browser session, but `Download session .zip` fails with `FACTORY EXPORT OFF` even though two entries are eligible for export. The user has no eligible-only ZIP export action. The fail-closed factory block **works as designed**; the missing operation is what is under review.
2. A real 32-record ZIP produces 32 preview record blocks and 14 indexed warning groups with no horizontal overflow. The first warning for record #2 is displayed **15,391 CSS pixels after the end of the corresponding metadata record**, because all warnings occupy a separate container after the 32 metadata blocks. Completeness improved in the prior fix, but locality at capacity is inadequate.
3. Real Chromium keyboard `Enter` on focused Keep and Discard hides the focused control. `document.activeElement` becomes `BODY` in both cases. After Keep there is a newly selected session record; after Discard the user can choose files again, but focus is not placed on either target.
4. The diagnostic returned process exit code **0**. These are observations about current behavior, **not passing tests for a fix**. Exact recorded terminal output:
   - `CURRENT_32 {"previewRecordCount":32,"warnGroupCount":14,"gapFromRecord2ToWarning1":15391,"scrollHeight":20412,"horizontalOverflow":false}`
   - `FOCUS_KEEP {"activeId":"","activeTag":"BODY","hiddenKeep":true,"sessionButtons":1}`
   - `FOCUS_DISCARD {"activeId":"","activeTag":"BODY","hiddenDiscard":true}`
   - `BEFORE_EXPORT {"count":3,"zipEnabled":true,"zipText":"Download session .zip"}`
   - `MIXED_EXPORT {"status":"FACTORY EXPORT OFF. FACTORY EXPORT OFF: Factory and factory-derived records cannot be downloaded No browser-session entries or keyboard data were changed.","eligible":3}`

## Proposed presentation: owner approval required **before** UI implementation

**Recommended A: inline-record warnings and explicit eligible-only download.** Preserve the existing dark-olive/amber DnBOX style and existing all-record, atomic Keep. In the 32-record Review list, each numbered record appears as a compact row, with its engine, export eligibility and any warnings immediately below its own name; extensive metadata is disclosed via an optional `details` region. Warnings must remain visible without opening that region, for all warning-bearing records. The list itself can scroll, but the Keep/Discard actions and batch counts stay readily accessible after it. No import-stage checkboxes are proposed: the user still either explicitly Keeps every validated record or discards the preview.

In Browser session, show per-record badges `EXPORTABLE` or `VIEW ONLY`, plus a labeled action `Download N eligible .dnpr files as ZIP`, accompanied by `M factory-derived excluded`. Eligible records retain exact original DNPR bytes and existing safe unique-name handling. A factory-derived entry remains viewable, never downloadable, and never included in the newly generated ZIP. For zero eligible records, disable the ZIP button and explain why. The selected-file download remains available only for an exportable record, with visible explanation for factory selection. Do not rename provenance or pretend that unknown/imported provenance is authenticated; the existing `isFactoryDerived` / `ensureExportable` policy is authoritative.

Keyboard focus: on successful explicit Keep, focus the first newly selected session record after its button has been rendered; on Discard, focus the existing file picker without opening it. Avoid focus placement for unrelated model/engine changes or validation errors. Keep the existing aria-live status messages. Maintain mobile 390px/320px and 44px minimum touch targets.

**Alternative B (not recommended):** keep a separate warning summary with anchor links back to each metadata row, and provide a separate filtered ZIP dialog. It has more navigation and extra dialog state; 32-record warnings are still far from their context, while consent is less obvious.

### Interaction wireframes (illustrative only)

```text
02 / Review                                         32 validated
[ 32 checked | 14 warning-bearing | 4 view-only ]
| #01  Owner Chicago    Chicago          EXPORTABLE     [Details v] |
| #02  Kit 909          909              EXPORTABLE     [Details v] |
|      ! Sample references not checked against keyboard.           |
| #03  Factory Miami    Miami            VIEW ONLY      [Details v] |
|      ! FACTORY EXPORT OFF. Inspect only, never export.           |
| ...              scroll within record list, up to 32             |
[Keep all 32 in browser session]      [Discard preview]
```

```text
03 / Browser session                              3 retained
Owner Chicago      Chicago          EXPORTABLE   (selected)
Kit 909            909              EXPORTABLE
Factory Miami      Miami            VIEW ONLY    (never export)
2 eligible files; 1 factory-derived file excluded.
[Download 2 eligible .dnpr files as ZIP]
[Download selected Owner Chicago .dnpr]
Keyboard: Keep -> selected session entry; Discard -> file picker
```

The visually rendered desktop and mobile mockups are supplied **in the owner's chat**, not as binary screenshot files in Git. They are proposal illustrations, not app screenshots.

## Focused verification to perform only after approval

- **Mixed ZIP exports:** 2 user + 1 factory yields exactly 2 original DNPR byte arrays in a ZIP after an explicit click, preserves 3 session entries and never emits factory-derived bytes. Factory-only session offers no ZIP, selected factory no single-file download. Duplicate eligible filenames retain collision-safe names. No extra download on preview or Keep.
- **32-record warning locality:** record-#2 sample warning is a descendant of record-#2 summary (not a detached global warning block), factory warning on later records remains next to its item, and each per-record warning is accessible even when details are collapsed; labels/counts correct with zero overflow at desktop and 390/320px. Preserve atomic preflight and Keep-all behavior.
- **Keyboard focus:** real Chromium keyboard Enter on Keep focuses the selected visible session-record button; keyboard Enter on Discard focuses file input; subsequent Tab reaches sensible next controls, with no hidden active element. No artificial file-picker opening.
- **Safety invariants:** file-only (no network/upload/MIDI/USB/device writes/storage), existing `ensureExportable` factory guard called for every included ZIP entry, explicit consent, original bytes, no native/host firmware files changed.
- Scope verification to **new/changed behavior**, reuse existing Node 18/18 and Chromium 19/19 evidence; independent reviewer gets exact-head test receipt and negative-control reproduction after implementation.

**Approval gate:** no W3 source, index, DOM harness or Chromium test implementation until owner approves this proposal. No merge, deployment, device transfer or publication.
