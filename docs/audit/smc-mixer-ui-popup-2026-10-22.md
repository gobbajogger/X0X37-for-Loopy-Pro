# SMC mixer UI page popup — stage 1 audit

Owner thread: `bc-01a16423-4043-76bd-8bc4-e110c6d15686` (Smc mixer ui popup).
Base revision: `e00741e2c1e8a018e7a7229dfd279e26aec34126` (`main`).
Lane: `cursor/smc-mixer-ui-popup-stage1-5686`.

## Request

Owner: make the SMC mixer pop up the **real UI page** its control is mapped to, **hold** that page for a while, then **return** to where the user was. Do the work in the seven-stage fresh-thread workflow. Stage 6 install must be a **local** thread (hardware). Host tests must verify before install.

## Activity check

| Thread | Overlap |
| --- | --- |
| [SMC page lock multiple mixers](https://cursor.com/agents/bc-9d689865-6ced-488b-a8e7-31248e12a0aa) | Different feature (persistent mixer-mode lock / multi-SMC). Audit only; no popup plan. |
| Hunt more SMC UX bugs | Done; unrelated top-5 list. |
| Audit post-SMC features | Done inventory; no popup design. |

This stage owns temporary mapped-page **peek / hold / restore** only. Do not implement page-lock.

## Current behavior (defect)

Evidence in `firmware/src/app/smc_mixer.c`:

1. Main-mixer faders/knobs edit `smc_home_target` / shared strip destinations while the screen may stay on SEQ, PART, SONG, clip, etc. (`smc_target`, `smc_fader`, `smc_knob`).
2. Feedback is only a header `say()` string (`PICK F…` / `SMC F…` / `SMC K…`) via `smc_show`.
3. `smc_show` forces `ui.touched = -1`, so the keyboard-style large readout never appears for SMC.
4. There is no save of `ui.view` / page / `mixsel`, no timed restore, and no navigation to the MIX/FX page that actually draws those meters.

Contrast:

- UI rule: every change is answered; a knob lift shows the parameter large for about a second (`docs/plans/2026-10-03-ui.md` rule 3; `BEHAVIOR.md` GEN-54; `ui.touch_until` ≈ 900 ms).
- MIX PARTS (`draw_mix`) and 909/808 MIX (`draw_drum_mix`) are the real meter pages for the strip destinations documented in `docs/SMC_MIXER.md`.

So SMC changes values without showing the mapped visual page. That is the gap.

## Intended behavior (proposed)

When an SMC fader or knob writes (or waits on) a **main-mixer strip destination**, briefly show the X0X page that owns that destination, keep it up while the control is active, then restore the previous screen.

### Destination → page map

| Strip | Target | Peek page |
| --- | --- | --- |
| 1–5 | Part level `T_MIX` / `MX_LEVEL` | `V_MIX` PARTS (page 0), `mixsel` = strip |
| 6 | `FX_RV_LEVEL` | `V_FX` REVERB (page 0) |
| 7 | `FX_DL_LEVEL` | `V_FX` DELAY (page 1) |
| 8 | `MST_DJF` | `V_MIX` MASTER (last MIX page) |

Already on that exact destination: refresh the hold timer only; do not nest another save.

Drum MIX banks: the user is already on the meter page; no peek. Visible-field knobs on sound/menu/clip pages: already on the page; optionally set `ui.touched` when the field is in the current strip (small follow-up, not required for the first fix).

### Hold / restore

- Hold ≈ **1200 ms** from the last SMC move on that peek (slightly longer than the 900 ms knob readout so meters are readable while grabbing pickup). Refresh on every fader/knob event that keeps the same peek.
- Save before first peek: `view`, MIX/FX page index, `mixsel`, and `prev_view` (so HOME / overlay restore stays correct). Do **not** call `set_view` for peek/restore: it cancels held steps and rewrites `prev_view`. Use assign + `build_pages()` only.
- Restore when the timer expires, unless the user has already navigated with an SMC/panel button (clear peek without forcing the old page back).
- Block peek while `ui.help`, `O_ASK`, or `perf_testing()`. Keep existing pickup notices.
- Clip mode: **no peek** in stage 2 (clip grid owns the main area); header notice stays. Revisit after device feedback if needed.

### Pickup

Waiting (`PICK F`) and acquired (`SMC F`) both peek. Seeing the meters while hunting the cross point is the point of the feature.

## Scope for stage 2 (implement)

Production: `firmware/src/app/smc_mixer.c` (peek state, show/restore helpers, hook from `smc_fader` / `smc_knob` / `smc_poll` or `smc_frame`). Docs: `docs/SMC_MIXER.md`, `BEHAVIOR.md` (new SMC-05). Host tests: new focused check (or extend `smc_reachability_test`) proving peek view/page/`mixsel`, hold refresh, restore, and help/ASK/clip guards. Register in `tests/run_tests.sh` if a new name is added.

Out of scope here: page-lock, multi-SMC, BLE transport, install, formal gate.

## Approvals and decisions

| Item | Status |
| --- | --- |
| Feature itself (popup / hold / restore) | Approved by this owner request |
| Hold 1200 ms; peek on waiting pickup; no clip peek; FX pages for strips 6–7 | Defaults chosen in this audit — stage 2 may use them without re-asking unless flash or UX conflicts appear |
| Stage 6 install on a **local** thread only | Required by owner |
| Concurrent page-lock work | Separate lane; do not merge scopes |

## Stage plan (fresh thread each)

| Stage | Thread kind | Goal |
| --- | --- | --- |
| 1 Audit | This cloud thread | This document |
| 2 Implement | New cloud thread | Code + docs + host tests on a feature branch |
| 3 Test | New cloud thread | Run focused SMC host suite; dispose failures |
| 4 Validate | New cloud thread | Elite/Pro DEV package identities if toolchain available |
| 5 Merge | New cloud / integrator | Merge validated lane |
| 6 Install | **Local** thread | Flash Elite DEV; device check of peek/hold/restore |
| 7 Push | Per workflow | Remote ref matches intended commit |

## Completion criteria for stage 1

- Defect and proposed fix recorded with file evidence.
- Concurrent activity checked; no conflicting implementation claimed.
- Successor stage-2 handoff written with base SHA, scope, defaults, and test expectations.
