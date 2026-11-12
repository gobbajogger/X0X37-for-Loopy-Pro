# F6a site and data implementation plan

> Execute natively in this session using Superpowers test-first checks and Compound Engineering review. The attached cx-F6a.txt is the accepted specification.

**Goal:** Complete the remaining site/data safety work for rows 51–55, 75–80 and 89, preserving the implementations already integrated into build/0.0.23.

**Architecture:** Reuse the existing Install/Samples/Projects workspace and exact .x0xslots/.x0xproject formats. Preserve revision-13-only device imports and retain read-only inspection of older files. No second editor, sample codec or install flow.

**Tech stack:** Browser JavaScript, Node host tests, Python site builder, Chromium browser validation.

## Pins and boundaries

- Base build/0.0.23: bb1907494b8d1b52d52ac220dfeb9a0663ea3ee2; exact tree e340d89d180879b8d8a2fea07c56800a2feaac5c.
- PR690: bc9a8b98a694f2efc6abfa3fe2bdfbc94924ecef. 26/29 changed paths already match the base exactly. Preserve the three newer B1 installer files; no redundant cherry-pick.
- Owned lane: wip/f6a-site-data; target one new PR into build/0.0.23. No merge or publication.
- Exclude web/offline-sequencer/, firmware implementations, MIDI connect/install sequence, .github/, bootloader, device writes and all other briefs.
- Commit author required by the current brief: gobbajogger <255244738+gobbajogger@users.noreply.github.com>.
- Required native builds: SDK V1.2.1, Elite and Pro, X0X_DEV_PADCAL=0. Record missing inputs and unmeasured RAM truthfully; 2352 B is the brief's baseline, not a new measurement.

## Task 1: Complete sample restoration

Files: web/x0x_breaks.js, web/test_breaks_page.mjs and existing sample regressions.

- Strengthen the existing page test with non-erased reserved header bytes and empty slots; observe failure before implementation.
- Preserve all 512 archived header bytes when the current device accepts them. Never silently discard reserved bytes or retry an ambiguous mutation.
- Verify every selected slot byte, unchanged original codec/rates/order, sparse/mixed capacities and corrupt/incomplete archives. Keep partial and complete outcomes distinct.
- Retain current pre-write recovery archive and stale snapshot/session rejection.

## Task 2: Enforce revision 13 at every import entry

Files: web/x0x_projects.js, existing project tests; Projects page only if needed.

- Add a regression proving legacy input cannot issue IMPORT/WRITE/ADOPT_BASS through direct API calls, including a tampered prepared import.
- Keep old-file inspection and backup bytes intact. Refuse migration/adoption before any mutating command; accepted revision 13 still round-trips songs, patterns, sound data and original samples.
- Preserve rollback, uncertain acknowledgement, corrupt bundle, stale preview, reconnect and multi-client tests. Document any changed positive legacy expectations as the explicit new policy.

## Task 3: Site acceptance and delivery

- Reuse the existing workspace proposal and shared CSS. Capture actual browser BEFORE/AFTER desktop/mobile PNGs and check docs/history/local-file errors and offline sample speed-up. Do not claim browser screenshots are 240×240 device framebuffer captures.
- Register new tests in the host suite or extend already-registered tests. Run the complete applicable web closure and site generation; report all failures and missing checks.
- Independent ce-code-review on immutable changed bytes, repair actionable findings, rerun affected checks.
- Create one branch/PR using the connected GitHub API, verify head/parent/tree and forbidden-path exclusions. Leave it open for owner review.

## External acceptance dependencies

Row 55's full historical personal sample restoration requires the owner's original complete archive and matching-device readback. Synthetic round trips cannot establish archive completeness. No Files/Library search/materialization tools are exposed in this thread; mounted library-files and shared directories contain no files. Preserve this as owner-dependent acceptance, not PASS.

## Review focus

Reserved header bytes; empty archived slots; changed device/connection during restore; legacy API entry points bypassing page guards; ambiguous mutation replies. No test checks may be weakened to hide failures.
