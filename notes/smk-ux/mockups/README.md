# SMK-37 journey mockups — 94197fc8

Evolved from Dot’s six PROPOSAL screens in
`/workspace/handoff-dot-20261113/firmware/journey-6-screens-proposal.png`.
Not implemented. No firmware or host-UI build was run.

- **CURRENT** frames are source-faithful reconstructions at commit
  `94197fc839b00aff47b98ca02f3c6396e4a6eabf`, cited with `file:line`.
  They are not device photographs and not Dot’s host-UI framebuffer
  (that framebuffer’s HOME used an SMC footer; the novice path here is
  keyboard-alone, `ui.c` 6757–6759).
- **PROPOSED** frames start from Dot’s screen of the same number,
  then KEEP / DROP / FLAG each piece of chrome. Every proposed element
  is either existing source behaviour (cited) or labelled PROPOSAL.
- Native size is **240×240**. A **2×** copy is nearest-neighbour.
  Strip layout is the real **20+69+69+82**. Colours are RGB565-safe
  (same `RGB()` quantisation as `firmware/src/gfx.c:65`), sampled
  against Dot’s 94197fc8 host HOME.
- Fonts: Barlow Semi Condensed TTFs from the repo, PIL at the source
  roles **T 10 SemiBold, XS 12 SemiBold, S 15 Medium, B 15 Bold, M 23
  SemiBold**. These are **not** the firmware’s packed felucca bitmaps;
  glyph advances may differ by a few pixels. Tabular figures are not
  applied.

Generator: `/workspace/smk-ux/render_mockups.py`.

Newcomer path (owner order): **first sound → pattern → save**.
Expert muscle memory and live play are unchanged.

## Mapping to Dot’s screen numbers

| Dot # | Dot title | Evolved role | CURRENT 1× | PROPOSED 1× | Sheet |
|---|---|---|---|---|---|
| 01 | Find a part | **First sound** on boot 909 SEQ (Dot 01 was HOME, a first-sound dead end) | `1x/01-first-sound-CURRENT.png` | `1x/01-first-sound-PROPOSED.png` | `sheets/01-first-sound.png` |
| 01 (HOME lineage) | same, HOME chrome | After PATCH, and a second PATCH → SONG | `1x/01-HOME-after-PATCH-CURRENT.png`, `1x/01-SONG-second-PATCH-CURRENT.png` | `1x/01-HOME-after-PATCH-PROPOSED.png`, `1x/01-SONG-second-PATCH-PROPOSED-stays-HOME.png` | `sheets/01b-after-PATCH-home-song.png` |
| 02 | Put a hit on a step | **Pattern** on the 12-row kit grid | `1x/02-put-a-hit-CURRENT.png` | `1x/02-put-a-hit-PROPOSED.png` (matches CURRENT: the fix is what was not taken from Dot) | `sheets/02-put-a-hit.png` |
| 03 | Edit the drum sound | **PARA / SOUND**, how you tell you are there | `1x/03-edit-sound-CURRENT.png` | `1x/03-edit-sound-PROPOSED.png` | `sheets/03-edit-sound.png` |
| 04 | Edit one step only | Held-step p-lock; the tell versus 03 | `1x/04-edit-one-step-PARA-CURRENT.png`, `1x/04-edit-one-step-SEQ-CURRENT.png` | `1x/04-edit-one-step-PARA-PROPOSED.png`, `1x/04-edit-one-step-SEQ-PROPOSED.png` | `sheets/04-edit-one-step.png`, `sheets/03-04-how-you-tell.png` |
| 05 | Save while playing | **Save**, deferred | `1x/05-save-while-playing-CURRENT-toast.png`, `…-CURRENT-badge.png` | `1x/05-save-while-playing-PROPOSED.png`, `…-PROPOSED-badge.png` | `sheets/05-save-while-playing.png` |
| 06 | Saved after transport stops | Confirmation | `1x/06-saved-CURRENT.png` | `1x/06-saved-PROPOSED.png` | `sheets/06-saved.png` |

Dot’s unaltered 240 and 480 crops are in `dot-original/01-dot-original-*.png` … `06-…`.
A four-frame newcomer contact sheet is `sheets/00-newcomer-path-proposed.png`.

## Chrome ledger (Dot → evolved)

### 01 Find a part → first sound, plus HOME after PATCH

