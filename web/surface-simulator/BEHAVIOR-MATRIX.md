# Browser simulator behavior evidence

Source: `reference-smc-integration`, HEAD `8145957`. All paths below are relative to that checkout. Read-only source inspection, 2026-10-21. This is not a hardware PASS.

## Evidence boundaries

- Model the current requested topology as **SMC → direct BLE → SMK → USB → Loopy**. Do not draw three simultaneous physical BLE links. Generic shared-host routing is separate optional source behavior, not this topology.
- No actual Loopy project database or layout file was found in the workspace filename search. `approved-smc-forensics/incoming-20261021T1242/LoopyX0X37/project.zip` contains firmware investigation artifacts, not a Loopy project. Historical `APC40_IMPLEMENTATION.md:91` describes a 12×4 canvas and a 4×4 clip grid at columns 8–11; it does not establish the user's current project. Any supplied browser grid must be labelled **demo feedback fixture, not your Loopy project**.
- Source behavior below can be implemented in JavaScript. Synthetic sound, browser clock, fixture feedback, dragging virtual controls, connection toggles, and local save/reset are **simulation-only** unless explicitly connected to real MIDI. Do not imply firmware DSP, BLE reliability, USB enumeration, actual Loopy auto-map, or hardware colours have been reproduced.
- `APC40_IMPLEMENTATION.md` and even parts of `BEHAVIOR.md` contain superseded claims. Prefer the cited current implementation for colours, physical pad mapping, keybed routing, and feedback isolation.

## SMC wire map

`firmware/src/app/smc_protocol.h:5–14,24–27`; `smc_mixer.c:545–576`.

- DAW: pitch bend on channels 1–8 => faders 1–8; source uses the high 7 bits (`((MSB<<7)|LSB)>>7`). Relative CC16–23 on channel1 => eight encoders. Delta = bit6 set ? minus low6 : low6; 0 and64 are zero.
- User: CC40–47 => faders, CC16–23 => relative encoders. Direct source accepts these on any channel in User mode. DAW buttons use channel1. Shared USB/peripheral-host User routing reserves channel16; do not confuse that with direct BLE.
- Button press is Note On with nonzero velocity; Note Off or Note On zero releases. Record notes00–07, Solo08–0F, Mute10–17, SELECT18–1F (hex).
- Eleven globals: PLAY5E, STOP5D, REC5F, REW5B, FF5C, BankLeft2E, BankRight2F, Up60, Down61, Left62, Right63.
- Stock User SysEx: `F0 7F 7F 06 cmd F7`. Commands1–32 interleave Mute, Solo, Record, SELECT for each strip: `cmd=4*strip+{1,2,3,4}`, strip0–7. Commands33–43 follow the eleven globals listed above. No release exists for stock MMC; Solo uses latch fallback.

## SMC destinations and behavior

`docs/SMC_MIXER.md:7–68`; `smc_mixer.c:175–325,371–534`.

| Visible context | Faders1–8 | Encoders | Buttons |
|---|---|---|---|
| HOME / MIX PARTS | 909,808,303A,303B,BREAK levels; reverb,delay returns; master DJ filter | Same eight destinations | Part/FX controls and navigation |
| 909 or808 MIX/SEQ | Selected kit track levels1–8 or9–11 | Same destinations | All four rows target visible drum strips |
| Other ordinary page | Main eight destinations | First four visible fields; rest inactive | Ordinary navigation |
| Menu | Main eight destinations | Six visible rows; headings/actions/unused/duplicate fields inactive | Up/down rows; left/right value; R6 action; R7 back |
| Clip grid | Main eight destinations | Inactive: do not edit hidden X0X page | Up/down cycles grid/SCALE/CHORD/ARP |
| SCALE/CHORD/ARP | Main eight destinations | First4/4/8 displayed fields | Bank buttons select field; left/right edit; R7 grid |
| Help / confirmation | Inactive | Inactive | R7 dismiss help; R6 confirm/R7 cancel confirmation |

