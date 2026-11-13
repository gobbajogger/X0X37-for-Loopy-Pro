# DnBOX SMK-37 visual and interaction directions

Three coherent directions for the SMK-37 Elite and Pro, evolved from the existing
source-faithful frames in `/workspace/smk-ux/mockups/` and from Dot’s six-screen
journey in `/workspace/handoff-dot-20261113/firmware/`. They are design proposals
for the owner to review. **No firmware was edited, built, flashed or tested.
Nothing here was verified on hardware.** SHIPPED frames are reconstructions of
commit `94197fc839b00aff47b98ca02f3c6396e4a6eabf`, not device photographs.

Spell: **DnBOX**. Chicago is the 303 engine. Miami is never called a 303 or 303B.

## Shared commitments (all three directions)

- **Gestures and live meanings are unchanged.** Decision-record items 1 and 5–10
  stand: PATCH tap still toggles HOME/SONG; NOTE REPEAT is unchanged; KNOB BANK
  is still a single press; BT timings stay 250/400 ms; the header mode word stays
  `SEQ`; nothing is removed. No remaps, no tutorial layer, no LED layer, no new
  save behaviour (display only).
- **Items 2, 3 and 4** are no longer owner decisions to settle first. Each
  direction makes its own choice, all three picking 2A / 3A / 4A (show the SEQ
  footer from boot; show the existing `SOUND` word and keep the clock; show full
  save words and never cover the clock). The *execution* is what differs.
- **The 12-row kit overview is kept.** The 2×8 one-track grid from Dot’s original
  02 is not used (Dot’s own README already said this must not delete the kit).
- **Strip, type and colour.** Native 240×240, real 20+69+69+82 strips, Barlow
  roles T 10 / XS 12 / S 15 / B 15 / M 23, RGB565-safe colours, checked at 1×.
  Repo TTFs, not the packed felucca bitmaps; advances may differ by a few pixels.
- **Source strings only** for toasts and hints: `SAVES WHEN STOPPED`,
  `SAVE AT STOP`, `SAVED`, `STEP n  < >: CHANCE  PARA`, `BLACK: TRACK`,
  `WHITE: STEP`, `SEQ: STEPS`, `SOUND`, `SEQ`. No “PROJECT SAVED”, no “SAVE
  QUEUED”, no “DnBOX” title, no “EDITING: THIS STEP ONLY”.
- **User reports** in the plan (Deluge, M8, Digitakt) are unverified third-party
  commentary, relayed by Dot, not fetched in this pass. They do not outrank
  `ui.c`. Negatives are weighed as fully as positives. No product’s styling is
  copied.

Sheets: `/workspace/smk-ux/directions/{A,B,C}/sheet.png` (frames at real 240×240)
and `sheet-2x.png`. Comparison: `overview.png`. Individual frames:
`frames/1x/`, `frames/2x/`. Element-by-element source cites:
`source-mapping.md`.

The journey on every sheet, in this order: boot / first sound, a pattern (full
kit), sound editing, one-step editing, save while playing (deferred), saved,
live play. Save while stopped uses the same `SAVED` confirmation as a finished
queued save.

---

## Direction A — Plain speech

**Concept.** The firmware already wrote the right words. This direction shows
them, on every screen, as one sentence: where you are, what the keys do, and
whether a save is waiting. Selected, playing and queued keep the marks the
firmware already draws. It is the existing evolved proposal, completed across
the whole journey and shown without developer notes.

**Choices 2 / 3 / 4.** 2A, 3A, 4A, executed by surfacing strings and a header
path that already exist. No new marks.

### Choices

