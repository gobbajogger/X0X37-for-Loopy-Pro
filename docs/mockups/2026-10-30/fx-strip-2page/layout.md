# SMC FX strip, 2 pages (MOCKUP, not pushed)

Slot = page*4 + button. Button order REC, SOLO, MUTE, SELECT. Order = SMCFX_SUIT. `--` = empty.

| Bus | P1 REC | P1 SOLO | P1 MUTE | P1 SELECT | P2 REC | P2 SOLO | P2 MUTE | P2 SELECT |
|---|---|---|---|---|---|---|---|---|
| 909 | REV | DLY | STUT | SHUF | -- | -- | -- | -- |
| 808 | DLY | REV | STUT | SHUF | -- | -- | -- | -- |
| 303A (Miami) | HPF | DLY | REV | LPF | G8 | G16 | TRNC | -- |
| 303B (Chicago) | HPF | LPF | DLY | REV | G16 | G8 | -- | -- |
| BREAK | GATE | REV | DLY | HPF | -- | -- | -- | -- |
| REVERB | REV | G8 | G16 | TRNC | GATE | STUT | SHUF | -- |
| DELAY | DLY | STUT | SHUF | G16 | G8 | -- | -- | -- |
| MASTER | G16 | G8 | TRNC | STUT | SHUF | HPF | LPF | -- |

Note: in the popup grid the strip order is 909, 808, BRK, CHI, MIA, RVB, DLY, FLT (physical order). Each strip uses the bus that smc_fx_bus_now() maps it to.
The grid draws the rows top to bottom as M, S, R, then the new SEL row (row 4).

## Open questions (not decided)
1. Page flip: proposal HOME+LEFT/RIGHT (0x62/0x63). Today these keys switch the ROOM/TIGHT/ECHO presets (SMCFX_PAGE). Do the presets go away, move to another chord, or stay on HOME+LEFT/RIGHT while the page flips some other way (e.g. HOME+SELECT)?
2. HOME+button cycling (smcfx_cycle): with every suited FX on its own button, is cycling dropped? Or does HOME+button stay as a per-slot reassign?
3. In FX view, strips 5-7 R/S/M lose their nav roles (LIST/OK, GLOB/BACK, HOME/SONG, PART/SEQ, MIX). How do you leave FX view from the SMC? Keep one nav button? Use HOME only?
4. The TAP/short-press toggle (latch under 180 ms) on SELECT: should it apply to all 32 buttons?
5. Do the page 1/2 LEDs show which page is active? Which LED?
6. Held-LP behaviour (fx_lp_hold forcing LOW-PASS): with LPF on its own slot, is the forced LP removed? The mockup shows the real slot name when held.
