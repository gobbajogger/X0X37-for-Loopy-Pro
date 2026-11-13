# Juno and Drone factory presets — proposal for Main Builder

**Status:** PROPOSAL. Not implemented. Not a firmware patch. Not a shipping character claim.
**Baseline:** `94197fc839b00aff47b98ca02f3c6396e4a6eabf`.
**Role:** sound-design, add-only. Written under `/workspace/smk-ux/overhaul/presets/` only.
**Rule:** existing indices **0–3 stay unchanged** (name, pots, meaning). New roles, if any ship, **append** at index **4** and up. Old projects that point at 0–3 load unchanged (`presets-spec.md` §5.1, §6).
**Evidence:** every new index remains **intent only, pending render evidence** (`presets-spec.md` §5.3). Main Builder supplies the WAV package; BugBot checks it. This file does not claim heard character.
**Rights:** every proposed append is an **original** parameter program composed for this proposal. It is **not** copied from Chicago or Miami factory rows, and **not** copied from a KORG JUNO or string-machine pack. Main Builder records provenance (`presets-spec.md` §1.3) and completes a rights trace **before** any index ≥ 4 ships. Factory-export stays default OFF until that trace.
**Plugin compatibility (§3A):** not claimed. BLOCKED.
**Flash:** Elite headroom is **248 B** measured (Pro **256 B**), already competed with the chord namer (**285 B measured**, over Elite). Counts below are **HOST ESTIMATE**. Zero new slots is acceptable. A subset is acceptable. **Measure the reserve at the implementation SHA** before any append ships. Do not reuse Oct 25 / PR188 figures.

---

## 1. Existing four + four (do not retune)

Source: `firmware/src/app/original_sound.c` at `94197fc8`. Juno rows are 16 bytes (parameters 0–15). Drone rows are 6 bytes (parameters 0–5). Pitch bend and damper are **not** in the factory row; they use the param-table defaults (`original_sound_juno_params[16]` PBend centred, `[17]` Damper 0; Drone PBend index 6 default 64, Damper 7 default 0 — `original_sound.c` 31–49, `original_sound_centered` 69–77). Instantiated cell size is `ORIGINAL_SOUND_BYTES` **32** (`original_sound.h` 8). Counts: `ORIGINAL_JUNO_FACTORIES 4`, `ORIGINAL_DRONE_FACTORIES 4` (`original_sound.h` 15, 21).

Parameter **ranges** are 0..127 for every Juno pot (`original_sound_juno_params`, `original_sound.c` **31–36**). Drone pots are also 0..127 (`original_sound_drone_params`, **45–49**); table defaults for the six audible pots are the Cathedral values.

DSP laws used to report times (HOST, not a device measurement):

| Law | Source | Equation (`x` = pot / 127) |
|---|---|---|
| Juno WAVE mix | `juno6.c` **41**, `juno6_set` **68** (`macro[0]=x`; audio is `saw*(1-macro[0])+pulse*macro[0]`) | continuous saw↔pulse. **Not** a SAW/PUL switch. (`waveform=(uint8_t)(x*2.99f)` at **70** is a leftover field; the mixer uses `macro[0]`.) |
| Juno SUB | `juno6_set` **71** | `sub_level = x * 0.5` |
| Juno cutoff Hz | `juno6.c` **25**, set **72** | `35 + cutoff² · 12000` |
| Juno attack | `juno6_set` **74** | `0.002 + x² · 3` s |
| Juno decay | `juno6_set` **75** | `0.01 + x² · 3` s |
| Juno sustain | `juno6_set` **76** | `x` |
| Juno release | `juno6_set` **77** | `0.01 + x² · 4` s |
| Juno HPF | `juno6.c` **27**, set **78** | `15 + 2000 · hpf²` Hz in the one-pole |
| Juno LFO rate | `juno6.c` **28** | `0.1 + 8 · macro[10]²` Hz |
| Juno WIDTH duty | `juno6_set` **79** | `0.1 + x · 0.8` |
| Juno chorus | `original_runtime.c` **101**, `juno6_chorus_set` `juno6.c` **59** | rate `0.1 + 4.9 · macro[13]` Hz (clamped 0.1..5); depth fixed 5.4 ms; mix `0.5 · macro[14]` (clamped 0..0.5) |
| Drone registration | `drone_string.h` **64–66** | three ranks, continuous in `macro[0]`, not a 1-of-n switch |
| Drone tone | `drone_string.h` **67** | `0.012 + bright² · 0.48` |
| Drone attack | `drone_string.h` **68** | `0.004 + attack² · 2` s |
| Drone release | `drone_string.h` **69** | `0.08 + release² · 4` s |
| Drone level | `drone_string.h` **70** | `output = 0.24 · macro[5]` |

