# SMK-37 REC37: 303 implemented, drum and BREAK acceptance pending

Owner report (2026-11-13 07:55 CST): in REC, only the keys matching the FM-1
layout record; the full SMK-37 3-octave keybed (37 keys) should record. A later
owner direction (same day) extends that to REC generally, including 909, 808
and BREAK. The FM-1 range is the symptom, not by itself the cause.

Original source: `94197fc8` (`lane/026-combined-20261112`). Existing implementation: `lane/fmb-rec-full-keybed@e81b2cf6`. Isolated continuation: `wip/rec37-compact-continuation-20261113` (PR #896).

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

## 909 and 808: outer keys currently unmapped

This is observed current source behavior, **not** a user-approved exemption from the 37-key REC requirement. The original 11-track positional mapping is preserved until a compatible explicit all-part mapping is agreed.

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
behavior. Extending the black-key count past 11 would invent voices. Neither
change is authorized by the existing source, and neither is implemented. Their
nonimplementation does not resolve the owner-requested all-key recording.

The 10 outer keys (C3, C#3, D3, D#3, E3, G#5, A5, A#5, B5, C6) currently remain
unmapped in drum REC: they do not record a hit, they do not select `ui.sel`,
they do not toggle a step, and they do not sound a drum voice. The middle 27
keep the existing map, unchanged. The feeder never calls `bass_kbd_physical`
or `drum_key` for them while a drum part is selected.

## BREAK

The middle 27 already have a complete map: white keys are the 16 steps; black
keys 1–8 are live slices (paged); black keys 9–11 are reverse, half-speed and
stutter (`BRK-08`). The existing implementation does not extend it, and so
BREAK outer-key recording is not accepted as complete.

The low outer keys already have a **separate**, 37-bit path that does not go
through `plat_keys`: `smk_frame` builds a 4-bit mask from `fm1_in.keys` and
calls `ui_brk_page_keys`. C3/D3 change slice count, C#3/D#3 change the slice
page; E3 is unassigned (comment at `ui_brk_page_keys`). This already runs
during REC. The feeder must not steal those keys or write steps and slices
from them.

G#5–C6 have no BREAK function in the current source. They remain unmapped for
step and slice writes, which is still a gap against the owner all-part request.

## Other keybed REC

SONG uses the same 27-key `KEY_WHITE` / `KEY_BLACK` tables (white = the bar's
pattern, black = part mutes). That map is 16 patterns and five parts. Outer
keys stay unmapped. External MIDI REC (`seq.rec_ext_mask`, `ext_rec_drain`) is
not the keybed and is unchanged. Pads still alias into the 27-bit window.

## Tests

`tests/host/rec_keybed_37_test.sh` is the current behavior regression: all 37 keys on a 303 (octaves
−2..+3, mono and chord, stopped and playing, note-on/off, ownership, REC exit,
octave change, suppress at entry); 909 and 808, all 11 voices and the 16 step
keys, with the outer ten writing nothing; BREAK steps and slices on the middle
27, paging on C3–D#3 still intact, outer keys writing no step or slice.
`freeplay_live_keys_test.sh`, `rec_live_test` and Free Play owner-switch case 9
must still pass. The drum/BREAK tests that assert the outer keys write nothing
measure the present implementation only: they are **not owner acceptance** of
all-part 37-key REC.

PR #896 also adds `tests/host/rec_outer_compact_test.sh`: its differential
host test compares the old 64-bit masks and packed 10-bit masks for 1,024
combinations and 250,000 deterministic REC-edge transitions. The source-level
mask storage changes from three `uint64_t` values to three `uint16_t`
values, but no linked flash, RAM or target timing saving is claimed without
exact-source Elite/Pro measurements. Their build gates remain **NOT_RUN** while
SDK V1.2.14 raw bytes are unavailable in this execution session.

### Acceptance status

- 303 bass REC, outer physical keys 0–4 and 32–36: implementation present;
  owner/play-state, freeplay and regression tests still require exact-head
  qualification.
- 909/808 REC: outer ten unrecordable under current 11-track positional map.
  Owner requirement remains **OPEN**; mapping requires a separate compatible
  decision without inventing a 12th drum voice.
- BREAK REC: low outer C3–D#3 already have paging functions, E3 and high
  outers have no REC mapping. Owner requirement remains **OPEN**; do not
  overwrite page controls or invent slice/step aliases.
- Existing negative drum/BREAK regression assertions cannot close either
  outstanding requirement. Source/PR delivery is not a firmware build or
  device acceptance.

## PR #896 exact-source REC/Free Play ownership host continuation

Test input: PR #896 at `2584b83d833c560f7acc9e900caa5fc351cf0613`; GitHub `firmware/src/app/ui.c`
blob `5af70e36fe02e05287760a8e182c80d828d650b9`.
Three unmodified production C ranges were retrieved through the GitHub
connector and checked against local exact text (length / FNV-1a):
`live_token + ui_live_key_owner` 894/`25710e1f`,
`rec_outer_keys` 1788/`4ccb3d0f`,
`input` 263/`7f944367`.
Using the same production-source snippets and GitHub-matched
`rec_outer_keymap.h` blob `a23a24ae6c2eb7f40f4b1b41eeadf208a9159b8a`,
the focused host integration compiled and ran:
- GCC: **PASS**, 4,304 assertions, 130 note-ons, 130 note-offs, 0 failures.
- Clang ASan+UBSan: **PASS**, same 4,304 / 130 / 130 / 0.
- GCC UBSan: **PASS**, same 4,304 / 130 / 130 / 0.

The committed `tests/host/rec37_owner_integration_test.sh` extracts
the live production functions directly from `ui.c` and compiles them
against a deterministic mock input/engine. Coverage: both 303 parts,
physical 0–36 and octaves -2 through +3, token/pitch identity, held
pairs, suppression at REC entry, proper release across REC/part/page/
clip/project/performance boundaries, no central-key theft, Free Play
owner eligibility, and REC input-dispatch mask. **This is source-extracted
component integration, not the full-application REC/Free Play scripts,
PCM identity, SDK build, or device hardware validation.** FNV values
guard against an accidental snippet mismatch; the Git blob pins the
remote source. Test scripts must be rerun after future source edits.

**Full-app host gates NOT_RUN on exact head:** `rec_keybed_37_test.sh`,
`freeplay_live_keys_test.sh`, `freeplay_latency_test.sh`,
`run_rec_live.sh smk37`, and affected ownership/lifecycle. These
require the entire source tree and consumed host input generators and
assets, which are not available from the current raw mounts. The GitHub
connector provides exact selected text blobs; the raw Library source ZIP
request to `files__materialize` returned: "This Project file does not
have an authorized raw-byte materialization path." Old Oct-23/Oct-30
source and precompiled 94197fc8 firmware do not replace current HEAD.

**Native SDK HOLD:** Files `files__materialize` returned the same
raw-byte error for the Library V1.2.14 SDK ZIP. Both target builds,
flash free >=8192 B per model, pool free >=8192 B per model, target
timing, and whole-app sound identity remain NOT_MEASURED. PR #893
modwheel's verified Elite/Pro builds are different source and are
flash-short by 516/508 B; never use their firmware bytes as REC proof.

**Producer handoff:** Obtain a real supported local file upload or
authorized container mount with the **raw current PR #896 source
archive** plus exact host generator inputs (not an indexed Project
reference). Verify all consumed file blobs and tree against the
GitHub connector's exact PR HEAD before testing. Run only the five
focused checks named above using the repository scripts and actual
test process exits; do not claim zero failures from a test not run.
Native builds additionally require a separately authorized,
physically accessible raw SDK V1.2.14 archive with verified component
hashes and JieLi Clang 4.0.1, and must embed the identical exact Git
source SHA. Log image sizes, flash/pool free, model identities and
hashes. Do not bypass materialization denial, substitute PR #893
artifacts, touch main/nightly or publish/flash.
**909/808/BREAK outer-key REC remains explicitly unimplemented and
awaits an approved mapping, not a reinterpretation of the old tests.**
