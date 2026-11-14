# Independent full-candidate review

Reviewed 14 November 2026. Candidate: `site/`, manifest SHA-256 `175c2936f8287bb364e3afb1fbea6f5caf3380be6036f7e160e742fde4810d3e`. All 68 listed file hashes matched at intake. Website source was read-only throughout. No earlier assessments, scores, root plans or QA reports were consumed. This is an independent product and browser assessment, not firmware qualification.

## Overall assessment

The site makes a convincing case for DnBOX as a standalone instrument: five parts sharing a clock, bass phrases that change their relationship with the drums, loops reshaped into phrases, motion and repeatable arrangements. Actual recordings now support those ideas. The short next-task pages connect the first beat to useful musical decisions without turning the product page into a long manual. The restrained dark layout, actual firmware imagery, readable narrow layout and physical key/pad diagrams work together well.

Five actionable defects remain: two teaching issues and three history-restoration issues. Fix those before treating these tasks and navigation as complete. I found no browser crash, invented live instrument, manipulated LCD, mismatched recording file or false claim of completed hardware qualification. Public emulator and hardware companion availability remain substantive unfinished deliverables, accurately disclosed here; this review does not close them.

## Actionable findings

### F1 — P2: make the transport and recording toggles conditional on their starting state

Location: `site/journeys.js:8` and `site/journeys.js:9`, rendered `#learn/break` and `#learn/movement`.

Reproduction: follow the BREAK page while the existing four-kick pattern is already playing, which fits its instruction to keep the drums steady. It unconditionally says “Press SEQ PLAY.” In the exact 0.0.25.1 source, `button_tap(B_PLAY)` stops an already playing sequence (`ui-79280e76.c:3943–3955`). The promised changing loop against a steady kick becomes silence, and the proposed silence checks omit the stopped transport. The bass page already uses the correct “if stopped” condition.

The movement page similarly starts with a running Chicago pattern but does not require REC off before saying to press SEQ REC. If REC is already on, the first press turns it off, knob changes are manual, and the second press turns REC back on. The page then asserts that recording is off and that recorded motion should return. `ui.rec = !ui.rec` at source line3971 and the `motion_touch()` branch at2758–2773 establish this outcome.

Recommended correction: “Press SEQ PLAY if stopped” for BREAK; explicitly start the movement task with REC off and check its state after recording. These are brief prerequisites, not additional tutorial sections. The page instructions were reproduced in the actual browser; the device-state consequence is established from the exact original source, not a physical-device test.

### F2 — P2: establish the first pattern bank before assigning Pattern 1 in SONG

Location: `site/journeys.js:10`, rendered `#learn/arrangement`.

Reproduction: enter SONG with pattern bank17–32 selected, a normal state left by OCTAVE+ or choosing a higher pattern with K4. The task says to tap F3 twice to put Pattern1 in two bars. Actual source computes `wp = ui.pbank * 16 + w` (`ui-79280e76.c:3632`) and writes that pattern to all five parts of each selected bar (`3650–3665`). F3 therefore assigns Pattern17 when `ui.pbank == 1`. The user may arrange an empty or unintended pattern and hear no promised alternating BREAK.

Recommended correction: select patterns1–16 with OCTAVE− on HOME/SONG, select bar1 on the BAR page before writing, and check the resulting table shows P1 on rows1 and2 with END after them. This also makes the assumed initial song cursor explicit without adding a separate checklist. Browser reproduction confirms the missing prerequisite; the different assignment is source-proven, not hardware-tested.

### F3 — P2: restore disclosure state before restoring scroll position

Location: `site/navigation.js:20–31` and page re-rendering.

Open “Let the phrase drift” on `#learn/bass` at390px, follow “Give the line movement”, then Back. The details element returns closed because `render()` rebuilds it from default markup. Its content and resulting height disappear before the saved scroll is applied. The user returns to different content and can lose the point they were reading. `history-edge-cases.json` and the retained screenshot show the closed disclosure after Back. Store disclosure state per history entry and restore it before focus/scroll. Reproduced independently in Chromium after the source owner flagged this class of edge case; no prior assessment was read.

### F4 — P2: capture focus changes that occur without scrolling

Location: `site/navigation.js:6–16` and event listeners.

