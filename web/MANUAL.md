# DnBOX manual

DnBOX turns an SMK-37 keyboard into a groovebox with 909 and 808 drums, Chicago and Miami synthesizers, a BREAK sample player, and Loopy Pro control. This development tree uses project revision 13. Keep backups with the firmware version that made them.

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

The current Chicago/Miami build imports revision 13 only. Earlier files can be inspected, but this workspace does not migrate engine projects. Keep their original firmware and backups. Import replaces the live project; Save on keyboard stores it. Load and Reset require explicit confirmation and keep uploaded sample slots.

A disconnect invalidates the device session and import preview. Local selected files and recovery downloads remain available. Reconnect, read the new target, and prepare a fresh preview. If firmware holds audio after an interrupted sample transfer, use the complete portable recovery backup described by the Projects workspace before playing.

## Detailed keyboard controls and provenance

The [keyboard control reference](legacy.html) preserves the project's full existing manual and diagrams. Its older engine and firmware descriptions are historical; current revision 13 uses only Chicago and Miami in the two synth slots. See the repository's SMK37.md for the physical control map. A website build, host test or package checksum does not establish hardware acceptance.