| Choice | Task | Audience | Reason | Tradeoff | Reference |
|---|---|---|---|---|---|
| SEQ footer from boot (`BLACK: TRACK / WHITE: STEP / PATCH+BLK: MUTE`) | First sound, then a pattern | Newcomers, and anyone on a factory or reset project | At boot the useful footer is hidden behind `HOLD ANY BUTTON` until a button has been pressed (`ui.c` 6738–6740, 6783–6786). A black key already plays BD while stopped (3193–3213). Naming the keys from the first moment makes that path obvious, without a tutorial. | The hold-help invitation is no longer the first line. Hold-help itself still opens at 900 ms (1909). | Deluge “blank defaults”: an empty pattern stays honest; the silence-or-confusion after a reset is the harm. M8 “task-based teaching”: teach the first task, don’t add a tour. SMK source: the strings are already written. |
| `909` chip + `SOUND`, pattern and tempo kept | Telling sound-edit from step-edit; staying oriented in a live pass | Anyone who presses PARA; a demo where the clock matters | Today’s compact PARA header (5373–5408) drops the mode word, the pattern and the tempo. The word `SOUND` is already at 5437 and is skipped. The same compact path makes sound and a held-step look almost the same at the top of the screen. | A busy header can no longer fit a full `UNSAVED` word (it would squeeze to `*`). The 2 px amber dot (5501) remains. This is not a new mode. | M8 “consistency once learned”: the same header skeleton as SEQ. Source-map problem on context shifts. Dot’s intent on 03, without the “EDITING: SOUND” banner (not in source). |
| Footer `BLACK: SELECT / WHITE: SOUND / SEQ: STEPS` on PARA | Finding the way back to the step grid | Newcomers who entered PARA | Those strings are already at 6776–6779 but are not drawn (`draw_knobs` returns first, 8337–8343). `SEQ: STEPS` is the existing way back, not a new gesture. | The arrow / `1-3` pictograms share the 13 px hint. Bank range `1-3` is kept on the right. | M8 task teaching, without a tutorial layer. Decision 8: the header word stays `SEQ`; `SEQ: STEPS` is the existing footer, not a rename. |
| Held-step tell is the existing amber line `STEP n  < >: CHANCE  PARA` | Edit one step without an accidental mode change | Anyone holding a step into PARA | The line is already at 5803–5806. Combined with a `SOUND` header, the comparison to the sound-edit screen is the presence of that line, not a new banner. `P-LOCK` wording and `RELEASE STEP FIRST` (2755–2759) stay. | The tell is a 10 px line. It is easy to miss in a hurry, which is why B and C add a stronger waiting mark. | Digitakt “unclear scope”: name the object (`STEP 5`), don’t invent “THIS STEP ONLY”. SMK source. |
| Pickup-style left chip for `SAVES WHEN STOPPED`, `SAVE AT STOP`, `SAVED` | Save while playing, and confirmation | Live players, and a demo | A full-header toast (5345–5369) covers pattern, tempo and the play icon. After it, on a 909+SEQ header, `SAVE AT STOP` shrinks to `Q` (5458–5460). Fader pickup already keeps the clock (5410). Reuse that rule, with the firmware’s own words. Flash still waits until stop (2222–2294, 8783). | During the message, the `909` chip and `SEQ` word are temporarily replaced by the words. The clock stays. No `Q`. | Digitakt “accidental loss” and “unclear scope”: queued (`SAVE AT STOP`) and written (`SAVED`) must look different, and neither may look like a write that has already happened. Deluge upgrade worries: stream numbers and write rules are untouched. |
| Kit overview, play icon, playhead, pattern cue, `CHN` | Pattern writing and live play | Everyone | These already work. A does not redraw them. | The dim play icon when stopped can still be read as “playing” (source map §3.4). B addresses that; A does not, to avoid a mark experts will notice. | SMK `draw_drum` 5511–5549; play icon 5495–5498. Dot README: do not delete the kit overview. |
| HOME keys still do not play a sound | First sound after an accidental PATCH | Newcomers who left SEQ | Making HOME keys play 909 would break `WHITE: PATTERN` and `BLACK: MUTE`. Forbidden without the owner. A keeps SEQ as the obvious first-sound place, and names the keys there. | After one PATCH, keys are still silent. PATCH still toggles HOME/SONG (decision 1C). | M8 “instant pad improvisation”: the want is real; on this instrument those keys have other jobs. SMK 3733–3787. |