Navigate from the first-beat page to Save, scroll the save-message field into view, click it and close its picker with Escape. Its focus is now `save-state`, but the recorded history entry still has `focus:null`: no scroll followed the focus change, and non-link clicks do not call `rememberRoutePosition()`. Back then Forward returns focus to BODY, although the scroll position is restored. Capture focus changes as well as scroll/link exits. Actual native select clicks, history traversal and state receipts are retained in `history-edge-cases.json`.

### F5 — P2: initialize history entries created by direct fragment changes

Location: `site/navigation.js:18–24,51–56`.

From the homepage, make a native same-document fragment change to `#learn/bass`, scroll, follow its movement link and go Back. The fragment entry has `history.state:null`. The browser emits popstate before hashchange; `showRoute('history')` renders and updates `renderedURL`, so the hashchange handler returns without creating `dnboxRoute`. Subsequent scroll snapshots cannot save a missing entry. Back lands atscroll0. This was reproduced with the browser's native fragment navigation, not a custom routing mock. Initialize a missing route record before processing popstate/history rendering. Details are in `history-edge-cases.json`.

## Recording-by-recording assessment

These are **host-rendered outputs of actual firmware in the preserved emulator**. They are neither physical-device recordings nor live emulator controls on the website. Browser playback worked for all nine at320px; only the selected player continued, and leaving the route removed/stopped the players. All nine published WAV hashes match the accepted evidence. Every PCM sample matches the documented clamping/rounding conversion from preserved float PCM; all inputs are finite and none of the PCM16 files clip. The recorded phase counters have zero short-buffer delta and no reported errors. This verifies these excerpts, not the runtime's broader realtime performance.

| Recording | Claim assessed against actual evidence | Judgment |
|---|---|---|
| 808-groove,6.08s | Original events select808 BD/SD/CH and set1/5/9/13,5/13,3/7/11/15. Actual LCD shows those rows and positions. PCM has the recurring drum/hat transients. | Supports the isolated808 groove description. It does not claim other parts or hardware. |
| parts-performance,12.47s | One raw PCM span; original HOME key10 adds BREAK near3s, key13 removes Chicago near6s and restores it near9s. PCM spectrum becomes much denser when BREAK enters; actual endpoint HOME shows the intended active parts. | Supports the continuous enter/drop/return description; no concatenation is implied. A dense BREAK masks some bass detail, so this is a demonstration of arrangement gestures rather than an isolated bass comparison. |
| length-before,5.02s | Original909 and Chicago entry events, actual16st sequence, notes1/4/7/10 atA2. PCM has the repeating bass attacks against the kicks. | Supports the stated baseline. |
| length-chance,8.10s | Actual LCD reads13st; separate held-step capture explicitly reads STEP10:70%. Events change K5 length and held-step K2 chance. PCM has the shifted bass attacks and gaps against continuing regular kicks. | Supports the13-step/chance transformation. The file does not isolate chance from length or establish a measured70% hit frequency, and the copy does not claim either. |
| motion-record,9.20s | Actual pre-capture LCD highlights FILTER1, with CUTOFF under K5. Original REC on, twelve K5 movements, then REC off; captured PCM continues for about4s after REC off. Actual endpoint is still FILTER1. | Supports recording and replay of cutoff movement. This is a subtle filter example over drums and the existing chance pattern, not a dramatic standalone sweep or a controlled acoustic measurement. |
| motion-takeover,8.43s | Same running setup, a real K5−16 event with REC off, continuous PCM, and final unchanged LCD showing cutoff31% after the previous recording. The original UI routes unrecorded touches of an existing lane to `engine_motion_hold()`. | Supports the bounded manual-takeover/return interpretation. Exact within-pattern takeover/resume timing has not been independently instrumented by this review; no physical timing or acoustic qualification follows. Separate excerpts are labelled correctly. |
| break-straight,6.13s | Actual LCD shows GROOVE0/0/0/0 and all16 steps; original events mute all other parts. PCM has a stable repeating loop-derived pattern. | Supports the zero-groove firmware-playback baseline. Correctly distinguished from the original source WAV. CRAMEN identity/provenance is inherited from the qualified slot fixture, not inferred from the LCD’s BR1 label. |
| break-phrase,8.03s | Same setup, actual LCD80/0/100/100, real parameter changes and new PCM. Transient placement differs from the straight take. | Supports a generated BREAK phrase from the same loop and enabled steps. No invented source sample, exact slice recipe or live remix is claimed. |
| song-two-bars,9.96s | Actual pre/post LCDs show two rows using P1, Chicago absent in row2, END atrow3, selected startbar1 and playing bar2. Original events assign both bars/muteChicago/start. PCM retains regular kicks and alternates bass-rich passages with drum-only passages. | Supports the two-bar arrangement description and repeated structure. It demonstrates Chicago mutes; the separate written task teaches BREAK mutes and does not label this capture as that task’s exact result. |

