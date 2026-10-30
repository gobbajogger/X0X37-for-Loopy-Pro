# SMC-Mixer input map

Choose **GLOBAL → BLUETOOTH MIDI → BT ROLE → SMC HOST**, enable Bluetooth, then restart. Choose **SMC PRESET → DAW** for the mixer's factory DAW mode or **USER** for its editable User preset. A role change takes effect after restart; the preset setting takes effect immediately. The SMC connection is separate from the APC40/Loopy port.

## Two direct Bluetooth mixers

The development controller and application support two direct SMC links. Each
link has its own input ring, reconnect epoch, captured targets, pickup history,
held buttons, Solo targets and feedback cache. Pause locks only the controller
that sent it. Keyboard navigation does not redirect either controller's captured
targets. The temporary display shows the last controller used; it does not retain
or restore a different underlying page.

Solo is the union of both controllers' held targets. Releasing or disconnecting
one does not release the other's Solo. Momentary FX holds also retain their
owners. If both controllers hold different effects on the same engine bus, the
first connection slot has priority until it releases; the second controller's
still-held effect then becomes effective. The engine still has one effect per
bus. Project reset clears both controllers' held state and old queued input.

USB/peripheral User-preset input retains its historical association with the
first direct-link slot; it is not a third independently identifiable controller.
A shared host MIDI port cannot identify multiple physical senders. Use direct
Bluetooth links for independent controller contexts.

This change requires the matching two-link controller image as well as the app.
The existing single-link hardware acceptance is not discarded, but it does not
prove the changed controller or simultaneous links. Production host tests cover
two-peer routing, overload, reconnect, HCI command arbitration and per-handle ACL
credit conservation. Radio timing, simultaneous physical devices, actual lamps
and device acceptance remain a separate authorized hardware stage.

The eight faders use pickup. A fader starts changing a value when it reaches the X0X target (within two MIDI steps, or crossing it). The temporary SMC screen shows the actual destinations and values. **PICKUP: MOVE UP/DOWN TO TARGET** means the fader is waiting; the displayed value is the target, not a successful edit. **ACTIVE** means pickup is acquired. Changing the target or changing its value elsewhere resets pickup and discards the old crossing history. X0X sends the target position back to the SMC. A frame after the fader's own write preserves pickup.

Fader, encoder and action feedback uses the full control screen instead of a yellow SMC header notice. The screen returns to the underlying UI after 1.2 seconds without control activity. Changed fader samples, nonzero encoder turns and action presses extend that interval. Unchanged absolute samples and zero encoder deltas do not open or extend it. Page/target feedback refresh is silent, including after navigation cancels an active screen. It does not change the underlying view, page, selection, menu or clip editor. Panel input and intentional SMC navigation dismiss it immediately; an old timeout cannot undo the new navigation. Help keeps its card while controls work underneath. Confirmation retains its input lock. Clip-editor feedback shows the field edited by the SMC; clip-grid encoders remain inactive. These behaviors have host coverage; physical LCD and controller behavior still require device testing.

A main-mixer fader move also peeks the real page of that strip under the control screen: faders 1-5 open MIX PARTS with that part selected, fader 6 opens FX REVERB, fader 7 opens FX DELAY and fader 8 opens MIX MASTER, with live meters. About 1.2 seconds after the last move, X0X returns to the previous view, page and MIX selection. Open menus, help, clip mode and performance tests do not peek, but the control screen still shows. Any navigation, confirmation, blocked controls, SMC LOST or a project reset ends the peek where you are. With two SMCs, the first one to peek owns it until it ends. (BEHAVIOR SMC-12)

| Screen | Faders | Encoders |
| --- | --- | --- |
| HOME | 909, 808, 303A, 303B, BREAK, reverb, delay levels; master filter | K1–5: that part's own cutoff (127 is open); K6: reverb decay; K7: delay feedback; K8: filter resonance |
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
| SELECT 1, 2, 3 and 5 outside drum MIX/SEQ | Momentary effect on 909, 808, 303A and BREAK. Home + that button changes only its effect |
| SELECT 4 outside drum MIX/SEQ | Select 303B and open its sound pages |
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

Outside drum MIX and drum SEQ, the SELECT buttons beside the 909, 808, 303A and BREAK faders are momentary effects. See the bus map below. **M** still toggles that part's mute, **S** still solos it while held, and **R** still selects the part and toggles recording. 303B's SELECT still opens that part. Drum MIX and drum SEQ keep SELECT on the drum.

## Momentary effects on four buses

The four buttons are the SELECT keys beside those faders. A press holds the effect. Releasing it returns the bus, without a click. A reverb or delay throw keeps ringing in the existing send after the finger comes up. Hold Home and press one of those buttons to change only that button's effect. Hold Home and press Left or Right to change the page. All four buttons are filled on every page.

