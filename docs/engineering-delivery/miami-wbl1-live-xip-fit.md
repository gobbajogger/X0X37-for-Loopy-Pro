# WBL1 live audio behind ENGINE select — native XIP fit

Reason: owner acceptance needs seamless live engine switching; UI ENGINE
select (#265) is merged, but WBL1 audio is still missing from the product tip.
Purpose: measure whether WBL1 runtime/XIP can land behind that select on the
stream5 product tip without breaking Elite/Pro capacity; record the exact
overflow when it cannot.
User effect: no false claim that choosing WBL1 in ENGINE makes WBL1 sound;
the toast path stays until a measured native fit exists.

## Pins

| Item | Value |
| --- | --- |
| Product tip | `93b4be3b` (`cursor/miami-engine-switch-ux-a9c2` after #265 merge) |
| Stream5 base | `fabd8b14` |
| Miami runtime pin | `d7f156ec` (`experiment/miami-step-snapshot-20261026`) |
| Historical thin WBL1 fit | `fa26655c` (cost-recovery; reference only) |
| Toolchain | JieLi `jieli-linux-toolchains-20260730.1`, SDK `d179b448` |
| Schema | Unchanged — sole REC/Miami writers; no PE #246; no rejected BREAK/clock grafts |

## Verdict

**XIP FAIL — do not land WBL1 live audio on this product tip.**

Host ENGINE select remains the shipped UX. Native Elite/Pro packages for the
product tip already overflow **before** WBL1. Enabling thin or Miami WBL1
adds ~2.9–3.3 KiB of linked XIP. Do **not** claim device PASS from host
tests. Seamless live-audio bar stays **FAIL**.

## Measured Elite link (same toolchain; `X0X_DEV_PADCAL=1`)

| Tree | WBL1 | Result |
| --- | --- | --- |
| `fabd8b14` stream5 | off | **PASS** — 196 B free (image 552476 / limit 552672) |
| `6736e0c2` (#262 clarity) | off | **FAIL** — overflow **204 B** |
| `1a9bf35f` / `93b4be3b` (#265 ENGINE) | off | **FAIL** — overflow **620 B** (Elite+Pro) |
| `93b4be3b` + capacity #263 cherry-pick (measure only) | off | **FAIL** — overflow **76 B** (Elite+Pro); recovered ~544 B |
| `fa26655c` historical cost-recovery | off | **PASS** — 3044 B free |
| `fa26655c` | on (`X0X_DEV_WOBBLE=1`) | **PASS** — 116 B free (image 552556); WBL1 delta **2928 B** |
| `d7f156ec` Miami pin | off | **PASS** — 1524 B free |
| `d7f156ec` Miami pin | on | **FAIL** — overflow **1788 B** (WBL1+runtime delta **3312 B**) |

Implied on product tip:

- Tip alone is already **620 B** over (76 B over after capacity #263).
- Thin WBL1 (~2928 B) on tip+capacity ≈ **~3004 B** over.
- Miami pin WBL1 (~3312 B) on tip+capacity ≈ **~3388 B** over.

No budget/window/reserve was changed. No production `main` / nightly FF /
force-push.

## Largest safe partial

1. **Keep** the merged ENGINE select session API + honest
   `WBL1 NOT IN THIS BUILD` toast (#265). Operators see the control; audio
   stays native303.
2. **Do not** import `experiments/wobble`, `synth_engine`, or Miami slot
   swap onto the product tip until measured free XIP exceeds ~3 KiB after
   the tip itself links.
3. **Capacity #263** (share IMA tables, drop dead UI, const key maps) is
   behavior-preserving and recovers ~544 B on this tip, but **still does
   not** clear the tip (76 B short) and is nowhere near WBL1. Treat it as
   a separate size lane, not as WBL1 enablement.
4. **Forbidden** for this unit: PE #246 font-cache, rejected BREAK #93,
   `clock-rejected.patch`.

## Host (product tip `93b4be3b`; not device PASS)

Prior #265 local proof: `run_bass_engine_select.sh`,
`run_snapshot_storage.sh`, `run_stream5_combined.sh`,
`ui_page_layout_test.sh`, `smc_ui_test.sh` exited 0. Those prove UX/session
behavior only.

## PASS criteria (live switch) — not met

All required before claiming seamless PASS:

1. Elite **and** Pro DEV packages link with WBL1 selectable at runtime
   (`code` within XIP window; `pool_free` ≥ 8192).
2. `engine_bass_engine_select(..., WBL1)` succeeds without toast; audio
   boundary swaps the bass slot; knob strip live id matches audible engine.
3. No silent boot hardwire of part A to WBL1.
4. Schema persistence only via sole writers (out of this size gate).

Until (1)–(3) are evidenced on the product tip SHA, keep the refuse toast
and label this bar **FAIL**.