Dot 01 was a proposed HOME titled “DnBOX”. First sound in source is
**boot 909 SEQ** (`ui_init` → `V_SEQ`, 8604): black key 1 selects BD
and, while stopped, plays it (3193–3213). HOME keys are patterns and
mutes and make **no sound** (3733–3787). The evolved 01 is therefore
909 SEQ. Dot’s HOME chrome is decided here and applied on sheet 01b.

| Piece | Decision | Reason |
|---|---|---|
| Title “DnBOX” | **DROP** | Header is `HOME` (`VIEW_N`, `ui.c` 88, 5445). Product name is for USB / ABOUT, not this screen. |
| “125 BPM / STOP” (or PLAY) | **DROP** | Tempo number + `play_icon`; no STOP/PLAY word (5495–5498). |
| Large “PATTERN 01”, always-on “CHAIN 01” | **DROP** | Badge `#` + number; **CHN** only when `chain_a != chain_b` (5476–5477). Chain is live-only and is not saved. |
| Large “UNSAVED” | **DROP** | XS badge or `*`, plus a 2 px dot (5450–5460). |
| Footer “K3: part  SEQ: steps  PATCH: home/song  HOLD BUTTON: help” | **DROP** | Would replace live `WHITE: PATTERN / TWO: CHAIN / BLACK: MUTE` (6757–6759). `K3` is `UI_CAP_ALGO` (50), not “part”. Hold-help already exists (1909). |
| K5–K8 captions on PERFORM | **DROP** | Source titles are TEMPO / SWING / FILTER / PUMP (1470–1471). Physical knobs are already K5–K8. |
| Five-lane overview, PERFORM knobs | **KEEP** | On the HOME pair (01b). Display order 909 / 808 / BREAK / CHICAGO / MIAMI is `PART_ORDER` (99). |
| 12-row 909 kit on SEQ | **KEEP** (source, not Dot 01) | This is first sound and “find a part” of the 909 kit. |
| SEQ footer from boot | **PROPOSAL** | Use already-written `BLACK: TRACK / WHITE: STEP / PATCH+BLK: MUTE` (6783–6786) from power-on, instead of hiding it behind `any_button` (6738). Hold-help stays. No gesture change. |
| Second PATCH from HOME | **PROPOSAL, owner decides** | Current: HOME↔SONG toggle (4051). Proposed: stay on HOME. SONG via hold PATCH or GLOBE. Experts who double-tap PATCH for SONG would need a new path. SEQ is unchanged. |

### 02 Put a hit on a step

| Piece | Decision | Reason |
|---|---|---|
| 2×8 numbered one-track pad grid | **DROP** | `draw_drum` is a 12-row all-track grid (5511–5549). Dot’s README already said this must not delete the kit overview. |
| “BD BASS DRUM” | **DROP** | Voice name is the short `BD` (`engine_voice_name`). |
| “PADS: STEPS 1-16”, “TAP PAD: ADD/REMOVE HIT” | **DROP** | Pads alias white keys. Footer already `WHITE: STEP` (6784–6786). |
| “909 / STEPS”, “P01 / STOP” | **DROP** | Source is the 909 chip + `SEQ` (printed button) + badge + play icon. |
| Persistent “SEQ PLAY: start  PARA: sound  Hold step + knob” | **DROP** | Already hold-help. Do not replace the live footer. |
| K5/K6 captions on LENGTH/RATE | **DROP** (optional chrome) | Meaning is correct; they are not drawn on the SEQ strip today. |
| Hits on 1/5/9/13, LENGTH 16, RATE 1/16, kit overview | **KEEP** | Fixture and source. The evolved 02 **matches CURRENT** — the low-budget fix is not replacing the grid. |

### 03 Edit the drum sound

