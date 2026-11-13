# Flash budget — phase 2 overhaul (HOST ESTIMATE unless marked measured)

**Elite headroom:** 248 B above the 8192 floor. **Pro:** 256 B. Shared pool FMB measured at **148 B**. These figures are **measured** at earlier SHAs; Main Builder **re-measures at the implementation SHA** and does not reuse Oct 25 / PR188 numbers. Do not shrink the reserve without review. Do not silently evict.

All new figures below are **HOST ESTIMATE** except the chord-namer Tier 1 (285 B **measured**) and the headroom itself. Combined units + glyphs + namer already exceed Elite. **Measure at link.** Zero new factory slots is acceptable. The rejected §10 interval form is **not** in this budget.

## Running total against Elite 248 B

| Item | Bytes | Status | Notes |
|---|---:|---|---|
| Elite headroom (floor 8192) | 248 | **measured** | Pro 256. Shared with presets and the namer. |
| Chord namer Tier 1 | 285 | **measured** | **Already over Elite**, before anything in this overhaul. |
| Chord namer Tier 2 | +46 | HOST ESTIMATE | Exact-match extensions. |
| Chord namer Tier 3 exact names only | +82 | HOST ESTIMATE | `Cadd9`, `Cmaj11`, `C9`, … as **exact** matches. |
| §10 interval form (`C(3)`, `C(b2 2)`) | **0** | **rejected, do not ship** | Was ~200 B in §10 Rec. Not drawn, not budgeted. |
| §10 second reading | **0** | **not shipped** | Ambiguous sets show notes only. Was ~72 B. |
| Kind-specific SOUND map | 16 + 80–200 | HOST ESTIMATE | Table + dispatch. Add-only; do not flip `orig_strip_slot` alone. |
| Unique Juno/Drone 26×19 (WIDTH, HPF, SUSTAIN, REGISTER) | 200–300 | HOST ESTIMATE | Includes two 9×9 identities **if** they land; identities have no header slot today and are sheet-only in this overhaul. |
| Unsaved-never-timeout | **0** | policy | Existing badge, no second dirty flag. L’s 2 px rail ~20 B if drawn as new pixels, 0 B if Direction B’s signal bar is reused. |
| First-play hint string | 40–80 | HOST ESTIMATE | Static / `say()`. |
| S default-page-per-kind (Juno opens on the roll) | 20–40 | HOST ESTIMATE | S only. |
| Juno factory append, each | 32 | HOST ESTIMATE | 16 B pots + pointer + aligned name. `ORIGINAL_SOUND_BYTES` 32 is the **runtime** cell, not the table cost. |
| Drone factory append, each | 20 | HOST ESTIMATE | 6 B pots + pointer + name. |
| Recommended factory subset (Juno 4–8 + Horizon) | **180** | HOST ESTIMATE | SOFT, STR, STAB, BASS, LEAD + Horizon. |
| Full factory menu (Juno 4–11 + Horizon + contingent Foundation) | 276–296 | HOST ESTIMATE | Do not ship this next to a 285 B namer without a new measurement. |
| User bank / import | unmeasured | blocked on reserve | `presets-spec.md`. This overhaul does not invent a count. |
| §3A plugin compatibility | — | **BLOCKED** | Not claimed, not budgeted. |

## What already does not fit

285 (namer T1) > 248 (Elite). Adding T2 (+46) and T3 exact (+82) is **413 HOST** for names alone, before glyphs and factories. The previous §10 Rec ship set (T3 + interval + second) was **401–425 B on top** of T1; dropping interval and the second reading is the only namer-side cut this overhaul takes, and it is taken because the **owner rejected the interval form**, not only because of bytes.

## Ranked cuts (if the link is still over)

1. Keep WAVE, SUB, filter, tails, LFO, chorus, LEVEL, and ms/Hz text (reuse, dispatch only).
2. Drop unique WIDTH, REGISTER, HPF, SUSTAIN pictures if the functions do not fit; the kind-map can point those cells at the nearest reused painter.
3. Keep identities off the header (already done).
4. Ship **zero** new factory slots, or a prefix of the 180 B subset, after the reserve is measured. Never evict Chicago 81 or Miami 30. Never retune 0–3.
5. Do not implement T3 exact names if `Cadd9` can wait; T1 already names `Cmin7` and `C/E`.
6. Do **not** shrink the flash reserve to make a full menu fit.

## CPU / note-scan

- Chord name: once per held-step / onset, cached, invalidated on every input (`piano-roll.md` §9.8 F5). **Never** in the sequencer ISR. **Never** on the note-scan path. Stock KEYS-screen dropout is the warning (`smk-sentiment.md`).
- SOUND painters: idle, same cost as the existing 909 / Chicago strips. No per-sample work.
- Factory tables: `memcpy` of 16 or 6 bytes at init (`original_sound.c` 227–230). **0** additional ISR cost.
- No decorative transition delays.

## RAM

0 additional RAM if no new cache beyond the one chord-name slot already implied by §9.8 F5 (a few tens of bytes for the cached string and a generation counter). Declare it explicitly at implementation and measure.