### Risks to live play and muscle memory

- **Muscle memory:** none for gestures. Experts will see a `SOUND` word and a
  save chip they did not have; the buttons do the same thing. BT tap, 250/400 ms
  copy and help, hold-step p-locks, and PATCH+SEQ REC / SEQ PLAY are untouched.
- **Live play:** header blit only, still the 20 px strip. The clock staying
  during a save is a gain for a demo. Audio, flash-during-play, and live-note
  ownership (`ui_live_key_owner` 4279–4297, `live_boundary` 4358–4376) are not
  touched.
- **Residual risk:** of the three, A is the closest to “small footer and header
  tweaks,” which is what Dot’s review said was not enough. The journey is
  complete and consistent, but the visual system is the firmware’s existing one,
  revealed rather than redesigned.

---

## Direction B — Live signals

**Concept.** One family of marks you can read at a glance, even mid-performance.
A **fill** is selected, or playing, or (briefly) confirmed. An **amber outline**
is waiting, and is always named, so the scope is not in doubt. A **hollow
triangle** is stopped. The clock never goes away. A quiet 1 px rail sits under
the header at rest and turns amber when something is waiting.

**Choices 2 / 3 / 4.** 2A, 3A, 4A, executed as a designed status language, not
only as revealed strings.

### Choices

| Choice | Task | Audience | Reason | Tradeoff | Reference |
|---|---|---|---|---|---|
| Same SEQ footer from boot as A | First sound, then a pattern | Newcomers | Same causal reason as A. The first screen also teaches the mark language: the triangle is hollow, so the pattern is not running. | Same as A on the footer. The hollow triangle is a display change experts will notice (see risks). | Deluge blank defaults; M8 task teaching. Source-map §3.4: the dim filled triangle is already easy to read as “on”. |
| Same `SOUND` header and PARA footer as A | Sound vs steps; the way back | Anyone on PARA | Same as A, so SEQ and SOUND share one header skeleton. Waiting marks are reserved for a held step or a queued save — they are not used as decoration on the resting SOUND screen. | Same `UNSAVED` squeeze as A; B draws `UNSAVED` as an outline pill when it fits, otherwise the 2 px dot. | M8 consistency. |
| Hollow triangle when stopped; filled when playing | Reading transport in a glance, including at boot | Live players; newcomers looking at first sound | The shipped play icon is visible while stopped, only dimmer (5495–5498). The source map already flags that this can be read as “playing.” Same shape, empty vs full, no new icon, no `STOP`/`PLAY` word (those are not in source and were dropped from Dot 01). | Experts who have learned the dim triangle will see a change. The outline is a few pixels at 1× and must stay legible (it was checked at 1×). | SMK play icon 5153–5162, 5495–5498; source-map ambiguous feedback. Not a copy of any other instrument’s transport mark. |
| Waiting = amber outline, always named. Confirmed = brief fill. Quiet rail at rest, amber bar when waiting. | Queued save, held step, queued next pattern, unsaved work | Live players, and anyone deciding “has this been written?” | Shipped marks for these states are a mixed set: a `Q`, a 2 px dot, a full-header toast, a cue arrow, a small hint line. They do not read as a family, and the save cue is the weak one in a demo. B uses one pending mark and **different words** for different objects: `SAVE AT STOP`, `STEP 5`, a framed next-pattern number, `UNSAVED`. `SAVED` is a filled pill — waiting became written — and the rail goes quiet. The 909 chip stays next to `SAVE AT STOP` and `SAVED` (those strings fit). The long `SAVES WHEN STOPPED` is an outline pill with a 3 px part-colour tick, because it cannot sit next to the chip. | A new on-screen language to learn. The 2 px bar and the outlines are new chrome, still inside the 20 px header and the existing 13 px hint. They are not a new LED layer (Deluge’s praise for lit shortcut groups is taken as “make groups readable,” not as a lamp redesign; adding lamps is out of scope). | Digitakt: a reload is praised but **not added**; the harm is accidental loss and unclear scope. Naming the object, and making queued look unlike written, is the check against `ui.c` 2260–2265, 2291–2294, 5450–5460, 5470–5475, 5803–5806. Deluge “lit groups”: on-screen, not LEDs. |
| `STEP 5` outline pill in the header, plus an outlined hint using the source line | Edit one step, without thinking the whole sound is changing | Anyone who holds a step | A’s amber line is easy to miss. Putting `STEP 5` in the same waiting family as `SAVE AT STOP` makes the scope a first glance, and still uses the firmware’s words. `SOUND` stays, so this is not a new mode. `P-LOCK` on SEQ, and `RELEASE STEP FIRST`, are unchanged. | The pill must fit left of the pattern badge at x=132. At XS it does (`909` + `SOUND` + `STEP 5` ends at 122). | Digitakt unclear scope. SMK 5803–5806, 8284–8286. Dot 04’s banner is not used. |
| Save: full words, clock stays, never a `Q` | Save while playing; confirmation | Live players; a demo | Same causal reason as A. The outline vs fill is B’s addition, so a queued save and a finished save are not the same kind of chip. | Same as A on write rules. The 909 identity stays on the persistent `SAVE AT STOP` and on `SAVED`; it yields for the long 1300 ms `SAVES WHEN STOPPED`. | Digitakt; SMK deferred save 2222–2294. Pickup rule 5410, reused and then designed. |
| Kit overview kept; selected row still a part-colour fill; playhead still a white bar | Pattern writing; live play | Everyone | Selected = fill is already in `draw_drum`. B adopts it as the “fill” half of the family, rather than inventing a new selected mark. | No change to how a muted track or a p-lock notch is drawn. | SMK 5511–5549. Dot README on the kit. |

