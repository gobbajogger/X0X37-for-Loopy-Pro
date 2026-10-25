# Miami engine switching UX

Reason: operators must change between sound engines (native303 ↔ WBL1)
without ambiguous labels or silent state.
Purpose: ship the public select/switch affordance and active-engine label on
the product (stream5 / PR #262) tip; keep schema ownership unchanged; refuse
to claim seamless audio PASS while WBL1 runtime is absent from this tip.
User effect: PART → ENGINE chooses 303 or WBL1 per bass part; the knob strip
always shows the live engine (`303A · 303`), distinct from the part name; a
WBL1 choice without runtime toasts `WBL1 NOT IN THIS BUILD` and stays on 303.

## Pins

| Item | Value |
| --- | --- |
| Product base | PR #262 tip `cursor/miami-engine-switch-ux-a9c2` / stream5 `fabd8b14` |
| This branch | `cursor/miami-engine-switch-select-00bd` |
| Miami runtime pin | `d7f156ec` on `experiment/miami-step-snapshot-20261026` |
| Schema ownership | sole REC/Miami writers; no stream rewrite |

## Verdict

**FAIL for seamless live switch** — public select + active label shipped;
WBL1 audio runtime is not on the product tip (no `synth_engine` / wobble XIP
on this head). Selecting WBL1 is refused with an honest toast. Do not treat
this as seamless PASS.

Native measure (tip `93b4be3b`, Elite+Pro): product tip already **XIP
overflow 620 B** before WBL1; thin WBL1 costs **~2928 B** (`fa26655c`
on−off); Miami pin WBL1 costs **~3312 B**. See
[miami-wbl1-live-xip-fit.md](miami-wbl1-live-xip-fit.md). Live-audio PASS
criteria there are **not met**.

## Shipped here

- `bass_engine_select` — session live-engine table (1=303, 2=WBL1); WBL1
  available only when `X0X_DEV_WOBBLE` is linked.
- PART view **ENGINE** page (first page on 303A/B): operator select 303/WBL1.
- Knob strip always appends live engine; held snap mismatch shows `LIVE/SNAP`
  (e.g. `303A STEP 5 · 303/WBL1`).
- Toast `WBL1 NOT IN THIS BUILD` when select is refused; `ENGINE: 303` on
  successful native select.
- No silent DEV_WOBBLE part-A fixture on this tip (product builds never had
  it). Select API contract: any future WBL1 boot must call
  `bass_engine_select` after the audio boundary — never replace part A
  without updating the live label.
- Host: `run_bass_engine_select.sh`, `run_snapshot_storage.sh`,
  `run_stream5_combined.sh`, `ui_page_layout_test.sh`.

## Deferred (still required for seamless PASS)

- Land WBL1 / selectable-engine runtime on the product tip without native XIP
  FAIL (Miami pin has the DSP; grafting it is a separate integrator job — no
  PE #246 grafts from this unit).
- Wire `engine_bass_engine_select` success path to the audio-boundary slot
  swap when that runtime exists; remove Miami’s silent boot hardwire.
- Persist per-part engine in project settings only via schema-owned writers.
- Distinct load toasts; stream4→5 migrate notice; engine-specific knob names;
  piano-roll UI import when source is available.

## How to verify

1. `sh tests/host/run_bass_engine_select.sh`
2. `sh tests/host/run_snapshot_storage.sh`
3. `sh tests/host/run_stream5_combined.sh`
4. `sh tests/host/ui_page_layout_test.sh`
5. On UI: PART on 303A → ENGINE page → turn to WBL1 → toast
   `WBL1 NOT IN THIS BUILD`, tag still `· 303`. Hold a snap-identity-2 step →
   tag `· 303/WBL1`.

Local-env review only — not GitHub Actions as merge gate.