### 1.1 Juno 0–3 — documentation only, no retune

Bytes from `original_sound_juno_factories` (`original_sound.c` **38–43**), names from `original_sound_juno_names` (**44**). Order: Wave, Sub, Cutoff, Res, EnvAmt, Attack, Decay, Sustain, Release, HPF, LFORate, LFODep, Width, ChoRate, Chorus, Level.

| Index | Name | Role vs §5.2 (pending evidence, **unmapped until BugBot**) | 16 pots | HOST DSP at those pots |
|---|---|---|---|---|
| 0 | JUNO POLY | General mid-attack poly. Not a dedicated pad, pluck, or brass. Do **not** treat as J-PAD / J-PLUCK / J-BRASS. | `{42,28,70,18,45,8,40,92,40,0,15,8,64,25,45,100}` | atk **14 ms**, dcy **308 ms**, rel **407 ms**, cut **3681 Hz**, chorus 45 (mix ≈ 0.18), cho-rate **1.06 Hz**, wave mix **33 %**, width duty **50 %** (pot 64) |
| 1 | JUNO PAD | Candidate for **J-PAD** (sustained, moderate-slow attack, chorus on). Do not append a second pad until evidence says this fails J-PAD. | `{90,18,42,24,35,65,55,95,75,0,12,15,55,40,55,100}` | atk **788 ms**, dcy **573 ms**, rel **1.41 s**, cut **1347 Hz**, chorus 55, cho-rate **1.64 Hz** |
| 2 | JUNO PLUCK | Candidate for **J-PLUCK** (atk 0, short release, chorus as seasoning). Do not append a duplicate pluck. | `{15,50,58,36,80,0,35,30,20,5,20,0,64,25,18,110}` | atk **2 ms**, dcy **238 ms**, rel **109 ms**, cut **2538 Hz**, chorus **18** (mild), cho-rate **1.06 Hz** |
| 3 | JUNO BRASS | Candidate for **J-BRASS** (fast attack, bright cutoff, filter-env modest, chorus on). Do not append a duplicate brass. | `{70,10,105,12,20,0,20,110,15,12,25,5,80,60,50,96}` | atk **2 ms**, dcy **84 ms**, rel **66 ms**, cut **8238 Hz**, chorus 50, cho-rate **2.41 Hz** |

### 1.2 Drone 0–3 — documentation only, no retune

Bytes from `original_sound_drone_factories` (`original_sound.c` **51–54**), names from **55–57**. Order: Register, Brightness, Flutter, Attack, Release, Level.

| Index | Name | Role vs §5.2 (pending evidence, **unmapped until BugBot**) | 6 pots | HOST DSP |
|---|---|---|---|---|
| 0 | Cathedral | Candidate for **D-ENS** (mid register 57, bowed-ish 70 ms attack, ensemble flutter 51, 1.8 s release). | `{57,66,51,23,83,95}` | atk **70 ms**, rel **1.79 s** |
| 1 | Cloud | Candidate for **D-WASH** (lower 35, darker 39, slower 421 ms, long 3.0 s). Not as slow as a dedicated D-DRONE. | `{35,39,38,58,108,105}` | atk **421 ms**, rel **2.97 s** |
| 2 | Aurora | Candidate for **D-AIR** (high register 94, bright 97). | `{94,97,70,17,72,83}` | atk **40 ms**, rel **1.37 s** |
| 3 | Organ | Organ-like (fast 7 ms, low flutter 12). Not a §5.2 Drone role; do not retune it into one. | `{73,75,12,5,55,94}` | atk **7 ms**, rel **830 ms** |

Main Builder still owes the §5.3 documentation package for these eight. Those packages **do not authorise a retune**.

---

## 2. Proposed Juno appends (indices 4–11)

A **menu** of distinct §5.2 roles that indices 0–3 do **not** already cover, in recommended append order. This is **not** a mandate to ship all eight. Final number follows the flash-reserve measurement and render evidence. If JUNO PAD / PLUCK / BRASS pass J-PAD / J-PLUCK / J-BRASS, those roles are **not** appended again (they are not in this list).

Pots are in the same 16-byte order as `original_sound_juno_factories`. Every value is in 0..127 (`original_sound.c` **31–36**). Chorus-defined roles use Chorus **127** (mix at the `0.5 · macro[14]` ceiling, `original_runtime.c` **101**), which is clearly above JUNO PAD’s 55.

