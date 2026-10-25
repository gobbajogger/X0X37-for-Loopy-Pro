# SMC-Mixer input map

Choose **GLOBAL → BLUETOOTH MIDI → BT ROLE → SMC HOST**, enable Bluetooth, then restart. Choose **SMC PRESET → DAW** for the mixer's factory DAW mode or **USER** for its editable User preset. A role change takes effect after restart; the preset setting takes effect immediately. The SMC connection is separate from the APC40/Loopy port.

The eight faders use pickup. A fader starts changing a value when it reaches the X0X target (within two MIDI steps, or crossing it). The temporary **SMC CONTROL** screen shows the actual destinations and values. **PICKUP: MOVE FADER UP/DOWN** means the fader is waiting; the displayed value is the target, not a successful edit. **FADER ACTIVE** means pickup is acquired. Changing the target or changing its value elsewhere resets pickup and discards the old crossing history. X0X sends the target position back to the SMC. A frame after the fader's own write preserves pickup.

Fader, encoder and action feedback uses the full control screen instead of a yellow SMC header notice. The screen returns to the underlying UI after 1.2 seconds without control activity. Repeated activity extends that interval. It does not change the underlying view, page, selection, menu or clip editor. Panel input and intentional SMC navigation dismiss it immediately; an old timeout cannot undo the new navigation. Help keeps its card while controls work underneath. Confirmation retains its input lock. Clip-editor feedback shows the field edited by the SMC; clip-grid encoders remain inactive. These behaviors have host coverage; physical LCD and controller behavior still require device testing.

| Screen | Faders | Encoders |
| --- | --- | --- |
| HOME | 909, 808, 303A, 303B, BREAK, reverb, delay levels; master filter | Same eight strip destinations as faders |
| 909 MIX / 808 MIX; 909 SEQ / 808 SEQ | Track levels 1–8, or tracks 9–11 on faders 1–3 | Same track levels as faders in the active bank |
| Other X0X pages | Main mixer, same as HOME | Four visible fields; knobs 5–8 inactive. Main MIX uses the same eight strip destinations as faders |
| Open menu | Main mixer, same as HOME | Six visible rows. Headings, actions and unused positions are inactive |
| Clip grid | Main mixer, same as HOME | Inactive; the hidden X0X page cannot receive edits |
| SCALE / CHORD / ARP editor | Main mixer, same as HOME | First four / four / eight visible fields; ARP LATCH is accessible through field navigation |
| Help | Same as the page underneath; the card stays until R7 | Same as the page underneath; the card stays until R7 |
| Confirmation | Inactive | Inactive |

The three mixer modes are the keyboard-equivalent main mix, 909 tracks and 808 tracks. Drum SEQ and drum MIX use the same drum-strip mapping. Each kit keeps its selected drum and bank across these views. In these modes, knobs 1–8 trim the same destinations as faders 1–8. Each control type has unique destinations; a paired fader and knob may share one. Turning a knob resets pickup for its paired fader when the value changes. Other sound pages, menus and clip editors retain their visible-field controls. Duplicate fields within the visible knob bank use the first position only.

Each kit has **11 drum tracks**. Engine slot 12 is kit-wide controls, not another drum. Earlier SMC documentation and a test fixture incorrectly claimed twelve tracks. Two banks remain: **Bank Left** selects tracks **1–8** and **Bank Right** selects **9–11** in drum MIX and drum SEQ. Faders and knobs on strips 4–8 are inactive in the second bank; all four button rows on those strips are inactive and unlit. Keyboard selection also sets the bank. Each kit keeps its selection.

## Drum buttons and SMK pads

For live switching, open MIX and use **Left/Right** to move through **PARTS → 909 MIX → 808 MIX → MASTER**, without opening a menu. PARTS uses the X0X main strip layout; the two drum pages use their individual sounds. Bank Left/Right changes the eight-strip drum bank. Drum SEQ follows its selected kit. This is a page-following map: other sound and effect pages retain the main faders and visible-field knobs.

On **909 MIX**, **808 MIX**, **909 SEQ** and **808 SEQ**, all four button rows follow the same kit and bank as the faders and knobs. MIX uses the kit shown by its page; SEQ uses the selected drum part.

| Button | Action on the visible drum strip | Light |
| --- | --- | --- |
| Record | Select that kit and drum, then toggle the existing recording mode | Recording is on and this drum is selected for that kit |
| Solo | Isolate that drum while held; release restores the ordinary mix | This drum has an active Solo target |
| Mute | Toggle that drum's mute | That drum is muted |
| SELECT | Select that kit and drum without changing recording | This drum is selected |

The **SMK pads sequence the selected drum in either drum SEQ or drum MIX**, and the current view stays open. The pad step lights follow that drum. Record and SELECT focus the visible kit as well as its drum, even if a different part was previously selected. Record uses the existing recording mode; it does not create separate record-arm states for individual drums.

Several held Solo buttons can isolate several drums together. Each release belongs to the target selected when its button was pressed, even after changing the page, kit or bank. Solo is temporary runtime state: it does not rewrite drum or part mutes, and it clears on disconnect, blocked input and project reset. Stock User MMC has no release event, so Solo uses its existing latch fallback there: pressing the same target again clears it; pressing that physical Solo key in another bank replaces its previous target. Existing reverb and delay tails are preserved.

R6–R8 and S6–S8 are drum controls on these pages. Use Up/Down to change views and Left/Right to change pages. The navigation shortcuts below apply outside drum MIX and drum SEQ; open menus, help, confirmations and clip editors keep their own button priority.

## Navigation

All required paths use buttons present in both the factory DAW and stock User presets.

| Control | Action |
| --- | --- |
| SELECT 1–5 outside drum MIX/SEQ | Select 909 / 808 / 303A / 303B / BREAK and open its sound pages |
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

Outside drum MIX and drum SEQ, strips 1–5 retain their part controls: **M** toggles the part's mute, **S** solos it while held, and **R** selects the part and toggles recording. Several held **S** buttons can solo several parts together. On strips 6–8, **M** toggles reverb, delay or master output. **PLAY**, **STOP** and **REC** control X0X transport and recording; rewind and fast-forward move the selected part's pattern. These actions do not send APC40 clip commands. Performance tests own the controls, clear temporary Solo and drain SMC input without editing.

Fader 8's main-mixer master filter matches the keyboard's bank-B filter: low-pass below the middle, neutral in the middle, and high-pass above it. Strip 8's M button toggles master output, and its light reflects that output mute. Drum MIX and drum SEQ assign fader 8 to track 8 in the first bank and leave it inactive in the second bank.

## MIDI messages

Factory DAW mode: faders are pitch bend on MIDI channels 1–8; encoders are sign-magnitude relative CC 16–23 on channel 1; buttons are Mackie Control note presses. X0X returns note velocities 127/0 for button lights and pitch bend for fader pickup.

The stock User preset sends faders as CC 40–47, encoders as relative CC 16–23, and its 43 buttons as six-byte MMC SysEx (`F0 7F 7F 06 01..2B F7`). On the direct SMC Bluetooth connection, X0X recognizes these messages regardless of MIDI channel because the connection identifies the controller.

For an SMC routed through a computer or another Bluetooth MIDI host into X0X, choose **SMC PRESET → USER** and program its channel messages to **MIDI channel 16**. Only that channel's CC and note messages enter the host SMC mapping, so normal instrument and APC40 messages keep their existing behavior. The exact stock User MMC SysEx envelope is accepted from the USB or peripheral Bluetooth MIDI input when User preset is selected. This host route does not identify the physical sender; use a dedicated MIDI port or routing rule if another device sends the same MMC commands.
