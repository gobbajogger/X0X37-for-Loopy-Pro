# SMC-Mixer input map

Choose **GLOBAL → BLUETOOTH MIDI → BT ROLE → SMC HOST**, enable Bluetooth, then restart. Choose **SMC PRESET → DAW** for the mixer's factory DAW mode or **USER** for its editable User preset. A role change takes effect after restart; the preset setting takes effect immediately. The SMC connection is separate from the APC40/Loopy port.

The eight faders use pickup. A fader starts changing a value when it reaches the X0X target (within two MIDI steps, or crossing it). **PICK F1 …** means fader 1 is waiting; its number is the target value, not a successful edit. **SMC F1 …** means the fader has pickup. Changing the target or changing its value elsewhere resets pickup and discards the old crossing history. X0X sends the target position back to the SMC. A frame after the fader's own write preserves pickup.

| Screen | Faders | Encoders |
| --- | --- | --- |
| HOME | 909, 808, 303A, 303B, BREAK, reverb, delay levels; master filter | Same eight strip destinations as faders |
| 909 MIX / 808 MIX | Track levels 1–8, or tracks 9–11 on faders 1–3 | Same track levels as faders in the active bank |
| Other X0X pages | Main mixer, same as HOME | Four visible fields; knobs 5–8 inactive. Main MIX uses the same eight strip destinations as faders |
| Open menu | Main mixer, same as HOME | Six visible rows. Headings, actions and unused positions are inactive |
| Clip grid | Main mixer, same as HOME | Inactive; the hidden X0X page cannot receive edits |
| SCALE / CHORD / ARP editor | Main mixer, same as HOME | First four / four / eight visible fields; ARP LATCH is accessible through field navigation |
| Help / confirmation | Inactive | Inactive |

The three mixer modes are the keyboard-equivalent main mix, 909 tracks and 808 tracks. In these modes, knobs 1–8 trim the same destinations as faders 1–8. Each control type has unique destinations; a paired fader and knob may share one. Turning a knob resets pickup for its paired fader when the value changes. Other sound pages, menus and clip editors retain their visible-field controls. Duplicate fields within the visible knob bank use the first position only.

## Page lock

Pause on an SMC toggles a lock for that SMC only. In this firmware Pause is note 0x5D. The stock button map calls that note STOP. One press does not stop the sequencer. PLAY stays note 0x5E and still starts playback. The keyboard's own stop control is unchanged.

The first press turns the lock on and stores the mix that SMC is already using. A visible 909 MIX page stores 909 MIX. A visible 808 MIX page stores 808 MIX. Any other screen stores the main mix, because that is the fader mode already in use. While the lock is on, that SMC's faders and knobs stay on the stored mix. Changing the keyboard screen does not move them. A 909 or 808 lock keeps Bank Left and Bank Right on that kit's two banks after the screen leaves the drum MIX page. The Pause light is on while the lock is on.

Another press turns the lock off. That SMC follows the screen again. A changed fader target still clears pickup for that fader.

Help and a confirmation still block the faders and the knobs. Navigation, mute, solo, record, and transport other than this Pause key keep their current actions. During a performance test, Pause, PLAY and back still abort the test. The lock toggle applies only when a performance test is not running.

## Two mixers

The keyboard can hold two SMC connections at the same time. Each SMC has its own lock, its own mix mode, its own pickup and its own solo hold. Button lights and fader positions go back out on the connection that owns them. After the first SMC connects, scanning accepts a second device whose name starts with SMC. Disconnect clears that SMC's lock, pickup and solo bits, then applies whatever solo the other SMC still holds.

Two SMCs locked to the same kit share that kit's one track selection. USB and the peripheral User stream have no sender identity. Those bytes stay on the first mixer. They are not a second mixer.

The second central connection uses spare room in the existing Bluetooth RAM object. It does not add a second object past the 4096-byte Bluetooth RAM. The controller configuration allows two links. The controller image already on a unit stays one link until that image is rebuilt and installed. This change does not install it.

