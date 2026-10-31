# Row 41: hold one FX button and use arrows

Latest owner instruction, 2026-11-01: no previews and no visual-approval wait for this change. Source delivery proceeds; the Orchestrator verifies the exact PR head on the box.

Owner change, based on PR #800 at `af1a3a5e6ab9d4442977ed283072ded3a034ddee`: hold one of the four FX buttons on a strip and press `<` or `>` on the SMC or keyboard. Left steps backward and Right forward through that bus's existing suited effects, with wrap. Only that physical button's mapping changes. The held effect updates live; key-up ends it. HOME+FX-button no longer remaps.

All assigned FX buttons fire on key-down, including the three main SELECT buttons that previously waited 180 ms. Their short taps retain the existing page navigation on release; an arrow edit cancels that navigation. Other dual-purpose FX buttons retain their short-tap navigation too. No mapping chord repeats while an arrow stays down. MMC toggles have no physical hold and cannot be remapped with arrows.

## Arrow conflicts and resolution

The arrows were **not free** in this state:

| Existing route | Resolution while a physical FX key is held |
| --- | --- |
| SMC `<`/`>` (`0x62/0x63`) changes the mixer parameter page. | Held-FX edit consumes the press before `smc_page_set`. Page, bank, locked targets and pattern cues stay unchanged. |
| SMC HOME+`<`/`>` changes ROOM/TIGHT. HOME is also physical FX note 6 on the FX screen. | Note 6 now owns the new held-button mapping chord, like the other FX keys. To select an FX preset page with the old modifier, physical keyboard HOME+SMC arrows still works when no FX button is held. HOME+Up/Down cutoff save/reset remains. |
| Keyboard `<`/`>` generates a SELECT detent; an open menu edits a row and repeats. | An optional `arw_frame` interceptor consumes a fresh edge before a detent or repeat is generated. The existing keyboard source call site is unchanged; no main_fm1.c edit. |
| Modal/menu/help, project transfer, performance test, clip mode, physical keyboard HOME, or arrows assigned by hardware calibration. | Existing input ownership remains. FX interception requires an eligible physical-key owner on its FX view with no modal overlay/help/project/performance/clip context. Existing routing/masking handles HOME/calibration before the hook. |

One eligible held key selects the target. If several keys are held, arrows are consumed without any mapping or navigation change. SMC checks its own peer; keyboard checks all eligible peers. Releasing down to one key allows subsequent arrow presses to edit that remaining key. Original bus and page ownership are preserved, including empty cells. Central BLE, USB and peripheral BLE User inputs share the existing ownership path; a central BLE link is not required. Empty cells remain silent; Right assigns the first suited effect and Left the last.

## Visual design and source review

Reuse the approved 9x9 glyph style and production framebuffer painter. The top bar reserves x182..199,y1..18 for an 18x18 effect picture while the button is held. Context text ends by x178, the control picture starts at209, and the strip number remains at235. Room waves represent reverb throw, smaller repeated blocks delay, curves high/low-pass, distinct pulse patterns the gates, repeated chunks stutter, offset beats shuffle, and a crossed cell an unassigned slot. The footer retains exact effect/bus/page names for identification. No new animation clock.

Independent design review preceded implementation and required original bus/page ownership, empty-cell feedback, immediate main SELECT activation and navigation suppression after a chord. The [plan](../../superpowers/plans/2026-11-01-fx-held-arrow.md) records those decisions. Compound Engineering source review completed with correctness, standards, testing, maintainability and adversarial lenses. The central-link guard and remaining-key focus corrections received one independent static validator batch; the stale HOME caption and ambiguous-chord popup corrections were checked separately. No supported source finding remains. See the [review receipt](review/fx-held-arrow/review.json) and [source hashes](review/fx-held-arrow/source-manifest.json). Runtime qualification remains pending.

## Tests written; no execution in Codex

The existing registered `smc-remainder` fixture adds real production MIDI dispatch, strip ownership, actual `arw_frame` interception and framebuffer assertions. Coverage includes forward/back/wrap, one-cell scope, blank cells, original page retention, multiple holds, immediate main SELECT, chord release/lost-release/context exit, removal of HOME remapping, modal guards, arrow repeat suppression and restored normal arrows. The dedicated capture driver excludes new assertions on the BEFORE pin, as before; it uses the same production scene calls on both pins.

Existing checks changed only where the owner changed behaviour:

- `smc-select-mode`: main SELECT must activate immediately instead of staying silent until 180 ms. Its release/navigation/context checks remain.
- `smc-fx-preset-preserve`: main taps publish the mapped effect immediately; mapping uses the real one-note held step rather than HOME+SELECT; an empty held cell opens its unassigned target feedback. Long hold, lost release, chosen effect, independent peer, silent blank and MMC checks remain.

No new selector is needed: all new assertions are called by the already registered `smc-remainder` main. Box qualification must include `smc-remainder`, `smc-select-mode`, `smc-fx-preset-preserve`, `smc-fx-two-page-strip`, `smk-arrows`, `smk-arrows-owner` and the full suite with 0 fail / 0 skip. Also exercise `smc-remainder` with `X0X_HOST_DEFS=-DBLEC_TWO_PEER=1` for the optional two-peer configuration. RED/GREEN and all runtime results are **NOT_RUN** here.

## RAM reason and qualification

The new original-page bitmask adds **4 B per mixer peer** to `smcfx_strip_t` (default one peer; optional two peers add 8 B in total). This is the written reason for that source-level RAM increase: an arrow edit must update the page owned at key-down, including a blank slot, rather than a page selected later. No extra timer or allocation. Eleven 9x9 effect/unassigned glyphs add 198 B of constant atlas data; total atlas source data is 666 B, plus 10 B of effect-to-glyph constants. Native placement, padding and actual free RAM require link maps.

SDK **V1.2.1**, Elite and Pro, **X0X_DEV_PADCAL=0**: **NOT_RUN**. Free static RAM: **Elite NOT_RUN; Pro NOT_RUN**. Compare against the owner reference 2352 B and exact prior candidate maps; explain any decline beyond the documented ownership mask.

## Optional box framebuffer diagnostics

BEFORE is `af1a3a5e6ab9d4442977ed283072ded3a034ddee`; AFTER is the new PR head. Use the existing README capture command with the candidate fixture/capture driver copied into the BEFORE checkout. Do not run test assertions against the old behaviour. If the Orchestrator needs internal framebuffer diagnostics, the existing driver can produce real 240x240 host-framebuffer PNG pairs under:

`docs/engineering-delivery/f4-smc-mixer-remainder/framebuffer/{before,after}/`

- `fx-arrow-held.png`, `fx-arrow-right.png`, `fx-arrow-left.png`, `fx-arrow-release.png`, `fx-arrow-focus-return.png`
- `fx-arrow-empty.png`, `fx-arrow-empty-left.png`
- `fx-arrow-main-quick.png`, `fx-arrow-main-edit.png`, `fx-arrow-main-release.png`
- `fx-effect-0.png` through `fx-effect-9.png` (all ten effect pictures)

These paths are **planned and do not exist yet**. No PNG, host test, framebuffer driver, compiler or model build was executed in Codex. No preview, capture attachment or owner visual-approval wait is required. PR #800 remains draft for Orchestrator box qualification; this handoff authorizes no merge.
