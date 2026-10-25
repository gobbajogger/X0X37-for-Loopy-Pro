# Chicago Fx Type / Edit 1 / Edit 2 — fidelity blocker (FCDC-205)

Reason: Chicago factory banks name a per-voice insert FX strip (Type + two
edits), but the product has only a shared send bus. Shipping stub pots or a
silent global rewrite would mislead players.
Purpose: record why product FX fidelity cannot land in this PR without a large
XIP / DSP program, and what must stay deferred.
User effect: Chicago presets keep their Fx names in the bank tables; the
device does not claim insert FX (Ring Mod, Cho/Flg, Ensemble, Phaser, Gate
Reverb, true Decimator/EQ) that the firmware cannot run.

## Status

**BLOCKED — no firmware change in this PR.** Docs/evidence only.

| Item | Value |
| --- | --- |
| Lane | FCDC-205 / LX-5 Chicago Fx chunk |
| Branch | `cursor/chicago-fx-34c0` |
| Base | `origin/cursor/stream4-combined-schema-53a1` @ `a692adfd` (includes #281 Bite + amp ADSR) |
| Outcome | Honest blocker; parent opens PR from this tip |

## Chicago enum (canonical)

| i | Type |
| ---: | --- |
| 0 | Off |
| 1 | EQ |
| 2 | Compressor |
| 3 | Decimator |
| 4 | Ring Mod |
| 5 | Cho/Flg |
| 6 | Ensemble |
| 7 | Phaser |
| 8 | Short Delay |
| 9 | BPM Delay |
| 10 | Hall Reverb |
| 11 | Gate Reverb |

Factory content already stores names and values as Chicago AU indices 17–19
(`firmware/src/app/factory_banks.c`, `assets/factory_banks/chicago.json`).
Those three slots are **bank metadata**, not `BASS303_*` pots and not wired
into the insert path.

## Architecture mismatch

Chicago: **per-voice insert** before the part leaves the voice.

Product today:

- Shared `T_FX` voice 0 — `firmware/src/dsp/fxbus.{h,c}` (reverb, tempo delay,
  master drive/comp/dist).
- Per-part sends — `T_MIX` `MX_REV` / `MX_DLY` (`firmware/src/app/engine.h`).
- UI — `V_FX` (REVERB + DELAY) and `V_MIX` (PARTS sends + MASTER).

There is **no** per-303 insert chain.

### Honest map onto existing fxbus (approximation only)

| Chicago | Nearest product | Honesty |
| --- | --- | --- |
| Off | Clear that part’s `MX_REV` / `MX_DLY` | Mute-sends OK |
| Compressor | `FX_COMP` / master glue | Global, post-mix — not insert |
| Short / BPM Delay | `FX_DL_*` + part `MX_DLY` | One shared delay line |
| Hall Reverb | `FX_RV_*` + part `MX_REV` | Shared hall-ish; not gated |
| Decimator | `FX_DIST` Crush + Drive | Master stage, all parts |
| EQ | `MST_DJF` / master filter | Global tone, not insert EQ |

### No product DSP (would be silent stubs if exposed as Type)

Ring Mod, Cho/Flg, Ensemble, Phaser, Gate Reverb, true insert EQ, bit-depth
Decimator, and short un-synced insert delay. Confirmed absent from
`firmware/src/dsp` (no ring / chorus / flanger / ensemble / phaser / gate
reverb modules).

A Type enum that only drives Mix/fxbus for Delay/Hall/Comp and leaves the
rest as Off would still **not** be Chicago insert fidelity. Loading a factory
Type onto one part must not silently rewrite the global bus unless UX makes
that theft explicit. This PR refuses both silent stubs and a half-mapped
send bridge labeled as Chicago Fx.

## Why fidelity needs a large XIP program

Faithful strip ≈ 12 algorithms on the dry 303 path (ideally A and B), plus
delay/reverb state for Short/BPM/Hall/Gate and LFOs for Cho/Flg/Ensemble/
Phaser/Ring.

Live WBL1 alone already **XIP FAIL** in the ~2.9–3.3 KB class (PR #267 /
FCDC-182; Miami pin also recorded XIP FAIL ~1788 B). App code margin is
routinely hundreds of bytes to low KB; controller flash margin has been
measured near **44 B**. A new insert FX family does not fit as a param-wire
PR.

Verdict: product Chicago Fx fidelity is a **DSP + flash budget program**, not
this FCDC-205 wiring lane.

## Schema / NPARAMS headroom

Assignment allowed stacking on amp ADSR only if Fx needs new 303 pots **and**
that tip has `NPARAMS` headroom. Amp ADSR + Bite are now **on** stream5
(`#281` / `PROJ_SND_REV` 7).

| Tip | `BASS303_NPARAMS` | `NPARAMS_MAX` | Free indices | Need for Type+Edit1+Edit2 |
| --- | ---: | ---: | --- | ---: |
| stream5 @ `a692adfd` | 23 (0–22: …Bite, AAttack…ARelease) | 24 | **23 only (1)** | 3 → **short by 2** |

No headroom for three new `BASS303_*` pots without another `NPARAMS_MAX` /
`PROJ_SND_REV` bump. This lane does **not** widen schema for stubs.

## Miami wobble — confirmed out of scope

Miami sound params (Carrier / XMod / Wobble Shape·Depth·Rate / Filter Crush /
Stereo / …) target **WBL1**, not native303 and not Chicago Fx.

- Live WBL1 on product tip: **XIP FAIL**; ENGINE select toasts
  `WBL1 NOT IN THIS BUILD` where that path exists.
- Do **not** fake Miami pots onto Open303.
- Miami 303 factory bank (#276) remains content-only.

Unchanged disposition. See `docs/engineering-delivery/miami-engine-switching-ux.md`.

## What this PR ships

1. This blocker note under `docs/engineering-delivery/`.
2. `evidence/chicago-fx-34c0/BLOCKER.md` with the same decision and proof plan
   result for the parent PR body.

## What this PR does not ship

- `BASS303_FX*` pots or UI pages that imply insert DSP.
- Send-bridge that remaps factory Type into global `T_FX` without operator
  consent.
- Any Miami / WBL1 param wiring.
- Amp Env ADSR, Arp Switch/Mode/Range/Speed (other FCDC-205 chunks).

## Resume conditions

Resume only when one of these is an assigned program:

1. **Flash/XIP program** that budgets a real per-voice (or pre-send) insert
   family and proves Elite+Pro package margins, or
2. **Product decision** for a labeled “send FX” approximation (subset Type,
   Mix/fxbus only, explicit global-theft UX) with its own schema/UI lane after
   amp ADSR lands and `NPARAMS` headroom exists.

Until then: bank names stay; no fidelity claim.