Each kit has **11 drum tracks**. Engine slot 12 is kit-wide controls, not another drum. Earlier SMC documentation and a test fixture incorrectly claimed twelve tracks. Two banks remain: **Bank Left** selects tracks **1–8** and **Bank Right** selects **9–11** on drum MIX pages. Faders, knobs and SELECT buttons 4–8 are inactive in the second bank. SELECT 1–8 chooses the track within the current bank, so the page and deep menu can edit each track's pan and sends. The SELECT lights show the chosen track. Keyboard selection also sets the bank. Each kit keeps its selection.

## Navigation

All required paths use buttons present in both the factory DAW and stock User presets.

| Control | Action |
| --- | --- |
| SELECT 1–5 outside drum MIX | Select 909 / 808 / 303A / 303B / BREAK and open its sound pages |
| Up / Down, ordinary page | Cycle HOME, sound, sequence, generator (303 only), FX, MIX and song views |
| Left / Right, ordinary page | Previous / next page within that view |
| Bank Left / Right, drum sound page | Previous / next drum track, bounded at 1 and 11 |
| Bank Left / Right, drum MIX | First / second bank; SELECT then chooses the track. A 909 or 808 page lock keeps these two banks after the screen leaves that MIX page |
| Bank Left / Right, other ordinary pages | Previous / next part |
| R6 | Open the current view's deep menu; activate its selected action. In clip mode, open Global |
| R7 | Back / close menu, cancel confirmation, dismiss help; close a clip editor to the grid |
| R8 | HOME |
| S6 / S7 / S8 | MIX / Global menu / selected part's sound |
| SELECT 6 / 7 / 8 outside drum MIX | Reverb / delay / master page |
| Up / Down, menu | Select previous / next row; the list scrolls to expose all rows |
| Left / Right, menu | Edit selected row, including rows also assigned to faders |
| R6 / R7, confirmation | Confirm / cancel; other SMC controls are blocked |
| Up / Down, clip mode | Cycle grid, SCALE, CHORD and ARP editors |
| Bank Left / Right, clip editor | Select previous / next visible field, including ARP LATCH |
| Left / Right, clip editor | Edit the highlighted field |

Channel Left/Right messages, if supplied by a custom mapping, also select drum tracks or clip fields. The stock 43-button preset does not require them. Returning from a Global menu restores the clip editor underneath it.

For strips 1–5, **M** toggles the part's mute, **S** solos it while held, and **R** selects the part and toggles recording. Several held **S** buttons can solo several parts together. Solo clears on disconnect and while help or confirmation blocks input. With two SMCs, each SMC has its own held solo bits, and the engine hears both. Stock User MMC has no release event, so **S** latches there. On strips 6–8, **M** toggles reverb, delay or master output. **PLAY** starts playback and **REC** toggles recording. **Pause** (the stock STOP note) toggles that SMC's page lock and does not stop playback. Rewind and fast-forward move the selected part's pattern. These actions do not send APC40 clip commands. Performance tests own the controls and drain SMC input without editing. During a performance test, Pause, PLAY and back still abort the test.

Fader 8's main-mixer master filter matches the keyboard's bank-B filter: low-pass below the middle, neutral in the middle, and high-pass above it. Strip 8's M button toggles master output, and its light reflects that output mute. Drum MIX temporarily assigns fader 8 to track 8 in the first bank and leaves it inactive in the second bank.

## MIDI messages

Factory DAW mode: faders are pitch bend on MIDI channels 1–8; encoders are sign-magnitude relative CC 16–23 on channel 1; buttons are Mackie Control note presses. X0X returns note velocities 127/0 for button lights and pitch bend for fader pickup.

The stock User preset sends faders as CC 40–47, encoders as relative CC 16–23, and its 43 buttons as six-byte MMC SysEx (`F0 7F 7F 06 01..2B F7`). On the direct SMC Bluetooth connection, X0X recognizes these messages regardless of MIDI channel because the connection identifies the controller.

For an SMC routed through a computer or another Bluetooth MIDI host into X0X, choose **SMC PRESET → USER** and program its channel messages to **MIDI channel 16**. Only that channel's CC and note messages enter the host SMC mapping, so normal instrument and APC40 messages keep their existing behavior. The exact stock User MMC SysEx envelope is accepted from the USB or peripheral Bluetooth MIDI input when User preset is selected. This host route does not identify the physical sender; use a dedicated MIDI port or routing rule if another device sends the same MMC commands.