### Risks to live play and muscle memory

- **Muscle memory:** no gesture changes. The expert check “Did anything feel
  different under your hands?” will almost certainly get a **yes** on the play
  icon, and possibly on the amber outlines. That is a display-only yes; it
  should be logged, not treated as a blocked remap. If the hollow triangle is
  rejected, the rest of B can keep the outline language and revert the icon.
- **Live play:** still header and hint blits, no audio-path work, no flash
  during play, no live-note change. The clock staying, and a `SAVE AT STOP`
  that cannot shrink to `Q`, are a gain mid-set. A 2 px bar and a pill must not
  grow the strip heights (they don’t) or the per-frame allocation (they
  shouldn’t; planned only, not measured).
- **Learning cost:** waiting-is-outline is consistent, but it is new. A player
  who already treats the filled `UNSAVED` badge as “dirty” will see an outline
  instead. The word is the same.
- **Not claimed:** any of this has been seen on a device, or timed on a CPU.

---

## Direction C — Two rooms

**Concept.** Sound and steps are different rooms, with the same furniture. A
coloured **edge** tells you which room you are in (part colour for the 909,
amber when you are in a held step). Waiting and saving use **tickets** in the
same family as the 909 part chip. Footer action words are the bright ones, and
`SEQ: STEPS` is in accent, so the way back is obvious. The clock never goes
away. The play icon stays as shipped (this is one way C differs from B).

**Choices 2 / 3 / 4.** 2A, 3A, 4A, executed as a spatial / architectural system.

### Choices

