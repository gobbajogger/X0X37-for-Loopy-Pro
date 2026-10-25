# SMC-Mixer input map

Choose **GLOBAL → BLUETOOTH MIDI → BT ROLE → SMC HOST**, enable Bluetooth, then restart. Choose **SMC PRESET → DAW** for the mixer's factory DAW mode or **USER** for its editable User preset. A role change takes effect after restart; the preset setting takes effect immediately. The SMC connection is separate from the APC40/Loopy port.

The eight faders use pickup. A fader starts changing a value when it reaches the X0X target (within two MIDI steps, or crossing it). The temporary SMC screen shows the actual destinations and values. **PICKUP: MOVE UP/DOWN TO TARGET** means the fader is waiting; the displayed value is the target, not a successful edit. **ACTIVE** means pickup is acquired. Changing the target or changing its value elsewhere resets pickup and discards the old crossing history. X0X sends the target position back to the SMC. A frame after the fader's own write preserves pickup.

Fader, encoder and action feedback uses the full control screen instead of a yellow SMC header notice. The screen returns to the underlying UI after 1.2 seconds without control activity. Changed fader samples, nonzero encoder turns and action presses extend that interval. Unchanged absolute samples and zero encoder deltas do not open or extend it. Page/target feedback refresh is silent, including after navigation cancels an active screen. It does not change the underlying view, page, selection, menu or clip editor. Panel input and intentional SMC navigation dismiss it immediately; an old timeout cannot undo the new navigation. Help keeps its card while controls work underneath. Confirmation retains its input lock. Clip-editor feedback shows the field edited by the SMC; clip-grid encoders remain inactive. These behaviors have host coverage; physical LCD and controller behavior still require device testing.

| Screen | Faders | Encoders |
| --- | --- | --- |
| HOME | 909, 808, 303A, 303B, BREAK, reverb, delay levels; master filter | K1–5: corresponding part pan; K6: reverb decay; K7: delay feedback; K8: filter resonance |
| 909 MIX / 808 MIX; 909 SEQ / 808 SEQ | Track levels 1–8, or tracks 9–11 on faders 1–3 | Corresponding voice Pan in the active bank |
| Other X0X pages | Main mixer, same as HOME | Four visible fields; knobs 5–8 inactive. Main MIX PARTS uses the HOME knob map. GLOBAL/PERFORM K3 and MIX/MASTER K4 control filter resonance |
| Open menu | Main mixer, same as HOME | Six visible rows. Rows already on any active fader are parked and shown as ON FADER n; headings, actions and unused positions are inactive |
| Clip grid | Main mixer, same as HOME | Inactive; the hidden X0X page cannot receive edits |
| SCALE / CHORD / ARP editor | Main mixer, same as HOME | First four / four / eight visible fields; ARP LATCH is accessible through field navigation |
| Help | Same as the page underneath; the card stays until R7 | Same as the page underneath; the card stays until R7 |
| Confirmation | Inactive | Inactive |

The three mixer modes are the keyboard-equivalent main mix, 909 tracks and 808 tracks. Drum SEQ and drum MIX use the same drum-strip mapping. Each kit keeps its selected drum and bank across these views. Faders keep their existing levels, returns and DJ filter. Knobs provide pan, reverb decay, delay feedback and filter resonance as listed above. This gives the musician a separate dimension of control on every populated mixer strip. Other sound pages, menus and clip editors retain their visible-field controls, with any ordinary row already on an active fader parked. No unrelated parameter is substituted. Duplicate fields within the visible knob bank use the first position only.