The source-backed states and preserved output support all nine descriptions within the limits above. I did not perform human/speaker/acoustic listening and do not call these recordings sound-quality or performance qualification. The analysis image is an independent waveform/spectrum inspection aid, not replacement audio.

## Installation and version guidance

The backup warning appears directly beside the homepage install actions and the install-page CTA at all three widths. It explicitly says NIGHTLY may continue after a sample-backup failure and asks for independent readable project/sample backups. No installer changes are implied.

Normal certificate-verified Python HTTPS requests fetched the live installer, manual and sample tool. The installer currently identifies corrected0.0.26 source94197fc8, hardware startup/restart unverified, original600b withdrawn. Its embedded text says sample backup cannot block installation and can continue after failure. The public manual still says0.0.26 is withdrawn. The website’s version page accurately explains this discrepancy and links the exact97a88b44 draft manual; that draft identifies94197fc8 and separates0.0.25.1-only support. The live sample tool also supports the44.1/22.05kHz choice, original/encoded comparison and capacity-limited workflow described here. No tool source was edited.

Actual Chromium navigation to the installer stopped at `NET::ERR_CERT_AUTHORITY_INVALID`. No bypass or trust change was attempted. Live picker operation, MIDI permission, file import/transfer, hardware install, backup restore and boot remain unexecuted. A guessed root catalog URL returned404; the installer’s actual embedded catalog was inspected instead. This is a verification limit, not evidence that a linked product route is broken.

## Browser coverage and limits

- Chromium154.0.8037.57, local HTTP at127.0.0.1:4321. All35 linked routes traversed; images loaded and no page errors. Screenshots retained for desktop and narrow key pages. All35 have zero document overflow at1440/390/320px.
- Fourteen retained file/install aliases and six malformed routes checked separately. Aliases resolve to the current product guidance; malformed paths show the useful not-found page. This is49 valid route forms plus six invalid forms, not a claim to reuse or rerun another reviewer’s route count.
- Initial focus stays BODY; first Tab reaches Skip to content and second reaches the brand, preserving navigation. Intentional internal navigation focuses H1 atscroll0. Ordinary internal-link Back/Forward restores each recorded scroll position and the outgoing link focus. Exact position comparison at all three widths is in `navigation-position.json`; disclosure, focus-only and direct-fragment exceptions are the actionable findings above.
- All five teaching routes manually stepped to the end; Previous/Repeat/Play/Escape exercised where applicable. Reduced motion disables autoplay and retains Next. Atlas exercises include fourSEQ traces, sound-mode display, all16 bass-pad choices and all11 drum-row choices. Ten save explanations and eight recovery situations exercised. Both silence forks work. Mobile menu opens, Escape restores its button, and menu navigation focuses the new heading.
- All nine new audio players started from actual native-control clicks at320px, advanced time without media errors, and paused the preceding player. Source route changes pause audio. The older recorded mix/gallery were visually inspected; this review does not independently requalify its historical provenance beyond the supplied unchanged constraint.
- This review did not run firmware builds/host tests, edit runtime/DSP/codec/importer sources, write to a device, use network Git, deploy or modify certificate trust. No source defect was repaired during the frozen review.

## Evidence

`manifest-before.json`, `manifest-after.json`, `routes.json`, `route-text.txt`, `interactions.json`, `extra-checks.json`, `navigation-position.json`, `history-edge-cases.json`, `recordings-analysis.json`, `recordings-analysis.png`, desktop/narrow screenshots and reproducible browser/analysis scripts are in this directory. External verified-HTTPS source copies and fetch receipts are retained separately from the frozen candidate. Public source links: [installer](https://gobbajogger.github.io/x0x37-for-Loopy-Pro_nightly/install/), [public support manual](https://gobbajogger.github.io/x0x37-for-Loopy-Pro_nightly/manual/), [pinned SMK draft](https://github.com/gobbajogger/X0X37-for-Loopy-Pro/blob/97a88b44539110ded82257ebb20b0444ba4b8b8f/docs/MANUAL.md).