| Choice | Task | Audience | Reason | Tradeoff | Reference |
|---|---|---|---|---|---|
| SEQ footer from boot, with action words in ink and key names dim | First sound; reading what the keys do at 1× | Newcomers | Same as A, plus contrast: `TRACK`, `STEP`, `MUTE` are the bright words. This is still the source footer, not a tutorial sentence. | A two-colour footer is new chrome on a 10 px line. Experts who have learned the even dim line will see a change. | M8 task teaching; Deluge blank defaults. SMK 6783–6786. |
| `SOUND` header, clock kept, plus a part-colour spine (below the header) and a 2 px hairline | Knowing which part and which room you are in | Anyone moving between SEQ, PARA, and a held step | Context shifts are the source-map problem (white keys, PARA vs SEQ, a compact header that says only “909”). A continuous edge, in the part colour already used for the chip, makes “you are in the 909” a spatial fact, not only a word. The spine starts at y=20 so it does not cut a chip or a ticket. | Invented chrome. A 3 px bar could be read as a meter, a fader, or a crop. It is display-only and not a control. Draw budget is a planned concern, not measured. | SMK part colours `ui.c` 89–93. M8 consistency (same furniture: chip, grid or matrix, footer). Not a copy of another product’s “frame.” |
| When a step is held: the edge turns amber, and the source STEP line becomes a full-width amber plate with black text | Edit one step; not confusing it with the sound | Anyone holding a step | The room has changed scope. Amber is the waiting colour (shared idea with B, different execution). The words are exactly `STEP n  < >: CHANCE  PARA` (5803–5806), not Dot’s “EDITING: THIS STEP ONLY.” `SOUND` stays in the header, so this is not a new mode. | 909 orange and amber are close in the RGB565 palette. The tell therefore leans on the **plate** (full width, black on amber) and on the three-sided frame (hairline, spine, plate), not on hue alone. | Digitakt unclear scope. SMK 5803–5806. Dot 04, evolved, banner dropped. |
| Tickets for `SAVES WHEN STOPPED`, `SAVE AT STOP`, `SAVED`, and a queued next pattern | Save while playing; live play with a pattern waiting | Live players; a demo | A ticket is the same object-family as the 909 chip (filled plate, dark text). `SAVE AT STOP` sits next to the 909 chip. `SAVED` is a brief ticket in the **part colour** (the same face as the shipped toast), not a new “success green” and not a persistent panel. The long `SAVES WHEN STOPPED` is a ticket with a 3 px part-colour tick. The next-pattern cue is a small ticket, so it cannot be mistaken for a save (different word, different place). | The 909+`SEQ` words yield during `SAVED` (the spine and the clock remain). A ticket is more visually loud than A’s amber text. Write rules are unchanged. | Digitakt accidental loss and unclear scope. SMK 2262, 294–300, 5410, 5470–5475. Deluge upgrade: streams untouched. |
| `SEQ: STEPS` in accent on the SOUND footer | Finding the way back | Newcomers on PARA | Same strings as A (6776–6779). Accent on the way-back action only. | Must not look like a new button. It is a colour emphasis on an existing hint. | M8 task teaching. Decision 8: header stays `SEQ`. |
| Kit overview kept; play icon as shipped | Pattern; live play | Everyone, including experts who would object to B’s hollow triangle | C’s distinctiveness is the edge and the tickets, so the play icon can stay. An expert who dislikes B’s icon has a direction that still overhauls sound-vs-step and save. | The dim-triangle ambiguity at boot is not fixed in C. | SMK 5495–5498. |
| HOME keys still silent; PATCH still toggles | Accidental PATCH | Same as A | Same prohibition as A. The room language is not applied by giving HOME a false “sound room.” | Same as A. | M8 instant pads; decision 1C; SMK 3733–3787, 4051. |

### Risks to live play and muscle memory

- **Muscle memory:** no gesture changes. Experts will notice the spine, the
  hairline, the contrast footer and the tickets. None of those is a remap. The
  play icon does not change, which is a smaller “felt different” risk than B.
- **Live play:** display only, same strip heights. A full-height 3 px spine and
  a full-width plate are more pixels than A or B; they still sit in the existing
  20+69+69+82 blit. Not measured on hardware. Must not touch audio or flash.
