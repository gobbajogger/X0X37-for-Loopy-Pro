# DnBOX Chicago/Miami UI-latency: independent source diagnosis (2026-11-13)

Status: **read-only source investigation, PARTIAL**, not a performance result or firmware fix.
Pinned source: `94197fc839b00aff47b98ca02f3c6396e4a6eabf`. This report changes no production files. Consumers: FMB (implementation/integration) and Firmware UX (design).
Owner scenario: Chicago parameter-page lag while Chicago arp + held Juno triad + BREAK + four-on-the-floor 909 kick; same scene with Miami responsive. **Owner hardware observation, not independently reproduced.** Chicago is the 303 engine; Miami is **not** a 303.

## Exact engine-specific page-entry/draw trace

1. UI navigation enters `set_view(V_PART)` at `firmware/src/app/ui.c:2752-2780`, which calls `build_pages()`. Repeated EDIT presses in V_PART increment page (`ui.c:4059-4070`); this is a separate page-change route.
2. `build_pages()` calls `describe_pages` (`ui.c:1584-1602`). V_PART bass chooses `engine_bass_bank(b)` and **Chicago rows/titles/page count vs Miami rows/titles/page count** (`ui.c:1509-1527`), populating four parameter refs for each row, plus the SOUND page. Difference is concrete, not a generic shared-page claim.
3. Main foreground loop invokes `ui_frame` on a nominal 16 ms minimum cadence (`firmware/src/app/main_fm1.c:2564-2588`). `ui_frame` calls `draw_header`, `draw_main`, `draw_knobs` (`ui.c:8821-8900`); `draw_main` dispatches V_PART bass to `draw_part_sound(band)` for **two bands** (`ui.c:6814-6892`).
4. `draw_knobs` calls `draw_part_sound(2)` **again** for the V_PART matrix (`ui.c:8319-8345`). Consequently each changed matrix frame can visit three matrix bands. Each visible row iterates up to four `draw_param_widget` calls (`ui.c:5740-5780`), which resolve descriptor, matrix value, name, wave/glyph and formatted value (`ui.c:7200-7285`). Number actually visited depends on page grouping/visibility; do not claim 12 or 16 calls for every scene without instrumentation.
5. `pref_desc` routes bass engine refs to `engine_param` (`ui.c:536-550`). `pref_name` routes them to `ui_original_name(sound_bank,id)` (`ui.c:935-956`). `pref_value_of` routes to `original_value_text` under an edit hold (`ui.c:1148-1158`), and `pref_value` may call `original_display_value` first (`ui.c:1282-1286`).
6. Chicago-specific control text dispatch occurs in `original_value_text` (`ui.c:1054-1111`), including gate, arp on/mode/range/speed and FX name. `draw_param_widget` has an additional **Chicago-only** arp 13..16 branch, calling `original_text_view` and (for arp on) `matrix_value` (`ui.c:7200-7225`). This is a distinct path relative to Miami, not proof of its runtime cost.
7. `original_text_view` scans a kind-dependent TEXT_MASK over controls, asks lock/motion preview for each relevant control, and updates a shared controls snapshot (`ui.c:2893-2909`). `original_wave_view` repeats a mask scan for applicable waveform slots (`ui.c:2911-2925`). `draw_param_widget` calls `matrix_name(r)` multiple times for label width/shadow/text (`ui.c:7232-7245`). These are repeated operations within a single foreground redraw, potentially multiplied by visible cells.
8. The normal V_PART matrix explicitly bypasses `knobs_sig()` and its redraw short-circuit: the direct V_PART branch in `draw_knobs` occurs before `sig=knobs_sig()` (`ui.c:8336-8351`). **Important correction to potential false theory:** the costly `knobs_sig` all-param hash (`ui.c:8200-8270`) is **not** run on the ordinary Chicago/Miami V_PART matrix path. Do not attribute page-entry lag to `knobs_sig` without another caller.
9. LCD pixel transfers are capped at 1,536 bytes per DMA chunk and foreground input is serviced between emitted chunks (`ui.c:4706-4770`). `lcd_sync` waits for previous DMA (`firmware/src/lcd.c:33-63`). A 16 ms frame request is not a guaranteed 16 ms completion. Dirty-tile hashing can reduce SPI transfers but not pre-transfer CPU cost of drawing the canvas (`ui.c:4780-4838`).

