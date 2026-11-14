# SONG pattern continuity: actual emulator and source check

The corrected arrangement recipe preserves the displayed common and mixed pattern choices. The former F3-twice recipe demonstrably replaces them with P1. No remaining mechanism defect was found in the released arrangement recipe.

This is an immutable 0.0.25.1 Pro firmware run in the existing fm1-web CPU emulator, through the unchanged qualification harness at `http://127.0.0.1:4322/qualification.html?model=pro`. It is not a physical instrument check, a live website emulator qualification, or a 0.0.26 check. No firmware, emulator, website source, samples, real MIDI, certificates, Git remotes or deployment state was changed. The only messages sent were emulated physical keys, pads and encoders. No new WAV or musical asset was produced. PNGs are actual unaltered LCD canvas screenshots.

## Exact inputs consumed

- Firmware source revision: `79280e763341d03ba43266d3e25dad66f12de860`.
- `fixture-pro.fwsc`: 651721 bytes, SHA256 `6f135a3d03bb1a2a4d09d63040c719313612b85590a6af69b293a928e22285bf`.
- `fm1_web.wasm`: SHA256 `8f053c488056eed4d3f71d77901ef796e42e055f64eb04fc71a476dd0b58c33d`.
- Source read: `/workspace/website-v4-inputs/DnBOX-v4-product-and-evidence-20261114/DnBOX-v4/evidence/ui-79280e76.c`.
- Existing capture/control methods read: `tests/record_evidence.py` and `tests/record_additional.py`.
- Released recipe consumed for mechanism qualification: `site/journeys.js` SHA256 `4bc40a06d57697842f45e4269ac30a643b1a65bdb213d84ce33e69ade57a0950`.
- Final guide re-read after the writer added displayed-value and actual-marker wording: `site/journeys.js` SHA256 `1f4075e2887b0f9b1c1af017c9fb84f4f35410a1e896fd251fa402ee53f5f6e5`. These wording changes do not alter the qualified mechanism.

The source, harness and initial guide byte receipts are in `receipt.json`; final guide and selected results are in `qualification.json`. `manifest.sha256` covers all evidence files. `verify.py` contains the controller; capture JSON contains the exact emitted message history with host and emulated timestamps. The harness records audio RMS/peak measurements; its `frames.playing` field describes runtime state and was not used to infer the musical transport.

## Observed results

| Case | Result | Actual observations and proof |
| --- | --- | --- |
| Original P5 to P1 failure | PASS, defect reproduced | HOME, first bank, C4 selected common P5. Four 909 kick steps were enabled. `02`/`03` show 909 P5 and active playback; measured peak 0.236084. `04` shows an empty SONG with P5 controls. F3 twice made both rows P1 in all five columns and selected END row 3 (`05`). The resulting P1 SONG output had peak and RMS zero (`06`). |
| Mixed choices retained | PASS | Actual SEQ choices were 909 P5, 808 P5, BREAK P7, Chicago P11 and Miami P5 (`30`–`35`). These values were read from the LCD; they were not inferred from intended encoder counts. First-page 909 away/back created row 1 and restored P5 (`36`/`37`). Second BAR page displayed Chicago P11 and Miami P5 (`38`). Its K6 moved only Chicago, then restored it (`39`/`40`). K5 selected END row 2 (`41`); first-page 909 away/back created that row and restored P5 (`42`/`43`). Both rows contained `[P5,P5,P7,P11,P5]`, with END on row 3. |
| Mixed BREAK bar mute | PASS for actual assignment/mute/transport | A#3 while displayed BAR 2 changed only row 2 BREAK to `-`; its lower control retained P7 (`44`). Row 1 retained BREAK P7. The second page retained Chicago P11 and Miami P5 (`45`). Starting from selected BAR 1 (`46`) produced actual SONG playback (`47`–`49`): the position marker and row play triangle showed bar 2 while selected BAR remained 1. Mixed output peak was 0.326263; no harness errors. BREAK had no newly entered steps in this structural scenario, so these captures do not claim an audible BREAK-only drop. |
| Clean common P5 and audible Chicago target | PASS for actual assignment/mute/transport | Fresh boot, HOME first bank and C4 established common P5; 909 four kicks and Chicago four notes were entered (`60`). Empty row 1 (`61`) was created by displayed 909 away/back (`62`). K5 selected END row 2 (`63`); the same creation gesture produced two all-P5 rows and END row 3 (`64`). C#4 on displayed BAR 2 muted Chicago only there (`65`); selected BAR 1 retained Chicago P5 (`66`). Actual SONG playback showed the row-2 play triangle and position marker 2, with Chicago muted only in row 2 (`67`/`68`). Output was nonzero (peak 0.310608), with no harness errors. |
| Encoder-count assumption | FAIL for the assumption, resolved by guide wording | Raw `clicks=1` sometimes caused no displayed change; raw `clicks=4` could skip several pattern/bar values. Attempt `10`–`26` therefore did not establish the intended two-bar result and is not used as positive evidence. In particular, muting an accidentally selected END row created an unintended third bar. The final guide expressly requires reading displayed BAR/P numbers, matching both rows, and checking END on row 3. The clean positive used displayed-value feedback. |