| Slot | Name | Role id | Musical role | 16 pots (Wave..Level) | Why it is distinct | HOST DSP at those pots |
|---|---|---|---|---|---|---|
| **4** | JUNO SOFT | J-SOFT | Slow swell pad. Arrives into a held chord; chorus on. | `{85,22,48,20,28,102,50,100,95,0,10,12,55,38,62,100}` | Attack **1.94 s** vs JUNO PAD **788 ms** — clearly slower, as §5.2 requires. | atk **1.94 s**, dcy **475 ms**, rel **2.25 s**, cut **1749 Hz**, chorus 62, cho-rate **1.57 Hz** |
| **5** | JUNO STR | J-STR | Bowed-ish ensemble strings. Mid-to-upper mid, chorus on, not a brass snap. | `{28,35,72,16,30,55,48,100,80,0,8,10,60,42,70,100}` | Attack **565 ms** (slower than STAB/BRASS 2 ms, not a swell). Wave more saw (22 %) than PAD’s 71 %. Chorus 70. | atk **565 ms**, rel **1.60 s**, cut **3892 Hz**, chorus 70, cho-rate **1.72 Hz** |
| **6** | JUNO STAB | J-STAB | Short poly chop. Bright, short release; not a held pad. | `{50,8,108,22,70,0,18,20,8,4,0,0,64,30,40,112}` | Sustain 20 and release **26 ms** vs PAD sustain 95 / **1.41 s**. EnvAmt 70 for a snap JUNO BRASS (EnvAmt 20) does not take. | atk **2 ms**, dcy **70 ms**, rel **26 ms**, cut **8713 Hz**, chorus 40 (present, not a wash) |
| **7** | JUNO BASS | J-BASS | Low bass. Tighter than the pads; chorus present but not washing the low end. | `{20,80,38,28,40,4,32,70,22,8,6,4,70,18,22,110}` | Sub **80** (63 %) and cutoff **1109 Hz**, vs POLY sub 28 / 3681 Hz. Chorus only 22. | atk **5 ms**, dcy **201 ms**, rel **130 ms**, cut **1109 Hz**, chorus 22, cho-rate **0.79 Hz** |
| **8** | JUNO LEAD | J-LEAD | Playable mid line. Clearer than the pads, shorter release, chorus on, some vibrato. | `{55,12,88,14,40,6,36,80,32,2,22,18,58,32,48,104}` | Release **264 ms** vs PAD **1.41 s**. Cutoff **5797 Hz**, LFO rate **0.34 Hz** depth 18. | atk **9 ms**, dcy **251 ms**, rel **264 ms**, cut **5797 Hz**, chorus 48, cho-rate **1.33 Hz** |
| **9** | JUNO CHO | J-CHO | Chorus-**defined** lush mid texture. A held chord without that chorus would fail the role. | `{40,25,58,14,22,40,50,105,70,0,8,6,62,48,127,96}` | Chorus **127** (mix at the 0.5 ceiling) vs PAD 55 / POLY 45. Moderate attack **300 ms**, not a pluck and not a brass snap. | atk **300 ms**, rel **1.23 s**, cut **2538 Hz**, chorus **127**, cho-rate **1.95 Hz** |
| **10** | JUNO HI | J-CHO-HI | Chorus-defined **bright** texture. Higher than J-CHO; still a texture, not J-LEAD and not J-PLUCK. | `{50,10,98,10,18,36,45,100,65,18,14,10,58,52,127,92}` | Same chorus 127, cutoff **7180 Hz** and HPF 18 vs J-CHO’s 2538 Hz and HPF 0. | atk **243 ms**, rel **1.06 s**, cut **7180 Hz**, chorus **127**, cho-rate **2.11 Hz** |
| **11** | JUNO ORG | J-ORG | Even mid organ bed. Little filter-envelope snap; chorus on; held live chords. | `{8,45,68,8,8,2,12,127,28,0,0,0,50,28,55,108}` | EnvAmt **8** vs BRASS 20 and STAB 70. Sustain **127**, almost-saw wave (6 %), no LFO. | atk **3 ms**, dcy **37 ms**, rel **204 ms**, cut **3475 Hz**, chorus 55, cho-rate **1.18 Hz** |

PBend / Damper are omitted, same as the existing rows — they stay at the table defaults (centred / 0).

---

## 3. Proposed Drone appends