## Ranked findings and uncertainty

**S1 (source-proven; high relevance):** The main Chicago/Miami engine selection yields different parameter row descriptors and Chicago-specific arp-control drawing (ui.c:1509-1527, 7200-7225). The observed asymmetry is compatible with this path; differential execution cost **unmeasured**.

**S2 (source-proven; high relevance):** V_PART redraw traverses matrix render in main-area bands and knob strip, with repeated per-cell descriptor/value/name/preview work (ui.c:6814-6892, 8319-8345, 5740-5780, 7200-7285). Redundant lookups are source-visible. Whether this dominates elapsed time **unknown**.

**S3 (source-proven; medium relevance):** `original_text_view`/`original_wave_view` repeatedly resolve lock/motion preview across mask bits (ui.c:2893-2925); `pref_value_of` takes `engine_bass_edit_hold` for each original bass value conversion (ui.c:1148-1158). Hold behavior and IRQ wait costs **not yet verified from its defining implementation**. Do not assert it disables IRQs or stalls audio.

**S4 (source-proven exclusion):** `knobs_sig` is bypassed on the matrix branch (ui.c:8319-8351). Avoid an irrelevant cache rewrite.

**H1 (hypothesis):** Chicago arp control rendering and extra live motion/lock queries make page-entry CPU time larger than Miami in the owner's workload. Needs existing-ELF symbol timing and call counts; no timing evidence yet.

**H2 (hypothesis):** Concurrency, cache/memory-bus contention and higher real-time load from Chicago + Juno + BREAK + 909 extend foreground drawing or DMA wait. Core-1 scheduling excludes Chicago due to stack safety; Miami may be assigned to core 1 (`firmware/src/app/engine.c:2615-2624`). Do not change that constraint based on UI observations.

**H3 (hypothesis):** A burst of dirty tiles on entering a Chicago page creates visible lag even if page descriptor generation is fast. Need separate time boundaries around `build_pages`, `draw_part_sound`, `cv_commit_rect`, and DMA sync.

## Minimal safe remedies for FMB, subject to measurement and Firmware UX approval

- Measure before changing behavior: distinguish page-entry descriptor construction, canvas CPU draw, text/wave/preview calls, tile hashing and DMA wait.
- If repeated lookups dominate, compute immutable per-cell label/name/descriptor once per drawing pass or cache only on safe invalidation boundaries. Retain page-specific text, current lock/motion preview, selected engine identity and safety guards. Verify static pointer lifetime before caching.
- If page-entry repaints dominate, retain exact pixel output and update only genuinely dirty regions. No new UI cadence or visual changes without Firmware UX sign-off.
- Do **not** move Chicago audio to core 1, change note/arp IRQ ownership, remove lock checks, bypass `engine_bass_edit_hold`, change SDK clock/SPI without evidence, or touch bass303/wobble.c.

## Existing-ELF / read-only probes (NOT executed)

Inspect exactly matching Elite and Pro ELF/map/disassembly, if already produced by FMB: `nm -S --size-sort`, `objdump -dr`, `addr2line`, symbol presence/inlining for `describe_pages`, `draw_part_sound`, `draw_param_widget`, `original_text_view`, `original_wave_view`, `original_value_text`, `engine_bass_edit_hold`, `lcd_sync`, `ui_frame`. ELF static size alone **cannot** establish runtime latency.
If existing exact-source trace captures exist, compare Chicago and Miami: navigation edge -> `build_pages` return -> canvas render -> dirty-tile emit -> last SPI DMA complete; separate p50/p95/max and audio IRQ half-block worst/late. Hold scene constant with Chicago arp + Juno triad + BREAK + 909 kick. Zero new builds, emu or device operations in this review.

## Ownership and verification

No measured durations, dropouts, locks, IRQ masking or successful remedy claimed. This report is source-only. FMB implements and validates; Firmware UX approves any display design changes. Follow-up investigation should locate actual hold implementation and verify contention assumptions without firmware edits or duplicate builds.