Screenshots are named with the numeric prefixes used in the table. Their sibling JSON files retain cumulative exact inputs and renderer measurements for that boot. Earlier unsuccessful attempts are retained rather than relabelled as a pass.

## Source-proven implications

- `ui-79280e76.c:97`: visible part order is 909, 808, BREAK, Chicago (`PART_303B`), Miami (`PART_303A`).
- `:509`: `song_fill` makes the first bar inherit current per-part `seq.ppat` values; subsequent bars copy the preceding whole bar, including mutes/repeats. The guide's explicit mute checks therefore matter.
- `:657` and `:800`: per-part BAR controls read/edit the selected bar; an empty/end row initially displays inherited pattern assignments, and an actual assignment change creates it. These edits do not advance the selected bar.
- `:1547`: BAR page 1 maps K5 to bar, K6/K7/K8 to 909/808/BREAK; page 2 maps K5 to bar and K6/K7 to Chicago/Miami. The page-2 K6 operation was also observed directly.
- `:3650`: a SONG white key assigns its bank-relative pattern to all five parts and advances to the next bar; a black key toggles only the corresponding part's mute. This matches the observed old failure.
- `:3976` and `:4471`: PATCH moves HOME to SONG; PLAY from stopped sets SONG playback only when SONG is visible.
- `:5975`: the table's triangle marks the actually playing row, independently of the selected editing row. A muted table column is drawn as `-` while its underlying pattern remains assigned.
- The supplied independent-assessment source snapshot's `firmware/src/seq/sequencer.c:495` explicitly loops the SONG to row 1 when its next row reaches song length. Its consumed file SHA256 is `3f28ca5c772a51e04dfd854b305199adbfe4a8bfea39aee4e7ab6c6a5f8d16e7`; that snapshot's association with firmware `79280e76` is recorded by its existing `ASSESSMENT.md`. This is source evidence for the return, separate from the actual row-2 playback stills.

The screenshots directly establish intended assignments, bar structure, selective mutes, nonzero renderer output and actual SONG transport. Return to the unmuted first row follows the two-bar source behavior; the timed playback still captures happened to land on row 2. No isolated listening or PCM A/B proof is claimed for a particular part's entry/exit.

## Limits and unfinished checks

Physical Elite/Pro behavior, device persistence, hardware sound quality, live/public delivery, and 0.0.26 are NOT_RUN. The supplementary clean P6/P7 setup (`50`–`56`) had already started when the parent narrowed the scope; its SONG continuity was NOT_RUN and it does not replace the completed P7/P11 mixed check. The existing nine musical recordings were unchanged.

Two wording observations were sent to the sole website writer and are reflected in the final consumed guide: read displayed numbers instead of counting turns, and refer to the actual song-position marker rather than a literal S. Source ownership and publication authority remain with their assigned owners.
