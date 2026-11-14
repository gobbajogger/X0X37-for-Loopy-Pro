# Independent DnBOX full-product review

Reviewed 2026-11-14, local candidate `http://127.0.0.1:8774/site/`, source `/workspace/dnbox-v4/site`. Product source was not edited. This review was given no prior score or target. It assessed discovery, musical proposition, installation, real tryout, projects/samples/presets, hardware MIDI and navigation.

## Judgment

The candidate explains a real replacement-firmware instrument: five simultaneous parts, independent pattern lengths/rates, probability and parameter locks, bass phrase generation, recorded knob movement, BREAK slicing, live mutes and SONG arrangement. The story connects those capabilities to evolving grooves and repeatable performance. The primary Install firmware action is immediate and unmistakable. This is credible product communication rather than a control inventory or a beginner lesson masquerading as the product.

It remains a stronger explanation than demonstration of the instrument's musical range. The only recorded session is an eight-second prepared mix. Different pattern lengths, knob motion, slice resequencing and song structure are static illustrations; they make sense but do not let a musician hear the transformation. Chicago and Miami are largely names rather than clearly differentiated sound-making characters. A compact explanation of Chicago's 303/filter/slide character and Miami's wobble role, grounded in the source, would add more value than more tutorial steps. Any richer audio comparison should wait for authentic captured evidence.

The main narrative is readable and scannable, but long on a phone. Repeated explanations of every static illustration's non-interactivity add weight. Retain the clear recording/illustration labels and put longer provenance explanations in details/About; trim repeated technical qualification from the product story. Keep the core progression: five-part instrument → evolving patterns and sound → performance → keep/install. This is a subjective refinement, separate from correctness findings below.

**Disposition:** suitable for bounded NIGHTLY website preview testing after the reproducible script error and contradictory recovery guidance are corrected and the changed qualification passes. This is not acceptance of a public playable emulator or connected hardware companion; neither is available through this candidate. It does not establish that a musician can actually try the instrument before installing it.

## Findings requiring fixes

### R1 — Home rendering throws, breaking heading focus/scroll completion (functional)

`experience.js:60` sees the new home `#session-audio` and dereferences absent `#audio-pause` / `#audio-mute`. Chromium reports `TypeError: Cannot read properties of null (reading 'addEventListener')`. This happens on initial home load and returning home. `app.js:62` consequently does not finish its scroll-to-top and heading-focus work. On return from Files, focus stayed on the brand link; on fresh home load it stayed on BODY. Native audio still plays, and later internal navigation works. Fix the binding to support the native-controls-only player, then verify initial home, returning home, gallery custom audio buttons, menu, audio playback and route focus.