Existing 0–3 already sit near D-ENS, D-WASH, D-AIR, and an organ. **Do not append duplicates.** The only clearly missing §5.2 role is a **very slow** D-DRONE (Cloud’s 421 ms attack is slow, but not “very slow” against a 1 s+ swell). Foundation is **contingent**: ship only if Cloud’s evidence package **fails** D-WASH.

| Slot | Name | Role id | Musical role | 6 pots (Register..Level) | Why it is distinct | HOST DSP |
|---|---|---|---|---|---|---|
| **4** | Horizon | D-DRONE | Sustained drone. Very slow attack, long hold, ensemble character, little sharp transient. The stab render may simply be a truncated swell. | `{48,50,55,95,120,100}` | Attack **1.12 s** vs Cloud **421 ms** and Cathedral **70 ms**. Mid register 48 (not Cloud’s low wash 35, not Aurora’s 94). Flutter 55 for ensemble. | atk **1.12 s**, rel **3.65 s** |
| **5** (contingent) | Foundation | D-WASH | Low, slow, muted, long bed — **only if** Cloud fails D-WASH evidence. If Cloud passes, **do not ship this slot** (do not pad the count). | `{18,22,28,80,124,100}` | Register **18** and brightness **22**, lower and darker than Cloud 35/39. Attack 798 ms, release 3.89 s. | atk **798 ms**, rel **3.89 s** |

---

## 4. Flash / size HOST ESTIMATE

Factory data lives in `.rodata` as the compact arrays, plus a name string and a pointer in the names table. The 32-byte `ORIGINAL_SOUND_BYTES` cell is the **runtime** image (`original_sound.c` **209–233**), not the table cost. Estimates assume Thumb-2, 4-byte pointers, names 4-byte aligned, no provenance payload (provenance is a record Main Builder keeps; if it is stored on-device, add that measurement).

| Item | Formula | HOST ESTIMATE |
|---|---|---|
| One Juno append | 16 B pots + 4 B name pointer + aligned name (`"JUNO SOFT\0"` is 10, aligns to 12) | **32 B** typical (range 28–36 B) |
| One Drone append | 6 B pots + 4 B pointer + aligned name (`"Horizon\0"` is 8) | **20 B** typical (range 16–24 B) |
| Bump `ORIGINAL_JUNO_FACTORIES` / `ORIGINAL_DRONE_FACTORIES` | already a `#define` | **0 B** |
| Dispatch / range checks that already use `original_sound_factories(kind)` | no new code if they already walk the count | **0 B** (confirm at review) |
| **Full Juno menu (8)** | 8 × 32 | **256 B** |
| **Horizon only** | 1 × 20 | **20 B** |
| **Full Juno + Horizon** | 256 + 20 | **276 B** |
| **Full Juno + Horizon + Foundation** | 276 + 20 | **296 B** |
| **Recommended first-ship subset** (if the measured reserve allows any): Juno 4–8 (SOFT, STR, STAB, BASS, LEAD) + Horizon | 5 × 32 + 20 | **180 B** |
| J-CHO, J-CHO-HI, J-ORG | 3 × 32 | **96 B** more, only after the subset and the namer/glyph budget are measured |

Elite **248 B** and the **285 B** namer already collide. **276 B** for a full menu does not fit next to a namer that is already over. Main Builder measures, then picks: zero, the 180 B subset, or a shorter subset. Do not shrink the flash reserve to make the full menu fit. Do not evict Chicago (81) or Miami (30).

CPU: factory tables are init-time `memcpy` of 16 or 6 bytes (`original_sound.c` **227–230**). **0** additional ISR or note-scan cost. Audition/load stay on the existing preset path (must not stall the note scan — `smk-sentiment.md` KEYS-screen dropout is the warning).

---

## 5. What Main Builder does with this

1. Do **not** implement from this file until the flash-reserve measurement at the implementation SHA is in, and until a rights/provenance record exists for each chosen append.
2. Keep indices 0–3 byte-identical.
3. If a role is taken, append in the slot numbers above (skipping any unused role so later numbers stay as this table — **or**, if a role is dropped, still **do not reuse** a skipped index for a different role; leave a hole, or document a new map). Preferred: ship a prefix of the recommended subset (4..k) so there are no holes.
4. Supply the §5.3 evidence package (held 4 beats at 120 BPM, Juno C4 / Drone C3, stab eighth, chorus listen, arp run for no J-PLUCK append). This proposal is **not** that package.
5. BugBot accepts or rejects **character**, not the pot table alone. A chorus-defined role that does not *sound* chorus-defined fails, even at Chorus 127.
6. Mark shipping names, slots, and provenance in the manual only after a pass. Until then this directory is a **proposal**.