Pickup: target scaled `round(value*127/max)`. A fader acquires within ±2 MIDI units or crossing target. Changed target identity OR externally changed value clears both pickup and previous-crossing history. Own successful write preserves pickup. Paired encoder changes invalidate pickup. Display `PICK F#` until acquired, then `SMC F#`. Source `smc_mixer.c:239–310`, `smc_protocol.h:17–22`.

Drum banks have **11**, not12, tracks. BankLeft selects track1 and BankRight track9. Bank2 strips4–8 are entirely inactive/unlit, never navigation shortcuts. Selection persists per kit. Record selects kit/drum then toggles global recording; SELECT only selects. Mute toggles track mute. Solo isolates while held, supports several held buttons, and release clears its original target across view/kit/bank changes. MMC Solo toggles; same physical Solo key in a different bank replaces old target. Temporary Solo clears on disconnect, blocked input and reset. `smc_mixer.c:371–430,528–533`; `docs/SMC_MIXER.md:19–38`.

Ordinary page Up/Down cycles HOME, PART(sound), SEQ, GEN(303 only), FX, MIX, SONG. Left/right changes page without wrap. Bank changes part except drum sound pages where it changes selected drum. SELECT1–5 opens909/808/303A/303B/BREAK sound. SELECT6–8 opens reverb/delay/master. R6 menu/action, R7 back, R8 HOME; S6 MIX, S7 Global, S8 sound. M6/M7/M8 toggles reverb/delay/master output. SMC PLAY/STOP/REC remain X0X transport even in clip mode: **not APC transport**. Rewind/FF cue previous/next selected-part pattern. `smc_mixer.c:433–525`.

SMC feedback: Note On channel1 velocity127/0 for all43 lights. Drum R lit only when recording and drum+kit selected; S active Solo; M muted; SELECT selected. Empty bank2 strips dark. Ordinary R1–5 recording+part selected, S1–5 active Solo, M1–5 muted, SELECT1–5 selected. R6 overlay active, R7 off, R8 HOME, S6 MIX, S7 Global, S8 PART. M6–8 reflect zero return/master output. STOP lit when stopped, PLAY when playing, REC recording. Other globals dark. Pitch bend target replies use statusE0+strip, both data bytes=scaled target (not motorized-fader movement). `smc_mixer.c:660–729`.

## Representative field pages

303 TONE: Cutoff, Reso, Envmod, Decay. VOICE: Accent, Wave, Drive, Slide. FILTER ADSR: Attack, Decay, Sustain, Release. Deep additions include Tune, Volume, Drive type, Accent decay, Filter envelope. `firmware/src/app/ui.c:973–984`.

Global begins MIDI heading, Clock Source, Clock Out, Notes Out, five part channels, Clip Keys channel; AUDIO output/USB settings; KEYS AND SCREEN; PROJECT Save,Load,Autosave,Clear Pattern,Reset; SYSTEM; BLUETOOTH MIDI Bluetooth, BT ROLE, SMC PRESET, status/name. Headings/actions must not respond to encoder value edits. `ui.c:1263–1311`.

Clip fields in exact displayed order (`smk_clip_keys.h:143–155`):
- SCALE: ON, ROOT(C..B), SCALE(24 names), SNAP(NEAR/FILTER).
- CHORD: ON, CHORD(30 names), INV(ROOT,/1,/2,/3), STRUM(OFF,10..70ms).
- ARP: ON, ORDER(UP,DOWN,UP/DN,DN/UP,PLAYED,RANDOM), OCT(1..4), SWING(50..71% in3% steps), HUMAN(0..7), RATE(13 values), GATE(10..100%), RHYTHM(16), LATCH(OFF/ON). Ninth field accessible by field navigation, not SMC K1–8.
- Names and scale masks/chords/rate ticks are directly in `smk_clip_keys.h:24–51`; edit clamps instead of wraps. SMC stage handling in `main_fm1.c:1358–1405`.

## SMK physical surface