Evidence: `crawl-results.json` (#home stack), `specific-results.json` (focus and audio observations). The 28-route crawl found the error only on #home.

### R2 — Recovery advice contradicts the working portable-project contract (accuracy)

Reachable `#help/recover`, options `app` and `partial`, says respectively “A project alone does not protect samples” and “A restored project does not restore BREAK samples.” Those categorical statements conflict with the new Files page and the existing project tool. `evidence/projects-live.html:126–127` has Include original samples checked by default; portable files preserve original sample bytes/rates/slot order. Lines 114 and 127 describe importing the displayed sample slots, with project-only files requiring matching samples. Distinguish portable projects from project-only files. A separate slot backup can remain recommended without denying the portable workflow.

Evidence: `specific-results.json` recoveryApp/recoveryPartial; current live project response hash matches the supplied HTML.

### R3 — BREAK diagram assigns playback imprecisely (accuracy, small)

The home diagram caption says “Import and slice playback happen in the sample tools and firmware.” The sample tool locally previews the prepared whole loop/encoded ADPCM and uploads slots; explicit slice sequencing/playback belongs to firmware. Say that allocation directly so visitors do not expect the sample web page to act as a slice instrument.

### R4 — Recorded-session relationship wording (accuracy, small)

“Chicago lands between Miami's repeated low notes” does not match the transcription: Miami is active on every step, Chicago on alternate steps. Describe the octave-above alternating pulse against Miami's repeated low notes, without implying interlocking onsets absent from the example.

## Product-area assessment

| Area | Observed result | Limit |
| --- | --- | --- |
| Firmware proposition | Hero identifies replacement firmware, SMK-37 Elite/Pro, standalone play, 909/808, Chicago/Miami, BREAK; subsequent sections connect musical operations. | Claims are based on pinned 0.0.25.1, not every currently installable build; page says so. |
| Installation | Header, hero and closing Install firmware links point directly to existing `/install/#firmware`. Internal install guide explains computer/browser/cable/model/backups and recovery. | External Chromium navigation hits environment certificate rejection. Verified-TLS HTTP returns 200 and exact supplied installer hash. No flash or MIDI connection attempted. |
| Genuine tryout | Hero says Browser emulator status; status page clearly says PUBLIC PLAYABLE TRIAL NOT AVAILABLE. Recording/gallery accurately described as recorded/static. | No genuine public playable trial exists in the reviewed route. #899 remains open draft at pinned head, not a deployed instrument. |
| Musical experience | Rhythm/part score, 16-vs-12-step example, recorded-movement curve, slice reorder and song form explain how parts can work together. | Only one short audio recording; no playable or comparative demonstration of the illustrated changes. Browser playback advancement was observed; acoustic output/quality was not independently measured. |
| Project/sample/preset files | Correct main distinctions: portable/project-only export; sample imports and slot backup; DNPR file-only, temporary session, no device transfer/save. Real project/sample tool links are obvious. | Recovery help has R2 contradiction. DNPR public deployment is unavailable and correctly not faked. #901 open draft head 7bece9b… verified. |
| Hardware MIDI | Separate page distinguishes physical sound engine, browser control, emulator and offline work. No fake Connect/mixer UI. Intended piano roll/drum/mixer scope is described as unestablished. | A usable public companion is absent. Contract-only support is not hardware qualification. Consider putting exact CC1/CC64 limits in a details/source link rather than leaving “control changes” broad. |
| Navigation/access | Menu keyboard Tab reaches visible first link; Escape closes and returns focus; internal route change closes menu and focuses H1 except home bug. Files and install are available in menu/footer/main paths. | Desktop browser with 390/320 viewport is responsive emulation, not physical phone testing. |

## Actual verification

- Real Chromium via Playwright (`require('playwright')`, `/usr/bin/chromium`), 1440, 390 and 320 CSS px.
- Full screenshots of home/try/files/companion/install/demo/learn/setups at each width (24 files). Inspected using `view_image`: desktop home, desktop installation guide, both phone home renders, phone Files, phone companion and narrow emulator page. No horizontal overflow measured in those routes.
- Crawled all 28 reachable internal routes at 320 px. No page-not-found journeys or horizontal overflow. One route had JS error: home (R1).
- Actual menu opening/Tab/Escape, mobile navigation to Files, native audio play (7.987664 s duration; currentTime advanced), recovery select options. Normal Files navigation ended with menu closed, H1 focus and no remaining audio element.
- Installer CTA clicked in Chromium. It reached the correct HTTPS target but displayed `NET::ERR_CERT_AUTHORITY_INVALID`; screenshot `install-live-desktop.png` records the warning, not an installer render. No certificate bypass, trust change, insecure context or hardware operation was attempted.
- Read actual current installer/project/sample/manual responses with default verified-TLS Python urllib. All returned 200. Installer/project/sample SHA-256 values exactly match supplied `live-routes.json` evidence. This proves route content availability through that HTTP client, not live Chromium operation.
- GitHub API confirms #899 open draft head `431d0db82cbadca761fb32c6548ca4765875324f`; #901 open draft head `7bece9b819790600bb755e2900817b249d875567` at review time.
- Source-checks used pinned pattern/motion/engine/UI excerpts and claim receipt. Additional hardware contract checked read-only at Git `e2d36dba34a2fed3bdb192d509687679674ccd7e`, `experiments/smk37-browser-contract-20261114/CAPABILITIES.md` (inspected firmware 94197fc…). Notes, CC1/64, bend and gated clock/transport are bounded source paths; no arbitrary mixer CCs or ACK/state editor. Existing entry point is a sealed fake-transport fixture, not a delivered public hardware companion.

## Reviewed identity

Complete SHA-256 list is `reviewed-source.json`; its own hash is `d8973e55920a1f871bd81717005b010ab45eb4a49d82e95a205b78ace514e140`.

| File | SHA-256 |
| --- | --- |
| index.html | cd8c1fb07c2d7e9cf06e5b475c9d9eda299f401d1b41c759169bbe683d6fdf23 |
| product.js | e92efa5a48d25e0847da0c7e8a5a316f0596cb5dc831b7999423de04291bd633 |
| product.css | 567c36505a506eb920462a9804f549595d11059cb9b66920a600136be07cf774 |
| app.js | a61959d444f22f90dc234a330d88d787ebc6734108ceb1bca0526a8b931c1352 |
| experience.js | 4b622cd8882ba82de86587752f98f771600b6087281d31d27a514a968bd03090 |

No firmware, emulator, codec, adapter, product source, live catalog/package, installer or deployed path was modified by this reviewer. Artifacts are confined to `/workspace/dnbox-v4/review`.

## Late status update

After the frozen-source pass, coordinator identified stacked W3 correction PR #915. A fresh verified-TLS GitHub API read independently confirmed it is open draft at `0472ca4ca75cf477be171554071f972f60dba88c`; response retained in `pr915-status.json`. The reviewed website still links #901. Update the preset status destination to include current #915 correction, while preserving file-only/no-device/no-deployment language. This is a factual freshness change to recheck; the newly corrected codec was not executed by this website reviewer.