| Page | 909 | 808 | 303A | BREAK |
| --- | --- | --- | --- | --- |
| ROOM | short reverb throw | delay throw | high-pass | decay gate |
| TIGHT | decay gate | short reverb throw | delay throw | high-pass |
| ECHO | delay throw | decay gate | short reverb throw | delay throw |

Each part's knob, including 303B, is that part's cutoff. It is applied on the bus. The master filter still comes after it. Hold Home and press Up to store the five cutoffs as the reset preset. Hold Home and press Down to snap back to that preset, or to fully open if none has been stored. The 303 mod wheel is unchanged.

Several held **S** buttons can solo several parts together. On strips 6–8, **M** toggles reverb, delay or master output. **PLAY** toggles play/pause using the sequencer's existing start/stop operations, including a queued start; **STOP** stops and **REC** controls recording. Rewind and fast-forward move the selected part's pattern. Holding a DAW PLAY or SELECT Note does not repeat the action; release enables the next press. Stock User MMC has no release and retains one action per command. These actions do not send APC40 clip commands. Performance tests own the controls, clear temporary Solo and drain SMC input without editing.

## Target lock and beat feedback

The target-capture mechanism retains fader, knob, drum-bank, pattern and song-bar destinations while keyboard navigation changes the visible page. The control screen still expires normally. SELECT, row Record, drum sound selection and SMC navigation keep their normal actions while locked. Fader and knob destinations stay captured. Unlock follows the current UI and resets pickup. Clip mode refuses lock entry; a lock acquired outside clip mode survives subsequent keyboard clip navigation. Song structural edits invalidate captured song-bar knobs until unlock.

The existing captured Pause route is Note `0x5D`. A press while following locks immediately. Unlock requires a separate press held for 900 ms; release before that point cancels it. Repeated press messages do not restart the timer, and the press that locked cannot also unlock. Disconnect/reconnect, project ownership, confirmation, performance testing and a lost Pause release cancel a pending hold. Pause never sends a transport action; User-preset MMC STOP and Play keep their separate behavior.

The existing NoteOn feedback route is steady while locked. While following, it shows the lit first half of each beat from the sequencer's accepted external clock, including clock pickup while stopped. Clock loss keeps the last measured visual BPM and phase; Stop does not stop this visual beat. A new external-clock pickup rephases it. With no external clock history, feedback follows the internal tempo; a manual internal-tempo edit starts the new visual period. Project reset clears the lock and the local visual state.

These behaviors implement the preserved LX-REQ-006/022 owner contract. Production host tests check input queues, hold/cancel/loss edges, clock phase and emitted MIDI bytes. They do not establish the physical light identity, controller timing, device behavior or hardware acceptance. No new note/CC mapping or transport binding is inferred.

Fader 8's main-mixer master filter matches the keyboard's bank-B filter: low-pass below the middle, neutral in the middle, and high-pass above it. Strip 8's M button toggles master output, and its light reflects that output mute. Drum MIX and drum SEQ assign fader 8 to track 8 in the first bank and leave it inactive in the second bank.

## MIDI messages

Factory DAW mode: faders are pitch bend on MIDI channels 1–8; encoders are sign-magnitude relative CC 16–23 on channel 1; buttons are Mackie Control note presses. X0X returns note velocities 127/0 for button lights and pitch bend for fader pickup.

The stock User preset sends faders as CC 40–47, encoders as relative CC 16–23, and its 43 buttons as six-byte MMC SysEx (`F0 7F 7F 06 01..2B F7`). On a direct SMC Bluetooth connection, channel controls use channel 1 or User channel 16. Channels 2–15 are ignored, including on overflow. MMC uses its exact six-byte envelope. The peer slot identifies the physical controller; the preset is captured when each frame arrives.

For an SMC routed through a computer or another Bluetooth MIDI host into X0X, choose **SMC PRESET → USER** and program its channel messages to **MIDI channel 16**. Only that channel's CC and note messages enter the host SMC mapping, so normal instrument and APC40 messages keep their existing behavior. The exact stock User MMC SysEx envelope is accepted from the USB or peripheral Bluetooth MIDI input when User preset is selected. This host route does not identify the physical sender; use a dedicated MIDI port or routing rule if another device sends the same MMC commands.

## Reconnect and feedback boundaries

Each peer publishes its link epoch together with lost-release watermarks. Old-link frames and old host Solo edges cannot replay into a new connection. Peer zero retains the historical USB/peripheral host association; peer one's disconnect does not clear that host's Solo. Preset changes clear held controls from the old mapping. Feedback is acknowledged only after the transmit queue accepts it in the same link epoch; rejected writes retry. Repeated saturated MMC STOP commands retain their separate FIFO boundaries on both host sources.

The Bluetooth NAME row lists all connected central peers in slot order, separated by a comma, including identical advertised names. Each name is bounded to 20 bytes. Long names use a smaller native font; if necessary each peer is shortened separately with `..` so the other peer stays visible. Disconnected cached names are omitted. The existing Bluetooth state and About paths remain in use. Host checks do not establish physical radio, LCD or LED acceptance.