| Piece | Decision | Reason |
|---|---|---|
| Banner “EDITING: SOUND”, “Applies to this drum sound” | **DROP** | Not in source. A mode word is enough. |
| Huge DECAY 1.0 s as the PARA identity | **DROP** | A 900 ms `draw_readout` already exists when a knob is turned (6455–6505). PARA itself is the 4-row matrix (5740–5834). |
| “BD BASS DRUM”, “P01 / STOP” | **DROP** | Same as 02. |
| “PARA: next page” | **DROP** | Wrong: PARA toggles `V_PART`; pages are K2 / the SCALE list. |
| Intent: SOUND vs STEPS is obvious | **KEEP** | |
| Tune / Attack / Decay / P.Dpth on K5–K8 | **KEEP** | `add_track_pages`, `drum909.c` 80–82 (Level is on MIX). |
| 4-row matrix | **KEEP** | It is PARA. |
| “909” + “SOUND” in the header | **PROPOSAL, sourced** | Word `SOUND` is already at 5437, skipped by the compact header (5373–5408). Pattern and tempo stay, same rule as pickup (5410). |
| Footer `BLACK: SELECT / WHITE: SOUND / SEQ: STEPS` | **PROPOSAL, sourced** | Already written at 6776–6779, but `draw_knobs` returns before `draw_footer` (8337–8343). `SEQ: STEPS` is the way back. No new gesture. |

### 04 Edit one step only

| Piece | Decision | Reason |
|---|---|---|
| Banner “EDITING: THIS STEP ONLY”, “Step 05 held. Release to return.” | **DROP** | Held-step UI is the grid, p-lock notches, and tags (8284–8288, 5803–5806). |
| Replacing the grid/matrix with a DECAY slider | **DROP** | Same as 03. |
| Persistent “BT + knob: remove its step lock”, “PATCH + SEQ REC: undo” | **FLAG, keep as shortcuts** | Both exist (2951–2953, 4007–4024) and stay in hold-help. They are not persistent captions. They must not be removed. |
| Intent: this-step vs the whole sound is obvious | **KEEP** | The tell is the SOURCE line `STEP 5  < >: CHANCE  PARA` (5803–5806) versus no such line on 03, and on SEQ the tag `BD P-LOCK STEP 5` (8284–8286). |
| `RELEASE STEP FIRST` (2755–2759) | **KEEP** | Correct safety, not a bug. |
| “P-LOCK” wording | **KEEP** | Expert vocabulary; do not rename without the owner’s words. |

### 05 Save while playing

| Piece | Decision | Reason |
|---|---|---|
| Title “SAVE QUEUED” | **DROP** | Visible strings are `SAVE AT STOP` / `SAVES WHEN STOPPED` (290–300). The enum is `SAVE_QUEUED`, not a title. |
| “Playback continues. SEQ PLAY: stop and allow saving.” | **DROP** | Verbose; would cover the music. The existing toast already says `SAVES WHEN STOPPED`. |
| “DnBOX”, “125 BPM / PLAY”, K3/SEQ/BT footer | **DROP** | Same as 01. SEQ footer stays. |
| Placing this on HOME | **DROP** | The newcomer is on 909 SEQ, where the pattern was made. The save path is the same on any view (`B_SAVE` → `save_project`, 4105). |
| Intent: explain SAVE AT STOP, do not write flash during play | **KEEP** | Already the source policy (2222–2294). |
| Strings `SAVES WHEN STOPPED`, `SAVE AT STOP` | **KEEP** | |
| Pickup-style left chip, clock stays | **PROPOSAL, sourced** | Fader pickup already keeps “song/pattern and tempo” (5410). On a busy 909+SEQ header the current badge shrinks to **`Q`** (5458–5460) — that is CURRENT-badge, and is map problem 8. Proposed persistent cue is the full words as a left chip (`05-…-PROPOSED-badge.png`). |
| BT = save, 250 ms copy, 400 ms help, deferred write | **KEEP** | Expert muscle memory. Timings are an owner decision if they change. |

### 06 Saved after transport stops

| Piece | Decision | Reason |
|---|---|---|
| “PROJECT SAVED”, “Your changes are stored.”, green persistent panel | **DROP** | Source word is `SAVED` (2262), 1300 ms (407), then the normal header. Success does not keep a panel. Only after `rc==0`. |
| “SEQ PLAY: start again” | **DROP** | Instructional chrome. SEQ PLAY is unchanged. |
| “DnBOX” | **DROP** | |
| The word `SAVED`, 1300 ms, success-only | **KEEP** | |
| Left chip, pattern and tempo stay | **PROPOSAL, sourced** | Same pickup rule as 05. Useful in a live demo. After 1300 ms, nothing new persists. |

Bluetooth device-save remains a **different** path and still says
`BLUETOOTH SAVED` (`ui.c` 8744). It is not the BT **button**.