`SMK37.md:24–43`; current pickup `main_fm1.c:801–880`; `BEHAVIOR.md:46–67`.

- 37 keybed, only middle F3–G5 (27 keys) control ordinary X0X; outer10 inactive outside clip mode. No keybed LEDs. Ordinary pad top row steps1–8, bottom9–16; HOME/SONG pads patterns. 303 REC pads become keyboard. Drum MIX/SEQ pads sequence selected drum without changing visible page.
- K1 master; K2 select; K3 algorithm; K4 presets; K5–8 page parameters. SCALE=SEL, CHORD=ENV, NOTE REPEAT=LFO, PARA=EDIT, GLOBE=GLO, PATCH=HOME, BT=SAVE, FX=FX, ARP/SEQ/PLAY/REC self-named.
- FADER BANK A:909,808,303A,303B levels; B:BREAK,reverb,delay,master filter. Pickup also applies to these four faders. Bank B light on.
- KNOB BANK toggles clip mode once per press/hold. In clip mode faders still control X0X; keybed is Loopy-only. K2–8 emit relative CC48–54 channel1 with no stage/menu open; stage/menu owns them otherwise. K1 stays master (`BEHAVIOR.md:431–445`).
- Ordinary arrows change page; menu/stage arrows edit selected value, repeat after400ms then80ms. Pitch wheel momentarily tunes selected303 ~±1.5semis, mod opens its cutoff, neither changes stored parameter.

## APC / Loopy fixture

Use feedback as truth, not a local guessed playing state. Forty APC cells indexed `note=sourceRow*8+sourceCol`; highest source row draws top. In current code the physical pad index first XOR8, then maps via `smk_apc40_note_for_pad_view` (`smk_apc40.h:536–556,588–600`).

For origin(0,0) 4×4 fold, physical top row notes **24,25,26,27,8,9,10,11**; bottom **16,17,18,19,0,1,2,3**. For8×4 first view physical top8–15, bottom0–7; next view top24–31,bottom16–23. Add source origins. Invalid/zero-feedback cells launch nothing. Press sends channel1 Note On with **current pad velocity**; release sends Note Off with127 in current `apc_note`, not the stale documented fixed127/0 pair (`main_fm1.c:224–228,1938–1960`).

Footprint trims empty margins but preserves interior cells.3–4 rows and≤4cols folds. Otherwise local8×2 views, OCT navigation; last5-row view clamps start to row3. Latch250ms, no shrinking, growth only completing4×4 fold. No APC navigation notes. `smk_apc40.h:325–410,507–524`.

Pad colours: owner table `apc40_pad_owner.h`; ordinary lit clips half-table, idle velocities7,11..59 full-table. LCD uses `apc40_lcd565.h` except idle shades use `apc40_lcd_idle.h`. Source `smk_clip_colour.h`, `main_fm1.c:763–780`. Browser RGB is illustrative; hardware calibration bytes are not a browser colour-management guarantee.

Queued feedback blink: 24PPQN, half-beat on; fallback250ms on/off when no clock500ms. Two off/on flashes within256ms detect queue; queue times out384ms. Page snapshot needs≥8cells,≥half lit,≥3rows and≥3cols,no cue state. `BEHAVIOR.md:347–351,389`, implementation `smk_apc40.h`.

SMK PLAY/REC in clip mode emit APC5B/5D momentary; PATCH+REC/PLAY emit custom Undo70/Redo71 on channel16, requiring Loopy bindings. Local transport LED press toggles provisionally, feedback replaces it. PLAY with live clock lights first6/24 ticks. `smk_apc40.h:413–429,473–485`.

No host feedback => WAITING FOR LOOPY PRO. Host presence begins on clip/transport feedback, not identity alone. Disconnect clears that link's cells/LED ownership. APC handshake suppresses feedback from sound engine on the handshake link even when clip mode exits; range notes0–98 plus CC16–31/48–63. `BEHAVIOR.md:336–338,463–467`, `main_fm1.c:307–405`.