The screen keeps all eight physical strips visible in left-to-right order. Each encoder sits above its fader, with the three button positions beside the fader in physical M/S/R order. Their captions name actual actions: PART/HOME on the filter strip, MIX and GLOB (Global) shortcuts, context-dependent LIST/OK and SONG/HOME/BACK, or GRID in the clip editor. Drum buttons remain M/S/R; X means locked and -- means unmapped. Shared shortcut descriptors and key resolution drive both input and captions. Knob arcs use the keyboard renderer and mapped runtime value; bipolar controls have a center mark. The exact lower value/unit and fader pickup message remain. The cryptic static footer legend is removed. The selected lane has a solid title highlight instead of an on-screen SELECT button. The touched control keeps its separate colored outline and value readout; LOCK in the header identifies captured control targets. Selection follows the current part or the main MIX channel editor selection, or the displayed kit’s selected drum; if it lies outside the displayed locked bank, no lane title is highlighted. Physical SELECT behavior is unchanged. The lower readout names the touched destination and formatted value. Its heading identifies the real page, kit and drum bank. Pickup shows direction toward the displayed target and separate target/physical-position markers; ACTIVE means the control can edit. The screen remains a temporary presentation over the underlying UI. The geometry is adapted to the 240 by 240 display; it is not a physical-scale drawing.

Each kit has **11 drum tracks**. Engine slot 12 is kit-wide controls, not another drum. Earlier SMC documentation and a test fixture incorrectly claimed twelve tracks. Two banks remain: **Bank Left** selects tracks **1–8** and **Bank Right** selects **9–11** in drum MIX and drum SEQ. Faders and knobs on strips 4–8 are inactive in the second bank; all four button rows on those strips are inactive and unlit. Keyboard selection also sets the bank. Each kit keeps its selection.

## Drum buttons and SMK pads

For live switching, open MIX and use **Left/Right** to move through **PARTS → 909 MIX → 808 MIX → MASTER → WAVE**, without opening a menu. PARTS uses the X0X main strip layout; the two drum pages use their individual sounds. MASTER keeps its peak-history / COMP view; WAVE is the sibling post-master scope (same knobs, GR bar kept). Bank Left/Right changes the eight-strip drum bank. Drum SEQ follows its selected kit. This is a page-following map: other sound and effect pages retain the main faders and visible-field knobs. SMC SELECT strip 8 still lands on MASTER (page index 3).

On **909 MIX**, **808 MIX**, **909 SEQ** and **808 SEQ**, all four button rows follow the same kit and bank as the faders and knobs. MIX uses the kit shown by its page; SEQ uses the selected drum part.

| Button | Action on the visible drum strip | Light |
| --- | --- | --- |
| Record | Select that kit and drum, then toggle the existing recording mode | Recording is on and this drum is selected for that kit |
| Solo | Isolate that drum while held; release restores the ordinary mix | This drum has an active Solo target |
| Mute | Toggle that drum's mute | That drum is muted |
| SELECT | Select that kit and drum without changing recording; in drum MIX, pressing the already-selected drum returns to main MIX | This drum is selected |

The **SMK pads sequence the selected drum in either drum SEQ or drum MIX**, and the current view stays open. The pad step lights follow that drum. Record and SELECT focus the visible kit as well as its drum, even if a different part was previously selected. Record uses the existing recording mode; it does not create separate record-arm states for individual drums.

Several held Solo buttons can isolate several drums together. Each release belongs to the target selected when its button was pressed, even after changing the page, kit or bank. Solo is temporary runtime state: it does not rewrite drum or part mutes, and it clears on disconnect, blocked input and project reset. Stock User MMC has no release event, so Solo uses its existing latch fallback there: pressing the same target again clears it; pressing that physical Solo key in another bank replaces its previous target. Existing reverb and delay tails are preserved.

R6–R8 and S6–S8 are drum controls on these pages. Use Up/Down to change views and Left/Right to change pages. The navigation shortcuts below apply outside drum MIX and drum SEQ; open menus, help, confirmations and clip editors keep their own button priority.

## Navigation

All required paths use buttons present in both the factory DAW and stock User presets.

