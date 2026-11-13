# DnBOX manual

This companion explains how to install DnBOX, back up samples and work with project files in the browser. Use the [SMK-37 keyboard manual](../docs/MANUAL.md) to play, record and navigate the instrument.

It describes the **0.0.26 hardware-test candidate** at source `94197fc8`, **not** the stable [0.0.25.1 release](https://github.com/gobbajogger/X0X37-for-Loopy-Pro/releases/tag/v0.0.25.1). Startup/restart of this exact nightly package remain unverified on hardware. Juno, Drone and held-key chord recording belong to this candidate, not 0.0.25.1. Chicago is the 303 engine; Miami is not.

This browser workspace imports project revision 13. Keep a project backup made with the original firmware version; incompatible engines and older project revisions are not silently migrated.

## Start with music

The [four musical tasks](../docs/MANUAL.md#3-quick-start-real-musical-tasks) take you through a 909 beat, a Chicago/Miami bass line, saving and power-off, then switching and chaining patterns. They use the **printed SMK-37 controls**, not the older FM-1 panel labels. For the control map, see [The panel](../docs/MANUAL.md#2-the-panel). The browser tools below are for installation and file management; navigation here does not play or modify a pattern.

## Connect and choose firmware

Use a USB data cable and a browser that provides Web MIDI with SysEx. Desktop Chrome or Edge over HTTPS or localhost is the supported starting point. Some phones, browsers and USB adapters do not provide it. An unsupported-browser notice appears before connecting; sample preparation and local file inspection still work offline.

Close other MIDI editors. Connect the keyboard, press Connect and allow MIDI and SysEx access. Scanning means identification is in progress. Try again follows a failed lookup; Rescan checks an existing connection again. Read the reported identity and check the model printed on the keyboard. An old Elite may identify as Pro; a shared USB name does not prove the physical model.

Choose an admitted published version or select your local `.fwsc` file. The file's current validation result appears beside the chooser. A new selection clears the old package and model confirmation. A controller image is optional only when the package permits it; its format and approved digest must match the selected model. Never substitute an unrelated controller image.

## Back up and install

Stop playback and recording. Keep USB connected throughout the operation. **Make and verify a separate `.x0xslots` backup before updating.** The [main installer](https://gobbajogger.github.io/X0X37-for-Loopy-Pro/install/) currently blocks a risky firmware write if the required recovery backup cannot be secured. The [nightly installer](https://gobbajogger.github.io/x0x37-for-Loopy-Pro_nightly/install/) attempts a backup but may **continue automatically with a warning** if it fails; unbacked-up samples can then be lost permanently. The browser cannot prove a download was saved: check that the backup file exists before proceeding.

The installer checks the selected package and target again immediately before OTA. A changed connection or sample snapshot may refuse the write. When a verified backup exists, the installer attempts to restore compatible sample slots after the application returns and checks the restored data. A partial restore is **not** complete: keep the named archive and resolve each skipped slot. If installation continued without backup, there may be no samples to restore.

Browser workspaces on the same origin share an operation lock. Close other MIDI applications too: this firmware does not provide an atomic snapshot-to-OTA ticket against every external client. A last-moment concurrent write from another application remains a limitation.

## Recovery and older firmware

If the keyboard is already in its update loader, the old application's sample reader may be unavailable. **No new backup is possible from an unreadable application**. Recover only with a correctly matched package, understanding that existing samples may be lost. Keep any earlier backup for restoration after compatible firmware runs again.

For stock firmware, obtain the exact model's package from [M-VAVE](https://www.m-vave.com/download) and use the installer's **local-file chooser**, or follow the manufacturer's M-UPGRADE procedure. The web installer does **not** include a factory firmware package. Stock firmware may have no DnBOX sample slots; keep your archive for use after returning to compatible firmware. A completed firmware transfer does not prove that all samples were restored.

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

A website build, host test or package checksum does not prove hardware startup, restart or recovery.