- **Misreading the spine:** as a level, a battery, or a damaged edge. The
  accompanying hairline and (on a held step) the word plate should hold the
  meaning. If the spine is rejected, the tickets and the plate can stand alone.
- **Amber vs 909 orange:** as noted above, the plate has to carry the step-room
  tell. This was checked at 1× on the overview and on C’s sheet; it was not
  seen on a device.

---

## Recommendation

**Recommend Direction B, Live signals**, as the one to take to an implementation
plan, subject to the owner’s review of the sheets.

It is a full visual and interaction system, not a set of header tweaks: every
screen of the newcomer-to-live journey uses the same marks, and the marks answer
the audit problems that actually show up in a demo (a `Q` for a queued save, a
clock that disappears, a dim triangle that looks like play, a held step that
doesn’t announce its scope). It stays inside the firmware’s chrome geometry
(20 px header, 13 px hint, existing strings, existing kit grid). It takes the
Digitakt reports seriously on both sides: there is no new reload button, and
queued vs written vs “this step” vs “next pattern” are named and marked as one
family. It takes the Deluge “lit groups” praise as a reason to make waiting
readable on screen, and the “don’t add an LED layer” decision as a hard limit.
Expert gestures are untouched.

**Look first at** the overview’s four moments, then at B’s sheet rows 4 (one
step), 5 (save while playing) and 7 (live play), at real 240×240.

**Pick A instead** if the owner wants only already-written pixels: revealed
strings, no new marks, no play-icon change. It is the lowest-risk path and the
least of an overhaul.

**Pick C instead** if sound-versus-step is the headline and a spatial “you are
in this room” is worth new chrome. C also keeps the shipped play icon, so it is
the overhaul for an owner who likes B’s save and step work but not the hollow
triangle. The spine is the risk to watch.

No direction should be implemented until the owner approves a plan. Elite and
Pro interaction is identical in source; these frames are that shared UI.

## What this pass did not do

- No firmware or host-UI edits, builds, tests, device writes, commits or pushes.
- No new web research. User-report links were not fetched.
- No gesture remaps, tutorial layers, LED layers, or new save semantics.
- Decisions 1 and 5–10 were not reopened. PATCH still toggles HOME/SONG; those
  screens are not a journey row, and their behaviour is unchanged.
- `/workspace/smk-ux/presets-considerations.md` was not touched.
- CURRENT / SHIPPED frames remain reconstructions. Fonts are repo TTFs, not
  packed felucca bitmaps.
- CPU, flash reserve, and LCD blit cost are not measured. A later
  implementation plan must include those, and the expert muscle-memory checks
  already in the plan, before any claim of live fitness.


---

## Appendix: Juno and Drone engine pages

An update to these three directions, not a fourth one. Juno and Drone
parameter pages ship as a name, a percent, and a thin bar, with none of
the 26×19 pictures Chicago, Miami, and the 909 already use, and with
times and rates shown as a percent.

The proposed pictures and units are the **same in A, B, and C**. Only
the already-agreed header, footer, and marks change (SOUND and the clock
in all three; B’s hollow play triangle and quiet rail; C’s part-colour
spine and contrast footer).

Full diagnosis, the ranked flash and RAM budget, and the writer-ownership
map are in [`engines-juno-drone.md`](engines-juno-drone.md). Frames and
sheets:

- `frames/1x/shipped-juno.png`, `shipped-drone.png`, `A-juno.png`,
  `A-drone.png`, and the B and C pairs (2× under `frames/2x/`)
- `A/engines-juno-drone.png`, `B/engines-juno-drone.png`,
  `C/engines-juno-drone.png` (and `-2x`)
- Combined sheet `engines-juno-drone.png`

Chord or note drawing on SEQ is not in this appendix; that work is under
`/workspace/smk-ux/seq-chords/`. The SOUND / preset row is not redesigned
(Main Builder). Firmware was not edited. PROPOSED is not implemented.