| Control | Action |
| --- | --- |
| SELECT 1–5 outside drum MIX/SEQ | Select 909 / 808 / 303A / 303B / BREAK and open its sound pages; pressing the already-selected 909/808 opens that kit's individual-sound mixer |
| Up / Down, ordinary page | Cycle HOME, sound, sequence, generator (303 only), FX, MIX and song views |
| Left / Right, ordinary page | Previous / next page within that view |
| Bank Left / Right, drum sound page | Previous / next drum track, bounded at 1 and 11 |
| Bank Left / Right, drum MIX/SEQ | First / second bank; SELECT then chooses the track |
| Bank Left / Right, other ordinary pages | Previous / next part |
| R6 outside drum MIX/SEQ | Open the current view's deep menu; activate its selected action. In clip mode, open Global |
| R7 outside drum MIX/SEQ | Back / close menu, cancel confirmation, dismiss help; close a clip editor to the grid |
| R8 outside drum MIX/SEQ | HOME |
| S6 / S7 / S8 outside drum MIX/SEQ | MIX / Global menu / selected part's sound |
| SELECT 6 / 7 / 8 outside drum MIX/SEQ | Reverb / delay / master page |
| Up / Down, menu | Select previous / next row; the list scrolls to expose all rows |
| Left / Right, menu | Edit selected row, including rows also assigned to faders |
| R6 / R7, confirmation | Confirm / cancel; other SMC controls are blocked |
| Up / Down, clip mode | Cycle grid, SCALE, CHORD and ARP editors |
| Bank Left / Right, clip editor | Select previous / next visible field, including ARP LATCH |
| Left / Right, clip editor | Edit the highlighted field |

Channel Left/Right messages, if supplied by a custom mapping, also select drum tracks or clip fields. The stock 43-button preset does not require them. Returning from a Global menu restores the clip editor underneath it.

Outside drum MIX and drum SEQ, strips 1–5 retain their part controls: **M** toggles the part's mute, **S** solos it while held, and **R** selects the part and toggles recording. Several held **S** buttons can solo several parts together. On strips 6–8, **M** toggles reverb, delay or master output. **PLAY** toggles play/pause using the sequencer's existing start/stop operations, including a queued start; **STOP** stops and **REC** controls recording. Rewind and fast-forward move the selected part's pattern. Holding a DAW PLAY or SELECT Note does not repeat the action; release enables the next press. Stock User MMC has no release and retains one action per command. These actions do not send APC40 clip commands. Performance tests own the controls, clear temporary Solo and drain SMC input without editing.

## Prepared target lock and physical identification limit

The reviewed PR150 target-capture mechanism is adapted to the distinct current knob mappings. Its neutral press/release API captures fader, knob, drum-bank, pattern and song-bar targets; keyboard navigation then leaves those targets stable. The control screen still expires normally. SELECT, row Record and SMC target navigation are parked while locked, so a repeated SELECT cannot silently redirect controls. Unlock follows the current UI and resets pickup. Clip mode refuses lock entry; a lock acquired outside clip mode survives subsequent keyboard clip navigation. Song structural edits invalidate captured song-bar knobs until unlock.

This source API has no physical Pause binding. The current replacement SMC's Pause event and light have not been identified by labelled project evidence. Historical note53/CC53 remains unconfirmed. Existing STOP `0x5D` is preserved. No Pause LED address or blink command is guessed; the requested blinking-follow/steady-lock indication remains pending that evidence. Host API tests and native rendering do not establish physical availability or hardware validation.

Fader 8's main-mixer master filter matches the keyboard's bank-B filter: low-pass below the middle, neutral in the middle, and high-pass above it. Strip 8's M button toggles master output, and its light reflects that output mute. Drum MIX and drum SEQ assign fader 8 to track 8 in the first bank and leave it inactive in the second bank.

## MIDI messages

Factory DAW mode: faders are pitch bend on MIDI channels 1–8; encoders are sign-magnitude relative CC 16–23 on channel 1; buttons are Mackie Control note presses. X0X returns note velocities 127/0 for button lights and pitch bend for fader pickup.

The stock User preset sends faders as CC 40–47, encoders as relative CC 16–23, and its 43 buttons as six-byte MMC SysEx (`F0 7F 7F 06 01..2B F7`). On the direct SMC Bluetooth connection, X0X recognizes these messages regardless of MIDI channel because the connection identifies the controller.

For an SMC routed through a computer or another Bluetooth MIDI host into X0X, choose **SMC PRESET → USER** and program its channel messages to **MIDI channel 16**. Only that channel's CC and note messages enter the host SMC mapping, so normal instrument and APC40 messages keep their existing behavior. The exact stock User MMC SysEx envelope is accepted from the USB or peripheral Bluetooth MIDI input when User preset is selected. This host route does not identify the physical sender; use a dedicated MIDI port or routing rule if another device sends the same MMC commands.
