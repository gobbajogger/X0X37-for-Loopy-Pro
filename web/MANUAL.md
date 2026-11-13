# DnBOX manual

This page covers install, samples and project files in the browser.

DnBOX turns an SMK-37 Elite or SMK-37 Pro into a groovebox. It has 909 and 808 drums, a BREAK sample player, two bass slots, and Loopy Pro control. At firmware source `94197fc8`, the bass slots can run Chicago, Miami, Juno or Drone. Chicago is the 303 engine. Miami is never a 303.

The browser workspace in this tree imports project revision 13. Keep backups with the firmware version that made them.

For keyboard controls, read [docs/MANUAL.md](../docs/MANUAL.md). That book matches this source. It is the control manual. This page does not replace it.

## Quick start: real musical tasks

The control book has a short path through four real musical tasks. Read [Quick start: real musical tasks](../docs/MANUAL.md#3-quick-start-real-musical-tasks) in `docs/MANUAL.md`. That book is the control manual. This page does not replace it.

### Make a drum beat

1. Stay on the 909 step grid, or press SEQ to return to it. See [First sound](../docs/MANUAL.md#4-first-sound).
2. Do not tap PATCH. A tap of PATCH opens HOME. Do not press KNOB BANK. KNOB BANK opens Loopy clip mode.
3. While playback does not run, press black key 1. This selects the bass drum (`BD`). You hear it.
4. Press white keys 1, 5, 9 and 13. These write kick steps on the selected track.
5. Press black key 7. This selects the clap (`CP`). Press a white key to place a clap step. See [First pattern](../docs/MANUAL.md#5-first-pattern).
6. Press black key 8. This selects the closed hat (`CH`). Place hat steps the same way.
7. Press SEQ PLAY to start the pattern. Press SEQ PLAY again to stop.

### Play and record a bass line

1. Turn K3 clockwise from the 909. Three clicks select Chicago. Chicago is the 303 engine. See [First bass line: Chicago, then Miami](../docs/MANUAL.md#6-first-bass-line-chicago-then-miami).
2. Four clicks select Miami. Miami is never a 303. Use Chicago or Miami for this task.
3. Press PARA to play the selected bass slot. The 37 keys play that slot.
4. Press SEQ REC to turn record on. The toast is `RECORD ON: KEYS + KNOBS`. See [Recording and free play](../docs/MANUAL.md#12-recording-and-free-play).
5. On a bass slot, the keys play and write notes while record is on.
6. Press SEQ PLAY to start the pattern. Play the bass line on the keys.
7. Press SEQ REC again to turn record off. The toast is `RECORD OFF`.

### Save a project, and power-off

1. If the pattern runs, press SEQ PLAY to stop. See [Save, and why it waits](../docs/MANUAL.md#8-save-and-why-it-waits).
2. Tap BT. A tap is a release before 250 ms. BT is Save, not Bluetooth.
3. If the toast is `SAVES WHEN STOPPED`, `SAVES WHEN QUIET` or `REC OFF TO SAVE`, stop the pattern. Wait until the unit is quiet.
4. Look for the toast `SAVED`. That toast lasts 1300 ms. The unit writes only after you stop and the machine stays quiet.
5. Autosave is on by default. After 4 s of quiet, while stopped, the unit can save on its own, at most every 20 s. The toast is `AUTOSAVED`.
6. Power the unit off, then on. The unit loads the last written project. It does not keep other live edits.
7. If the unit has no valid saved project, it starts with empty parts.
8. A chain does not return after power-off. The project does not save a chain. See [Patterns, chains and song](../docs/MANUAL.md#11-patterns-chains-and-song).

### Live set: switch, mute and chain

1. If you are not on HOME, tap PATCH to open HOME. The header word is `HOME`. See [Patterns, chains and song](../docs/MANUAL.md#11-patterns-chains-and-song).
2. If you are already on HOME, do not tap PATCH again. That tap opens SONG.
3. Press a white key, or a pad, to select a pattern. There are 32 patterns, in two banks: 1 to 16 and 17 to 32.
4. On HOME or SONG, press OCTAVE+ for patterns 17–32. Press OCTAVE− for patterns 1–16.
5. Turn K4 to cue a pattern. A pending cue shows a NEXT glyph and a blinking pattern number.
6. On HOME, press a black key to mute a part in display order. Black keys 1 to 5 mute 909, 808, BREAK, Chicago and Miami.
7. Press two white keys to make a chain. A live chain shows a small amber `CHN`.
8. A chain is live only. The project does not save a chain. A save or a power-off does not keep it.
9. Press one white key to end the chain. Turn K4 to end the chain as well.
10. HOME keys make no sound. Press SEQ to return to the grid.

## Connect and choose firmware

Use a USB data cable and a browser that provides Web MIDI with SysEx. Desktop Chrome or Edge over HTTPS or localhost is the supported starting point. Some phones, browsers and USB adapters do not provide it. An unsupported-browser notice appears before connecting; sample preparation and local file inspection still work offline.

Close other MIDI editors. Connect the keyboard, press Connect and allow MIDI and SysEx access. Scanning means identification is in progress. Try again follows a failed lookup; Rescan checks an existing connection again. Read the reported identity and check the model printed on the keyboard. An old Elite may identify as Pro; a shared USB name does not prove the physical model.

Choose an admitted published version or select your local `.fwsc` file. The file's current validation result appears beside the chooser. A new selection clears the old package and model confirmation. A controller image is optional only when the package permits it; its format and approved digest must match the selected model. Never substitute an unrelated controller image.

## Back up and install

Stop playback and recording. Keep USB connected throughout the operation. If the update puts samples at risk, the installer reads every slot, offers an `.x0xslots` backup, and requires you to select the saved file again. Every byte must match before firmware writing begins.

The installer checks the selected package and target again immediately before OTA. A changed connection or sample snapshot refuses the operation. On completion it waits for the application to return, then restores compatible sample slots and reads them back. A partial restore is a warning: keep the named archive and resolve every skipped slot. Reconnecting does not erase that warning.

Browser workspaces on the same origin share an operation lock. Close other MIDI applications too: this firmware does not provide an atomic snapshot-to-OTA ticket against every external client. A last-moment concurrent write from another application remains a limitation.

## Recovery and older firmware

If the keyboard is already in its update loader, the old application's sample reader is unavailable. Resume only with a verified existing backup, or after confirming that there were no samples to preserve. The explicit recovery prompt is not a claim that a new backup was made.

For stock firmware, obtain the exact model's package from the manufacturer and use the visible local-file chooser. Stock firmware may have no DnBOX sample slots. Keep the exported archive for restoration after returning to compatible firmware. Interrupted installation, partial sample restoration and successful firmware transfer are different outcomes.

## Prepare and preview samples

Preparation sits below installation and recovery in the Install workspace. Add audio without connecting a keyboard. Choose a storage rate and optional speed-up on import. The preview includes IMA ADPCM encoding. The page shows the encoded audio size and actual playback rate before sending; slot headers need additional space.

Speed-up reduces the number of stored samples and lowers their playback rate to preserve duration. It changes the sound. Listen to the preview; a smaller payload is not a claim of identical audio quality. Open Samples to transfer the exact processed audio into its local list. Nothing is sent to the keyboard by this navigation.

## Manage samples and backups

In Samples, connect and read the current slots before sending. Offline prepared loops stay in the local list when the keyboard is read. Add, replace and remove loops, then review the available room. Detect first bar and Trim silence change the audio; turn them off when the full duration is needed. Removing a loop can change later loop numbers used by patterns.

Back up all slots reads every slot and keeps original encoded bytes, playback rates and order. The archive parser rejects reordered slots, false empty flags, overlapping or trailing payloads, invalid zones and damaged checksums. Before a write, the page checks that the keyboard still matches its last read and retains a pre-write recovery download. Keep that file until you have checked the result.

Restore preserves slot numbering. It lists slots that do not fit and reports a partial result rather than claiming complete success. Complete means every selected slot was read back against its original bytes. This does not prove that an unrelated or incomplete personal archive contains all your earlier samples.

## Projects

Export a portable project with original samples for a complete project backup. A project-only export contains sample identities and requires matching sample slots at import. The page checks the file before preparing an import preview. A recovery backup is available before replacing occupied slots.

The browser workspace imports revision 13 only. You can inspect earlier files, but this workspace does not migrate engine projects. Keep their original firmware and backups. Import replaces the live project. Save on the keyboard stores it. Load and Reset require explicit confirmation and keep uploaded sample slots.

A disconnect invalidates the device session and import preview. Local selected files and recovery downloads remain available. Reconnect, read the new target, and prepare a fresh preview. If firmware holds audio after an interrupted sample transfer, use the complete portable recovery backup described by the Projects workspace before playing.

## Keyboard controls

The full keyboard control manual is [docs/MANUAL.md](../docs/MANUAL.md). That book matches firmware source `94197fc8`. It uses the SMK printed legends.

The part order is 909, 808, BREAK, Chicago, then Miami. The bass engines are Chicago, Miami, Juno and Drone. Chicago is the 303 engine. Miami is never a 303.

This page is the install, samples and projects companion. It is not a second control manual. Do not treat an older `legacy.html` copy as the current control book.

A website build, a host test or a package checksum does not establish hardware acceptance.
