# Owner glyph rule follow-up

Base for this follow-up: PR #800 at `97282d7fed8859f337d606d4880cb095b1ce2591`. The owner requests pictures, glyphs and small animations wherever they convey the same meaning, with real-framebuffer before/after approval in the original brief. The later owner instruction on 2026-11-01 removes previews and any visual-approval wait; the Orchestrator verifies on the box. No builds or tests run in Codex.

Later owner amendment: [held-FX arrow mapping](FX_HELD_ARROW.md) adds effect-specific pictures to the top bar and changes the mapping gesture. That amendment supersedes the no-routing-change statement below for row 41, adds 198 B to the constant atlas (666 B total), and documents its 4 B per-peer ownership-state reason. The earlier source review and 468 B figure below describe the original glyph-only follow-up.

## Design and implementation plan

Reuse the existing play triangle, clock-owned pulse, queued-cue blink, strip geometry and compact pattern/control motifs. Add a shared stateless nine-pixel pictogram atlas for the popup's smaller cells. Its 9x9 glyphs fit inside the existing 22x11 buttons; larger footer glyphs use integer scaling. The atlas is constant data, with no new timer, mutable static state or BSS.

| Existing text | Visual treatment |
| --- | --- |
| PREV / NEXT | Direction arrows in clip-stage buttons. |
| R / M / S | Record dot, crossed speaker and headphones; resulting on/off state has a positional toggle as well as colour. |
| HOME / MIX / SEQ / SONG / GLOB / LIST | Roof, fader rails, step grid, linked bars, globe and list. |
| SEL | Selection brackets, distinct from the confirmation/acquisition check. |
| LOCK / KNOBS / FADER / KEY | Padlock and matching control pictograms beside the retained context, bank and strip numbers. |
| PLAYING / STOPPED / RECORD ON/OFF | Transport/record picture and explicit resulting state. |
| PICKUP: MOVE UP/DOWN TO TARGET | Direction arrow plus target marker. |
| ACTIVE / USE FADER | Acquired check, or fader picture plus destination number. |
| Top-bar P1 >2 | Step grid, current pattern number, arrow and queued number; all measured inside x132..184. Song position uses linked bars. |

Retain parameter names, exact values/units, kit/bank identities, peer names, FX/effect/page names and modal questions. Keep unavailable `--` and blocked `X` distinct. No new REW/FWD action cards are opened: those keys already queue patterns, and their existing cue feedback gains the pattern/arrow picture. No control routing, cue timing or transport behavior changes.

Independent design review identified and resolved the 11-pixel button-height constraint, complete cue-group width, distinct selection/check meanings, and held-FX versus ordinary acquired state. The implementation and capture scenes must preserve those distinctions.

1. Extend the registered `smc-remainder` fixture first for glyph bounds, state distinctions, real queue/cue rendering, exact values and unchanged navigation. Preserve previous assertions.
2. Implement the compact atlas/painter and apply it to the two affected screen families: the keyboard top bar and existing SMC popup.
3. Extend the separate capture driver with every changed state. It uses only existing high-level rendering calls so the same fixture can capture the pre-change base.
4. Run a static Compound Engineering review, fix supported findings, then publish to the same branch/PR with the required author. Runtime checks and capture execution remain NOT_RUN.

## Optional box framebuffer diagnostics

For internal box diagnostics, the existing capture-driver command from README.md can compare this follow-up base and the new head. These optional 240x240 PNG scene pairs are defined under `framebuffer/{before,after}/`:

- `glyph-pattern-cue.png`, `glyph-pattern-cue-dim.png`, `glyph-song-position.png`
- `glyph-pattern-range.png`, `glyph-pattern-chain.png` (pattern 64, steps 49–64, 120 BPM)
- `glyph-main-mixer.png`, `glyph-locked-mixer.png`, `glyph-knob-bank.png`
- `glyph-pickup-up.png`, `glyph-pickup-down.png`, `glyph-acquired.png`
- `glyph-record-on.png`, `glyph-record-off.png`, `glyph-mute-on.png`, `glyph-mute-off.png`
- `glyph-solo-held.png`, `glyph-solo-release.png`, `glyph-play.png`, `glyph-stop.png`
- `glyph-fx-held.png`, `glyph-fx-idle.png`, `glyph-drum-bank2.png`

These are planned filenames, not generated images. No framebuffer capture, host test, compiler or model build ran here. No previews, image attachments or owner visual-approval wait are required for this change. The Orchestrator owns the remaining box qualification.

## Source review

Compound Engineering review `ui-glyph-rule-20261031` completed with five static lenses and no retained code defect. The [receipt](review/ui-glyph-rule/review.json) records the historical **Not ready** verdict. External qualification remains pending; the later owner instruction removes its visual-approval wait. Its two coverage gaps received additional written tests: actual action-card toggle mappings and complete header compositions (range, chain, cue precedence and tempo pixels). The new assertions were not run. No existing assertion changed or was removed.

The constant atlas contains 468 bytes of source data. It introduces no mutable static state or new animation timer. Native flash fit, actual placement and Elite/Pro free static RAM remain **NOT_RUN**; inspect both link maps and explain any RAM decrease before qualification.
