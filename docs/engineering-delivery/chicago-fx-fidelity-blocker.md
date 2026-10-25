# Chicago Fx Type / Edit 1 / Edit 2 — SEND FX bridge (FCDC-205)

Reason: Chicago factory banks name a per-voice insert FX strip (Type + two
edits), but the product has only a shared send bus. Silent stubs would mislead
players; a labeled Mix/fxbus bridge is the largest honest behaviour that fits.
Purpose: ship Off / Comp / Crush / Short·BPM Delay / Hall as a **SEND FX**
surface with explicit NOT_YET for Ring / Cho / Ensemble / Phaser / EQ / Gate
Reverb.
User effect: the 303 PART page gains **SEND FX** (Type, Edit 1, Edit 2). Choosing
Hall or Delay moves that part’s Mix sends and shared fxbus pots. Choosing Ring
(etc.) shows **NO INSERT** and clears that part’s sends — it does not pretend
to ring-modulate.

## Status

**APPROXIMATION SHIPPED — not insert fidelity.**

| Item | Value |
| --- | --- |
| Lane | FCDC-205 / LX-5 Chicago Fx chunk |
| Branch | `cursor/chicago-fx-34c0` |
| Base | `origin/cursor/stream4-combined-schema-53a1` @ `a692adfd` |
| Schema | `PROJ_SND_REV` 8; `NPARAMS_MAX` 28; pots `CHI_FX_*` at 23..25 |
| Module | `firmware/src/app/chicago_fx.h` |

## Honesty claims

1. **Not Chicago insert.** Shared `T_FX` + per-part `T_MIX` only.
2. **Page title is SEND FX**, not “Chicago Fx”.
3. **Supported map (real behaviour):**

| Type | Behaviour |
| --- | --- |
| Off | Inactive on load (keeps factory Mix). User Off clears that part’s MX_REV/MX_DLY |
| Comp | Global `FX_COMP` ← Edit1; clears part sends |
| Crush (Decimator) | Global `FX_DIST=Crush` + `FX_DRIVE` ← Edit1; clears part sends |
| Short Delay | Part `MX_DLY` ← Edit1; `FX_DL_TIME=1/32`; return level from Edit2 |
| BPM Delay | Part `MX_DLY` ← Edit1; `FX_DL_TIME` from Edit2 (0..12 divisions) |
| Hall | Part `MX_REV` ← Edit1; `FX_RV_DECAY` ← Edit2 |

4. **NOT_YET (enum kept, labeled with —):** EQ, Ring Mod, Cho/Flg, Ensemble,
   Phaser, Gate Reverb. Selecting them: UI `NO INSERT: <name>`, clear part
   sends only — **no** fake DSP.
5. **Global theft is labeled:** Comp / Crush / Delay time / Hall decay rewrite
   the one shared bus (all parts hear it).

## Remaining FX gaps

- True per-voice insert for all 12 types (XIP / DSP program).
- Gate Reverb, Ring, Cho/Flg, Ensemble, Phaser, insert EQ, bit-depth Decimator.
- Per-part Comp/Crush that does not steal the master stage.
- Automatic factory-bank Fx → Mix apply on load (intentionally **not** done:
  would silently rewrite global fxbus).

## Miami

Unchanged: Miami wobble stays **WBL1 / XIP**; coordinate with optimizer sibling.
Do **not** fake Miami FX onto Open303.

## Schema note vs arp sibling

Arp lane may also want indices after 22. This tip owns **23..25** for SEND FX
under `PROJ_SND_REV` 8. If arp lands first with 23..26, rebase FX to 27..29
(or serialize merges). Do not double-claim the same cells.
