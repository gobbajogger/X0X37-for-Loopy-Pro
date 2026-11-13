# SMK-37 REC: all 37 keys, and what drum and BREAK do with the outer ten

Owner report (2026-11-13 07:55 CST): in REC, only the keys matching the FM-1
layout record; the full SMK-37 3-octave keybed (37 keys) should record. A later
owner direction (same day) extends that to REC generally, including 909, 808
and BREAK. The FM-1 range is the symptom, not by itself the cause.

Source: `94197fc8` (`lane/026-combined-20261112`). Lane: `lane/fmb-rec-full-keybed`.

## 303

The HAL already scans 37 keys (`FM1_NKEY`, `plat_physical_keys`). REC then
threw the ten outer keys away:

1. `ui_live_key_owner()` returns 0 when `ui.rec` (`ui.c`), so `live_boundary`
   drains and discards `smk_extq`.
2. The REC writer (`key_event` → `bass_kbd`) only sees `plat_keys()`, which is
   the 27-bit FM-1 window (`NKEYS 27`, physical 5–31, F3–G5).

The fix keeps both of those as they are (removing the `ui.rec` exclusion would
stop the working 27 keys from recording, and would fail Free Play owner-switch
case 9). A REC-only feeder, `rec_outer_keys()`, reads `plat_physical_keys()`
and, for physical 0–4 and 32–36 only, calls `bass_kbd_physical(p, 36+p+12*ui.oct)`
on a rise, with identity `live_token(p)` (28–37) in `kbd_held_key`. Already-held
keys at REC entry are suppressed. Outer keys sound through the same
`engine_bass_on_velocity` / `engine_bass_record` path as the central 27.

Admit only when `ui.rec && is_303() && kbd_on()`. Elite and Pro share this path.

## 909 and 808: outer keys stay unmapped

This is a decision from the code, not a guess.

- `NDRUM` is 11: "tracks per drum machine: the 11 black keys" (`seq/pattern.h`).
- `DRM-01` / `DRM-02` / `DRM-03`: one track on each of those 11 black keys, in
  left-to-right order (909: BD, SD, LT, MT, HT, RS, CP, CH, OH, CR, RD).
- The map is `KEY_BLACK[NKEYS]` and `KEY_WHITE[NKEYS]` with `NKEYS 27`. The
  inverse `BLACK_KEY[11]` is the FM-1 indices of those 11 keys. `drum_key(v, …)`
  takes a voice `0..10`. `drum_rec_write` rejects `v >= NDRUM`.
- The same pitch class already names **different** tracks in different octaves
  inside the 27-key window: on the 909, F#3 is BD (voice 0), F#4 is RS (voice 5),
  F#5 is RD (voice 10). So the rule is **position in the FM-1 window**, not
  pitch class, and not "the same voice an octave away".
- White keys are the 16 steps of the current page (`KEY_WHITE` values 0–15).
  F3–G5 has exactly 16 white keys. There is no 17th step in that page map.

Octave-shifting the outer ten onto those voices would be a **new** rule, and it
would clash with the existing "same pitch, different octave, different track"
behavior. Extending the black-key count past 11 would invent voices. Both are
rejected.

The 10 outer keys (C3, C#3, D3, D#3, E3, G#5, A5, A#5, B5, C6) therefore stay
unmapped in drum REC: they do not record a hit, they do not select `ui.sel`,
they do not toggle a step, and they do not sound a drum voice. The middle 27
keep the existing map, unchanged. The feeder never calls `bass_kbd_physical`
or `drum_key` for them while a drum part is selected.

## BREAK

The middle 27 already have a complete map: white keys are the 16 steps; black
keys 1–8 are live slices (paged); black keys 9–11 are reverse, half-speed and
stutter (`BRK-08`). That map is not extended.

The low outer keys already have a **separate**, 37-bit path that does not go
through `plat_keys`: `smk_frame` builds a 4-bit mask from `fm1_in.keys` and
calls `ui_brk_page_keys`. C3/D3 change slice count, C#3/D#3 change the slice
page; E3 is unassigned (comment at `ui_brk_page_keys`). This already runs
during REC. The feeder must not steal those keys or write steps and slices
from them.

G#5–C6 have no BREAK function. They stay unmapped for step and slice writes.

## Other keybed REC

SONG uses the same 27-key `KEY_WHITE` / `KEY_BLACK` tables (white = the bar's
pattern, black = part mutes). That map is 16 patterns and five parts. Outer
keys stay unmapped. External MIDI REC (`seq.rec_ext_mask`, `ext_rec_drain`) is
not the keybed and is unchanged. Pads still alias into the 27-bit window.

## Tests

`tests/host/rec_keybed_37_test.sh` is the gate: all 37 keys on a 303 (octaves
−2..+3, mono and chord, stopped and playing, note-on/off, ownership, REC exit,
octave change, suppress at entry); 909 and 808, all 11 voices and the 16 step
keys, with the outer ten writing nothing; BREAK steps and slices on the middle
27, paging on C3–D#3 still intact, outer keys writing no step or slice.
`freeplay_live_keys_test.sh`, `rec_live_test` and Free Play owner-switch case 9
must still pass.
