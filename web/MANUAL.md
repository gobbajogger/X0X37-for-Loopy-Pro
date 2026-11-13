# DnBOX manual

This companion explains how to install DnBOX, back up samples and work with project files in the browser. Use the [SMK-37 keyboard manual](../docs/MANUAL.md) to play, record and navigate the instrument.

It describes the **0.0.26 hardware-test candidate** at source `94197fc8`, **not** the [recommended published 0.0.25.1 release](https://github.com/gobbajogger/X0X37-for-Loopy-Pro/releases/tag/v0.0.25.1). **Neither is fully hardware-qualified.** Release notes report Elite testing of 0.0.25.1 without a confirmed installed-package hash, and no Pro device test; the exact 0.0.26 candidate's boot/restart remain unverified. Juno, Drone and held-key chord recording belong to the 0.0.26 candidate only. Chicago is the 303 engine; Miami is not.

This browser workspace imports project revision 13. Keep a project backup made with the original firmware version; incompatible engines and older project revisions are not silently migrated.

## Start with music

The [four musical tasks](../docs/MANUAL.md#3-quick-start-real-musical-tasks) take you through a 909 beat, a Chicago/Miami bass line, saving and power-off, then switching and chaining patterns. They use the **printed SMK-37 controls**, not the older FM-1 panel labels. For the control map, see [The panel](../docs/MANUAL.md#2-the-panel). The browser tools below are for installation and file management; navigation here does not play or modify a pattern.

## Connect and choose firmware

Use a USB data cable and a browser that provides Web MIDI with SysEx. Desktop Chrome or Edge over HTTPS or localhost is the supported starting point. Some phones, browsers and USB adapters do not provide it. An unsupported-browser notice appears before connecting; sample preparation and local file inspection still work offline.

Close other MIDI editors. Connect the keyboard, press Connect and allow MIDI and SysEx access. Scanning means identification is in progress. Try again follows a failed lookup; Rescan checks an existing connection again. Read the reported identity and check the model printed on the keyboard. An old Elite may identify as Pro; a shared USB name does not prove the physical model.

On the [published main installer](https://gobbajogger.github.io/X0X37-for-Loopy-Pro/install/), choose the **visible Elite/Pro model and admitted version in the firmware list**. The separate **`Restore original firmware / local package` chooser is hidden and disabled** on that site; a downloaded `.fwsc` cannot be dropped there. The [nightly installer](https://gobbajogger.github.io/x0x37-for-Loopy-Pro_nightly/install/) has a separate visible local-file section in its published source, but that is not proof of a safe or hardware-tested stock recovery. Prefer its listed admitted packages for deliberate testing. Changing model/version clears the previous confirmation. Only supply a controller image if that specific admitted package asks for it and validates it.

## Back up and install

When the **normal app responds**, stop playback and recording. From [Samples](https://gobbajogger.github.io/X0X37-for-Loopy-Pro/breaks/) save an `.x0xslots` archive, and from [Projects](https://gobbajogger.github.io/X0X37-for-Loopy-Pro/projects/) separately export a portable `.x0xproject` including its samples; **verify both files exist on disk**. A project export and a raw sample-slot backup serve different purposes. Keep USB connected during any write. The [main installer](https://gobbajogger.github.io/X0X37-for-Loopy-Pro/install/) blocks a write if a required at-risk slot backup fails. The [nightly installer](https://gobbajogger.github.io/x0x37-for-Loopy-Pro_nightly/install/) may warn and **continue automatically without a backup**; unbacked-up samples can be lost permanently. A browser download request alone does not prove the file was retained.

The installer checks the selected package and target again immediately before OTA. A changed connection or sample snapshot may refuse the write. When a verified backup exists, the installer attempts to restore compatible sample slots after the application returns and checks the restored data. A partial restore is **not** complete: keep the named archive and resolve each skipped slot. If installation continued without backup, there may be no samples to restore.

Browser workspaces on the same origin share an operation lock. Close other MIDI applications too: this firmware does not provide an atomic snapshot-to-OTA ticket against every external client. A last-moment concurrent write from another application remains a limitation.

## Recovery and older firmware

Use the **observed state** to choose a path; an SMK-37 USB product name alone does not prove the printed Elite/Pro model.

1. **Normal DnBOX application responds (first install or re-install).** Save and check the separate sample and project backups above. In the **main installer**, Connect, grant MIDI/SysEx permission, choose the printed physical model and a **listed, admitted published DnBOX package**, then follow the visible confirmation. A file downloaded from GitHub Releases is a download, **not** a working local upload route on the main website.
2. **Only the update loader responds after an interrupted write.** Connect and check the installer's actual loader status (`ota-` identity). The running application's sample reader is unavailable, so **no new `.x0xslots` or project backup can be made from that state**. The main installer has a loader-resume path for a **listed, admitted package matched to the physical model**; it bypasses app-side sample backup. Proceed only if that exact option is available and a retained backup or the possible sample loss is acceptable. A matching USB name alone is insufficient. If no safe match is offered, stop; do not use the hidden file chooser or invent a button combination.
3. **Uncertain restart or no device found.** Wait until any in-progress write ends, then reconnect and use **Connect / Rescan** to distinguish the normal app from the update loader. Check MIDI/SysEx permission and close other MIDI apps. Do **not** guess the state from a transient disconnect or repeat an unverified write.
4. **Return to M-VAVE stock firmware.** Obtain the package for the **exact physical model** from [M-VAVE](https://www.m-vave.com/download) and follow the manufacturer's **M-UPGRADE** procedure. The main DnBOX web installer's local stock chooser is hidden; it does not contain or support uploading the factory package. Keep your existing `.x0xslots` / project archives for later use with compatible DnBOX firmware. Stock firmware may not read DnBOX sample slots.

**What completion means.** The browser's `Done` status follows a **fresh, matching application MIDI response** after the write, not a flash-byte attestation, an audio test, or a successful sample restore. Check that the normal app truly runs and plays sound, then inspect the saved project and sample slots. If sample restoration reports skipped slots, a partial result, or missing read-back, keep the recovery archive: **restoration is not complete**. Neither published package nor nightly build has full Elite/Pro hardware qualification.

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

For playing, control glyphs, chord recording and the difference between factory presets and project backups, read the source-matched [SMK-37 manual](../docs/MANUAL.md). This companion does not reproduce its controls or supersede them.

A website build, host test, matching USB identity or package checksum does not prove hardware startup, restart, audio, or sample recovery.
