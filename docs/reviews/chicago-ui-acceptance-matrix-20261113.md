# Chicago V_PART responsiveness: FMB execution/acceptance matrix (2026-11-13)

**Owner:** Firmware Main Builder (FMB) executes/integrates; Firmware UX decides any UI changes. **This PR:** documentation-only QA contract. **Current status:** protocol prepared; owner-scene UI and audio qualification **NOT_RUN**. Source pinned to `94197fc839b00aff47b98ca02f3c6396e4a6eabf` (tree `72d28fbc741244d18fa3d18495da16ab0fcb1d24`), JieLi SDK V1.2.14. Compare a candidate to this exact parent; do not silently compare different firmware images.

Source path and hypotheses were already investigated in [independent review #888](https://github.com/gobbajogger/X0X37-for-Loopy-Pro/pull/888) (`b1888beb15724b5c57f8c18783cb57fd93d082cf`; `docs/reviews/chicago-miami-ui-latency-20261113.md`). This matrix **does not** re-diagnose the code or implement a fix. [BREAK/audio observer #892](https://github.com/gobbajogger/X0X37-for-Loopy-Pro/pull/892) and [Chicago cache focused tests #897](https://github.com/gobbajogger/X0X37-for-Loopy-Pro/pull/897) remain separately owned. Chicago is the **303**; Miami is **not** the 303.

## 1. Exact run matrix (repeat for Elite and Pro, independently)

Common scripted navigation per UI capture: start from HOME with bass A selected; press EDIT once into V_PART, wait for display completion; press EDIT to advance twice; turn the same *positioned* mapped knob through four reversible detents, recording the changed underlying `R_ENG` parameter and a changing pixel bounding box; return to HOME. Run one cold-page-entry trial and one warm-repeat trial, preserving keys/arp until their actual release. Record the actual input route (physical, USB MIDI, SMC), press/detent timestamps and whether a press was consumed by an overlay/preset cancellation. A page-advance in V_PART **does not call `build_pages()`**: do not assign page-change time to that function. Sequence 10 s setup/settle then 30 s observation (measurement window, **not** a performance gate); at least 20 navigation/knob events across repeated trials, same prerecorded event timestamps for both engines. Keep 30 s of audio halves even for the no-UI control. Any capture without true input edges or required phase points is `INCOMPLETE`.

| ID | Workload and interaction | Purpose |
| --- | --- | --- |
| U0 | No playback, no notes; scripted cold+warm V_PART page entry, page advances, knob edits | Page-generation/render/transfer control |
| U1 | Bass A Chicago arp enabled, B silent, no drums/BREAK; same UI actions | Isolate Chicago-specific arp presentation |
| U2 | U1 + B Juno with **three verified held notes** + 909 BD on steps 1/5/9/13; BREAK OFF, 808 OFF; same actions | Separate Juno/909 contention from BREAK |
| U3 | Full scene, audio only while V_PART remains displayed: A arp, B held Juno triad, **audible/verified contributing BREAK**, 909 BD four-on-floor, 808 OFF; no UI transitions | Concurrent audio-half/dropout control |
| U4 | **Owner scene:** exactly U3, with UI script and live knob edits while playback continues | End-to-end owner's reported failure; primary acceptance case |

For **each** ID run A=Chicago and A=Miami on the **same bass part**, B's **selected** engine=Juno unchanged (silent in U0/U1, held triad in U2–U4), both hardware models, and, when testing an implementation, both pinned baseline and candidate; distinguish source/model/condition, not just a screen name. On U1/U2/U3/U4 the Miami variant keeps the same musical arrangement and same number/timing of inputs; record the different engine-specific patch/parameter mapping rather than pretending that Chicago and Miami share a control index. When an arp control has no Miami counterpart, use the same physical page/knob travel and mark `parameter_semantics_comparable=false`; exclude non-equivalent value-text comparisons, **not** latency comparisons. Fix tempo, project, pattern, key velocity, Juno preset, BREAK source/level, FX/send/volume, MIDI clock, sample rate, SMC peers, screen brightness, LEDs, playback duration and power/USB state. Capture the exact snapshots on every trial. For BREAK record slot/pack hash, actual played steps and **nonzero current-stretch dry-mix contribution plus output/audibility evidence**; "sample loaded" or general nonzero PCM is insufficient. Owner's actual installed image, sample and precise settings are still unknown; a representative reproducible fixture is valid for diagnosis but `owner_scene_exact=false`.

## 2. Existing anchors and boundary probes: clock all durations in microseconds

At pinned source, `fm1_ticks() / FM1_TICKS_PER_US` is the existing hardware timer. Use timestamp pairs on the *same* clock, event IDs and monotonic wrap-safe differences; do not time printf/USB telemetry inside audio IRQs. FMB may add **bounded, opt-in** counters on **its** implementation branch; none are added by this QA PR. Export only after a run; instrumented vs noninstrumented timing must be distinguished.

| Field / event boundary | Exact existing source anchor | Meaning and warning |
| --- | --- | --- |
| `input_edge_us`, `dispatch_us`, `param_commit_us` | `main_fm1.c:2564-2588` dispatches `ui_frame()/ui_input_only()`; `ui.c:8821-8830` calls `input()`; `ui.c:4059-4070` B_EDIT; `ui.c:710-759` `pref_set_impl` (including `original_sound_set`/`engine_bass_sound_refresh`), `ui.c:904` `pref_set` for knobs; confirm actual caller/route | Distinguish raw physical/MIDI arrival, actual dispatch, value mutation. Existing LCD trace KEY is key/MIDI arrival-to-dispatch, **not** knob-to-param timing. For knob paths, FMB must identify the actual setter/caller in the candidate. |
| `build_pages_start/end_us` | `ui.c:2752-2780` `set_view(V_PART)`; `ui.c:1584-1602` `build_pages()/describe_pages()`; Chicago/Miami row selection `ui.c:1509-1527` | Page entry only; count calls. B_EDIT page advance changes page index without running `build_pages()`. |
| `frame_start/end_us`, `header_us`, `canvas_main_us`, `canvas_knobs_us` | `ui.c:8821-8900` `ui_frame -> draw_header, draw_main, draw_knobs`; `ui.c:6814-6892` main bands 0/1; `ui.c:8319-8345` knob band 2 | Capture three separately tagged band visits, count actual visible widgets; main and knob scopes include commit unless FMB splits them. Don't sum overlapping inclusive scopes. |
| `widget_calls`, `text_view_calls`, `wave_view_calls`, `matrix_value_calls` | `ui.c:5740-5780` `draw_part_sound`; `ui.c:7200-7285` `draw_param_widget`; `ui.c:2893-2925` original text/wave views; `ui.c:935-956,1054-1111,1148-1158` name/text/value | For diagnosis only: record per band and per row, including cache cold/warm, not presumed fixed cardinality. Chicago arp branch `ui.c:7200-7225`. Normal V_PART **bypasses** `knobs_sig` (`ui.c:8336-8351`). |
| `tile_hash_us`, `dirty_tile_count`, `emitted_bytes`, `dma_chunks` | `ui.c:4780-4838` `cv_commit_rows/rect/knobs`; `ui.c:4706-4770` `cv_lcd_emit/full` | Separate CPU canvas/render from hash/coalescing and actual bytes/chunks. One chunk <=1536 pixel bytes; reduced DMA does not imply reduced CPU canvas cost. |
| `lcd_sync_wait_us`, `dma_begin/end_us`, `pixel_transfer_done_us` | `firmware/src/lcd.c:33-63` `lcd_sync,lcd_data`; `ui.c:4706-4770` `cv_lcd_emit` | Measure **blocking wait** at `lcd_sync` separately from transfer elapsed. Map the changed pixel/tile to its last completed DMA; record panel-visible latency separately if optical capture available (DMA complete alone is not LCD scanout proof). |
| `audio_half_us`, `half_deadline_us`, `late_delta`, `xruns_delta` | `main_fm1.c:33-94` `render_block/x0x_dbg`; `main_fm1.c:96-116` `fm1_alnk0_irq` | Read coherent *completed IRQ halves*, not just smoothed PERF or `last_us` polling; correlate each half with the concurrent UI event/window. Actual DMA deadline and nominal 11609 us are separate fields. |

Already available **only when the existing ELF was built with `X0X_LCD_TRACE=1`**: `firmware/src/app/lcd_trace.h:1-104` has latest-32 sample nearest-rank P50/P95 of KEY, MIDI, DMA duration, FRAME interval, TIMER5 and AUDIO, plus LCD bytes/frame; hooks in `lcd.c:33-63` and `main_fm1.c:2564-2588`. It **does not** split `build_pages`, canvas, tile hashing, `lcd_sync` blocking wait, parameter commit or changed-pixel completion. `X0X_LCD_TRACE=0` does not provide these counters; no fabricated post-hoc durations. `x0x_dbg` tracks `halves,max_us,last_us,late,nested,ui_frames` (`main_fm1.c:24-34,69-116`) and is useful when captured coherently; PERF Miami/BREAK zeros and idle 808 spikes are **not** accepted as attribution. `engine_bass_edit_hold` is only depth counter + compiler barriers (`original_presets.inc:259-265`), not a spinlock/IRQ masking; Chicago is ineligible for core 1 due to the pinned stack boundary, Miami may be eligible (`engine.c:2615-2624`).

Compute separately for *every* action `input_to_param_us = param_commit - input_edge`, `input_to_first_pixel_dma_us = first changed-pixel tile DMA completion - input_edge`, `input_to_panel_pixel_us` only with external visual/photodiode timing, and `page_entry_to_pixel_dma_us` for ENTER. Keep all three distinct. A canvas unchanged because an edit was ignored is `no_pixel_change`, not zero latency. If no target pixel address/bbox is captured, input-to-visible is `NOT_RUN`, not estimated from `ui_frame` rate.

## 3. Required recording contract (one JSONL stream for UI, one #892 audio trace)

Header record `{"type":"run","schema":1,...}`: `run_id`, `source_sha`, `git_tree_sha`, `model` (`elite` or `pro`), `elf_sha256`, `map_sha256`, `package_sha256`, `trace_mode` (`none`, `lcd_trace`, `fmb_probe`, `optical`), `probe_build_sha256`, `clock_hz`, `baseline_or_candidate`, `scenario_id`, `bass_a_kind`, `bass_b_kind`, `part_ids`, `snapshot_sha256`, `tempo_bpm`, `input_route`, `event_script_sha256`, `break_pack_sha256`, `sample_slot`, `break_mix_verified` (bool), `break_audible_verified` (bool), `909_pattern_verified`, `808_inactive_verified`, `juno_triad_held_verified`, `owner_scene_exact` (bool), `steady_window_start_us/end_us`, `duration_us`. Never substitute an absent field with zero, especially scene predicates or timestamps.

Per action `{"type":"ui","schema":1,...}`: `run_id`, `event_id`, `action` (`edit_enter` / `edit_next` / `knob_detent`), `part`, `parameter_ref` (nullable), `page_before/after`, `input_edge_us`, `dispatch_us`, `param_commit_us` (nullable for navigation), `build_pages_start_us/end_us` (nullable on page advance), `frame_start_us/end_us`, `draw_header_us`, `canvas_main_us`, `canvas_knobs_us`, `band0_calls/band1_calls/band2_calls`, `widget_calls`, `text_view_calls`, `wave_view_calls`, `tile_hash_us`, `dirty_tiles`, `dma_bytes`, `dma_chunks`, `lcd_sync_wait_us`, `pixel_bbox` (nullable), `pixel_dma_done_us` (nullable), `panel_pixel_seen_us` (nullable), `no_pixel_change`, `trace_complete`. A frame may contain multiple UI actions: use stable `frame_id` and avoid double-charging shared canvas/DMA work. For per-action time, associate the first causally updated frame/pixel and retain all candidate frames until it commits.

Companion `{"type":"audio_window","schema":1,...}`: `run_id`, `observer_trace_sha256`, `observer_source_sha`, `coherent_halves`, `total_halves`, `worst_half_us`, `actual_min_slack_us`, `deadline_misses`, `late_delta`, `xruns_delta`, `discontinuities`, `break_mix_halves`, `status`. Preserve every half's exact scheduled DMA deadline and IRQ-return state in #892 `per_half.csv`; never reconstruct these from PERF percentages. Scene validity is a separate conjunct, **not** a consequence of zero late.

All times are absolute monotonic microseconds (not wall-clock); record the origin/calibration and timer wraps. Raw scenes/PCM/personal samples and proprietary firmware binaries stay private. Results `PASS`, `FAIL`, `NOT_RUN`, `INCOMPLETE`, `INVALID_SCENE` require a reason; bad/missing input does not become PASS.

## 4. Offline commands: reuse only matched existing artifacts; no new build in this PR

From an *existing* source/artifact workspace, locate candidates without network or writes:

```bash
find . -type f \( -name '*.elf' -o -name '*.map' -o -name '*trace*.jsonl' -o -name 'summary.json' \) -print
find . -type f -name '*.elf' -exec sha256sum {} +
find . -type f -name '*.map' -exec sha256sum {} +
```

Previously pinned exact-`94197fc8` ELF hashes from #892 (verify **bytes** and matching provenance; never infer source from basename): **Elite** `720f851c83a62fbef71fe52ad84e669409a69fde1d1fb313427d7942ed3ca1ea`; **Pro** `7e67c3b33fe2e690e5f913b5d048727a715c70daca06ddd326a10e37eacb9535`. Packages: Elite `95b3195f4767ef5df33b365197b01203873e5eec3d7fa497c73e5e509484a4a6`; Pro `ce67d560eebd03cb8b9087f7723b1e0b71fb7cca256184cdfdd55a8c27c708b8`. Their provenance was recorded by #892, **not rechecked here**. Mismatched map/ELF/source/build mode -> `INVALID_ARTIFACT`.

```bash
find . -type f -name '*.elf' -print0 | while IFS= read -r -d '' elf; do
  printf '\n=== ELF %s ===\n' "$elf"
  sha256sum "$elf"
  if command -v llvm-nm >/dev/null 2>&1; then
    llvm-nm -S --defined-only "$elf" | grep -E ' (ui_frame|build_pages|draw_part_sound|draw_param_widget|original_text_view|original_wave_view|lcd_sync|render_block)$' || :
  fi
  if command -v llvm-objdump >/dev/null 2>&1; then
    llvm-objdump -d "$elf" | grep -E -m 20 'build_pages|lcd_sync|ui_frame|render_block' || :
  fi
done
find . -type f -name '*.map' -print0 | while IFS= read -r -d '' map; do
  printf '\n=== MAP %s ===\n' "$map"
  grep -nE -m 24 'build_pages|draw_part_sound|ui_frame|lcd_sync|render_block|x0x_dbg' "$map" || :
done
```
Optimized/static/inlined symbols may be absent. An ELF symbol size or disassembly is **not** an elapsed-time result. The matching linker map is required to translate existing RAM snapshots into addresses; `llvm-addr2line -e "$elf" "$captured_address"` only after a **real** `captured_address` is captured. Never use symbol addresses from another build/model.

#892's existing host-only audio observer/reducer is the single source for per-half DMA/late/BREAK evidence. In the **existing #892 tooling checkout** (not this docs-only branch), after locating a *real* trace by `find`, the verified reducer interface is:

```bash
python3 dnbox/dropout/trace.py --help
python3 -m unittest discover -s dnbox/dropout -p 'test_*.py'
```
For a captured trace, this complete command **requires exactly one existing trace file** rather than silently selecting arbitrary evidence. Run in a local #892 tooling checkout; it neither records a new trace nor builds firmware:

```bash
set -eu
test -f dnbox/dropout/trace.py || { printf 'NOT_RUN: #892 reducer checkout absent\n'; exit 2; }
mapfile -d '' -t traces < <(find . -type f -name 'trace.jsonl' -print0)
if [ "${#traces[@]}" -ne 1 ]; then
  printf 'NOT_RUN: expected one preverified trace.jsonl; found %s\n' "${#traces[@]}"
  exit 2
fi
sha256sum "${traces[0]}"
outbase="$(mktemp -d)"
python3 dnbox/dropout/trace.py --trace "${traces[0]}" \
  --start-ms 20000 --end-ms 29000 --slot 1 --out "$outbase/reduced"
```
The 20–29 s values are the historical rejected-run window, **not** a universal acceptance window. Record command, exact trace hash and `summary.json/per_half.csv` together; never treat `OBSERVED` by itself as qualified owner scene. The known 30 s Elite run had **775** coherent halves, `max_guest_last_us=8467` and no observed late, **but was rejected**: A=Miami, B=Chicago, Juno absent, 808 active and BREAK silent/zero steps (see #892 `dnbox/dropout/evidence/2026-11-13-initial-rejected.json`). Do **not** reclassify this run as owner-scene PASS. Any separately reported **648-half** scene also remains a different, unqualified workload until its captured part/arp/Juno/BREAK/909 state is proven to match the owner's scene.

## 5. Interpretation and pass/fail decisions

Compute p50/p95/worst **per model, engine, scenario, action, cold/warm, exact source and trace mode**, plus raw ordered samples and counts. Baseline-to-candidate deltas use the **same scene/input script/trace overhead**; Chicago-vs-Miami is a controlled *diagnostic contrast*, not a claim of identical patches. Preserve input->parameter, input->pixel DMA, verified panel pixel, page-build time, canvas per band, tile hash/bytes and *LCD wait* independently.

- **H-page:** Chicago `build_pages` time diverges on U0/U1 with no extra dirty bytes; profile descriptor construction first. If B_EDIT next-page lag occurs without `build_pages` calls, H-page cannot explain that lag.
- **H-canvas/preview:** Chicago U1/U4 has larger `draw_part_sound/draw_param_widget/text_view` counts or CPU time but comparable DMA bytes/wait. A smaller number of `og_states` restorations (#897) is a *possible mechanism*, not a latency PASS until time/pixels prove it.
- **H-tiles/DMA:** canvas times match but U4 emitted tile bytes, transfer completion or `lcd_sync_wait_us` grow. Dirty tiles and DMA duration must not be conflated with first visible changed pixel.
- **H-contention:** U0/U1 acceptable but U2/U4 regress while completed audio-half slack shrinks or foreground waits grow; inspect exact plan/core masks, not the misleading per-part PERF bars. **Never** shift Chicago to core 1 without separate stack proof.
- **H-input:** `input_edge->dispatch` dominates, even when draw/transfer is modest; prove input path/queue service, not speculative `engine_bass_edit_hold` contention.

**UI acceptance:** must report per-model paired pinned-baseline versus candidate p95 and worst for page-entry, input-to-parameter and input-to-first-updated-pixel, with **no regressions** under matched conditions and owner-observed immediate response in the exact U4 hardware scene. An empirical absolute numeric page-entry/pixel latency gate has **not** been established; do not invent one (the 16 ms scheduling interval is not one). A false `0 us` value, an absent changed pixel or incomplete samples cannot pass. Distinguish DMA completion from true optical panel latency.

**Audio hard gate:** each accepted U4 window contains verified Juno held triad, Chicago arp, active+contributing audible BREAK, 909 four-on-floor and no 808; **worst measured audio half <11609 us** and **zero late/xruns/deadline misses/audio discontinuities** for the declared observation window. Use the actual scheduled DMA deadline too. A representative or rejected scene is not owner validation. Native resource gates for **exact candidate model-specific production builds** (FMB, not this PR): **flash free >=8192 B and pool free >=8192 B for both Elite and Pro**; trace-on debug images must be separately attributed, never substituted as shipping-size evidence.

**Minimal missing inputs before an owner-scene verdict:** actual owner model/installed package SHA, exact Chicago/other-part (Juno) presets + arp/note/velocity/tempo/FX settings, active BREAK sample/pattern and audibility proof; pinned/candidate **matched** timestamped UI traces with per-phase/changed-pixel boundaries, model-matched production flash/pool link receipts, and coherent U4 audio-half evidence from #892 or hardware. Until obtained: **U4=NOT_RUN**; do not block this documented fixture from being handed to FMB. No production firmware edits, builds, emulator runs, device writes, merges or release publication occurred here.
