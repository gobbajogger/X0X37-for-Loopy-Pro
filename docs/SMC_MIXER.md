# SMC-Mixer input map

Choose **GLOBAL → BLUETOOTH MIDI → BT ROLE → SMC HOST**, enable Bluetooth, then restart. Choose **SMC PRESET → DAW** for the mixer's factory DAW mode or **USER** for its editable User preset. A role change takes effect after restart; the preset setting takes effect immediately. The SMC connection is separate from the APC40/Loopy port.

The eight faders use pickup. A fader starts changing a value when it reaches the value currently shown by X0X (within two MIDI steps, or crossing it). A changed page or another change to that value resets pickup. X0X sends the target fader position back to the SMC so its pickup light can guide the move.

| Control | HOME and clip grid | Other X0X pages |
| --- | --- | --- |
| Faders 1–5 | 909, 808, 303A, 303B, BREAK levels | Current page's four values, then next page's four values |
| Faders 6–8 | Reverb level, delay level, master level | Same page rule; a one-page view uses the selected part's level, pan, reverb and delay sends for faders 5–8 |
| Encoders 1–8 | First four X0X page values, then the next four or the selected part's mix | The visible page's fields, one value step per click; on an open SCALE, CHORD or ARP page, the eight positions follow its two columns of fields |

For strips 1–5, **M** toggles that part's mute, **S** solos it while held, **R** selects the part and toggles recording, and **SELECT** selects the part. Several held **S** buttons can solo several parts together. Solo clears when the SMC disconnects. The stock User preset's MMC buttons provide no release event, so **S** latches on/off there. On strips 6–8, **M** toggles reverb, delay or master output; the other three buttons open their FX, mix, part or home pages. The SMC's **PLAY**, **STOP** and **REC** control X0X transport and recording. Rewind and fast-forward move the selected part's pattern. Bank and channel left/right move between parts; arrows move pages or a selected menu value. These SMC actions do not send APC40 clip commands.

## MIDI messages

Factory DAW mode: faders are pitch bend on MIDI channels 1–8; encoders are sign-magnitude relative CC 16–23 on channel 1; buttons are Mackie Control note presses. X0X returns note velocities 127/0 for button lights and pitch bend for fader pickup.

The stock User preset sends faders as CC 40–47, encoders as relative CC 16–23, and its 43 buttons as six-byte MMC SysEx (`F0 7F 7F 06 01..2B F7`). On the direct SMC Bluetooth connection, X0X recognizes these messages regardless of MIDI channel because the connection identifies the controller.

For an SMC routed through a computer or another Bluetooth MIDI host into X0X, choose **SMC PRESET → USER** and program its channel messages to **MIDI channel 16**. Only that channel's CC and note messages enter the host SMC mapping, so normal instrument and APC40 messages keep their existing behavior. The exact stock User MMC SysEx envelope is accepted from the USB or peripheral Bluetooth MIDI input when User preset is selected. This host route does not identify the physical sender; use a dedicated MIDI port or routing rule if another device sends the same MMC commands.
