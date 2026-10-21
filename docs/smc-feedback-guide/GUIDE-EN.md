# SMC → X0X: controls and lights

**Source:** `4cf8c2f` · SMK-37 Elite development/test profile · 2026-10-21.  
**Scope:** source-backed usage, not a claim that this build has passed a physical-device test. [Build and scope](README.md)

## 1. Connect the right way

Use **one SMC directly over Bluetooth to the SMK running X0X**. Keep the SMC itself in **DAW mode**. On X0X, open **GLOBAL → BLUETOOTH MIDI**, set **SMC PRESET → DAW**, and enable **BLUETOOTH**. If X0X says **RESTART**, restart normally with Bluetooth enabled.

In the current central-only test profile, **BT ROLE reads `SMC TEST / USB` and is fixed**. Do not look for a selectable SMC HOST option. On an ordinary non-test build only, select **BT ROLE → SMC HOST** and restart after changing that role. The preset setting takes effect immediately. [S1](SOURCES.md#s1-setup-and-profile)

Use **USB between SMK/X0X and the Loopy host** in this test profile. SMC control/feedback and Loopy's APC/clip route are separate. This profile does **not** provide simultaneous Loopy-over-Bluetooth or three SMC connections. SMC PLAY/STOP/REC operate X0X; they are not Loopy APC clip commands. [S2](SOURCES.md#s2-loopy-and-routing)

**DAW versus USER:** the X0X preset chooses a decoder; it does not switch or detect the SMC's physical mode. DAW expects factory DAW messages. USER accepts the stock User CC/MMC formats and also enables reserved host forwarding. A customized User preset may no longer match. **DAW does not reliably ignore a physically User-mode controller:** encoder CCs and compatible notes can overlap. Keep both ends deliberately set to DAW for this guide. [S3](SOURCES.md#s3-preset-selection-is-not-mode-detection)

## 2. Choose the layout on the X0X screen

The surface follows the visible page, not a separately pinned SMC mode. Open **MIX**, then use **Left/Right** through **PARTS → 909 MIX → 808 MIX → MASTER**. On drum **SEQ**, the selected part chooses 909 or 808. Each kit remembers its selected drum and bank. [S4](SOURCES.md#s4-destinations-and-page-changes)

| Visible context | Faders | SMC knobs |
|---|---|---|
| HOME or MIX → PARTS | Main map below | Same eight destinations |
| 909/808 MIX or 909/808 SEQ | Individual drum levels, active bank | Same drum levels |
| Other ordinary pages, including MASTER | Main map | First four visible fields; 5–8 inactive |
| Menu open | Main map, **even over drum MIX/SEQ** | Six visible rows; actions/headings/read-only/empty rows inactive |
| Clip grid | Main map | Inactive; no hidden-page edits |
| SCALE / CHORD / ARP editor | Main map | First 4 / 4 / 8 fields |
| Help or confirmation | Inactive | Inactive |

**Main map, strips 1–8:** **909 · 808 · 303A · 303B · BREAK · reverb · delay · master filter**. The first seven are levels. **Fader 8 is not master volume:** below the centre band is low-pass, the centre band (60–68 on the 0–127 scale) is neutral, and above it is high-pass. In this layout **M8 mutes/restores actual master output**. [S4](SOURCES.md#s4-destinations-and-page-changes)

### Individual drums

**Bank Left** selects bank 1 and its first drum; **Bank Right** selects bank 2 and its first drum. Then press the wanted strip's **SELECT**. [S5](SOURCES.md#s5-eleven-drums-and-pad-targets)

| Kit / bank | 1 | 2 | 3 | 4 | 5 | 6 | 7 | 8 |
|---|---|---|---|---|---|---|---|---|
| 909 / 1 | BD | SD | LT | MT | HT | RS | CP | CH |
| 909 / 2 | OH | CR | RD | — | — | — | — | — |
| 808 / 1 | BD | SD | LT | MT | HT | RS | CP | CB |
| 808 / 2 | CY | OH | CH | — | — | — | — | — |

BD kick; SD snare; LT/MT/HT toms; RS rimshot; CP clap; CH/OH closed/open hat; CR crash; RD ride; CB cowbell; CY cymbal. The 808 sound variants stay on their existing track. There are **11 tracks, not 12**; KIT is kit-wide settings. Bank 2 strips **4–8 are inactive and unlit**, including all four button rows.

## 3. Read the lights; choose SELECT, not R, for editing

On **drum MIX and drum SEQ**, fader, knob and all four strip buttons address the same visible drum. [S6](SOURCES.md#s6-buttons-and-feedback)

| Button | Action | Lit means |
|---|---|---|
| SELECT | Select kit/drum; leave REC unchanged | This drum is selected |
| R | Select kit/drum, then toggle global recording | REC is on and this kit/drum is selected |
| S | Solo while held in DAW mode | This drum has an active Solo target |
| M | Toggle drum mute | **Muted**, not enabled or currently sounding |

After SELECT, **SMK pads edit that drum's steps directly in MIX or SEQ**, with REC on **or off**. Pad step lights follow the selected drum; the view stays open. There are no separate per-drum record-arm states. [S5](SOURCES.md#s5-eleven-drums-and-pad-targets)

Hold several S buttons to combine Solo targets. Release clears each button's **original** target even after a bank/page change. Solo does not rewrite saved mutes and clears on disconnect, blocked input or project reset. Stock User MMC has no release event, so its Solo is a press-again latch instead. [S6](SOURCES.md#s6-buttons-and-feedback)

Outside drum MIX/SEQ, strips **1–5** address whole parts: SELECT opens the part's sound pages; R selects and toggles REC; S solos while held; M toggles mute. **M6/M7** zero/restore reverb/delay levels; **M8** zero/restores master output. Their lights mean zero level/muted output. A part R light follows the actual selected recording destination, including selection made on the SMK itself.

**Navigation lights outside drum MIX/SEQ:** R8 = HOME; S6 = MIX; S7 = Global; S8 = sound view; SELECT 6/7/8 = reverb/delay/master page. R6 indicates an open overlay; R7 is normally unlit. PLAY = running, STOP = stopped, REC = recording. Arrow/bank/rewind/forward keys have no persistent on-state assigned here. Unlit navigation keys do not diagnose a lost connection. These are **state indicators, not input acknowledgments or audio meters**. [S6](SOURCES.md#s6-buttons-and-feedback)

## 4. Fader pickup

Move a fader **through its current X0X target** before expecting it to change the value. Pickup occurs within two MIDI steps of the target or when crossing it.

- **`PICK F…`** on X0X: input reached the fader handler, but pickup is still waiting. The displayed number is the target parameter value, **not a successful change**.
- **`SMC F…`**: pickup is acquired. Movement can now change that destination.
- **`SMC K…`**: an SMC knob is addressing the displayed parameter.

A new destination or a changed target value detected by X0X resets fader pickup, including changes made with the paired knob. Reconnection also resets it. X0X sends target-position feedback back to SMC; this guide does not promise a particular physical ring, colour or motor response. [S7](SOURCES.md#s7-pickup-and-feedback-output)

## 5. Fast navigation and examples

- **Up/Down:** cycle ordinary views HOME, sound, SEQ, generator (303 only), FX, MIX, song. **Left/Right:** pages within that view.
- Outside drum MIX/SEQ: **S6** opens MIX; **S7** Global; **S8** sound. **SELECT 6/7/8** opens reverb/delay/master. **R6** opens the deep menu or activates its action; **R7** backs out; **R8** goes HOME.
- **On drum MIX/SEQ, R6–R8 and S6–S8 are drum buttons, not shortcuts.** Leave with Up/Down or change MIX page with Left/Right.
- In a menu: Up/Down chooses a row; Left/Right edits. R6/R7 confirms/cancels a confirmation; R7 dismisses help.
- On a drum sound page, Bank Left/Right selects the previous/next drum. On other ordinary pages it selects the previous/next part.
- Clip mode: Up/Down cycles grid/SCALE/CHORD/ARP; Bank Left/Right chooses a field, including ARP LATCH; Left/Right edits it; R7 returns to grid. R6 opens Global. Clip/menu/help contexts override ordinary shortcuts.
- Rewind/forward chooses the selected part's previous/next pattern. [S8](SOURCES.md#s8-navigation-and-clip-editors)

**909 snare:** open 909 MIX → Bank Left → SELECT 2. Edit steps on SMK pads; trim with knob 2 or acquire fader 2.  
**909 ride:** Bank Right → SELECT 3. Hold S3 to isolate; release to restore the ordinary mix.  
**808 hats:** open 808 MIX → Bank Right. SELECT 2 chooses open hat; SELECT 3 closed hat. No R press is needed for step editing.

## 6. Troubleshooting without false positives

In this TEST profile, first feedback waits for incoming notification activity: application link-up is queued first, and controller output is enabled only after an eligible notification has been copied. This is **not a lamp-based startup handshake**. Initial darkness alone does not identify the failing stage.

| Symptom | Meaning / next check |
|---|---|
| `READY` in the current TEST profile | Runtime ready, but no application link-up yet. **Not necessarily scanning**: physical GATT connection/discovery/subscription may already exist before the first eligible notification callback. |
| `CONNECTED` / `SMC CONNECTED` | Application received link-up. In this profile it is triggered from an eligible notification callback, **before MIDI decoding or control application**. It is not an end-to-end input PASS. |
| Lamps work, controls do not | Outgoing state feedback alone does not prove the input path. Check the physical DAW mode and X0X preset, then visible page/overlay. |
| Fader shows `PICK` but value stays still | Expected pickup, not absent input. Reach/cross the target. |
| Wrong parameter or drum changes | Check page, kit and bank; opening a menu restores main-map faders. |
| R toggles recording unexpectedly | Use SELECT to change the target without changing REC. |
| Last five strips do nothing | Expected in drum bank 2. |
| Knob works but paired fader stops | Reacquire pickup after the knob's value change. |

Raw packets, successful subscription submission, notification counts and dispatch counts are different stages; **none alone proves that the intended control changed X0X**. Look for the corresponding selected drum, mute, transport or parameter change; a fader can be received correctly while still waiting for pickup. No new hardware success is claimed here. [S9](SOURCES.md#s9-ready-connected-and-first-feedback)
