# DnBOX 0.0.26: Drone synth slot engine (ROADMAP 26.1–26.2)

Base: `lane/025-zero-dropout` @ `78f81bad07b8e53161326c056a2111c663349772`; 0.0.25.1 firmware `79280e763`.

## Scope and purpose

Add a lightweight, smooth Drone voice inspired by the MIT-licensed Mutable Instruments Plaits String Machine as a **third selectable engine for an existing Chicago/Miami synth slot**, not a sixth part. Drone uses that slot's level, mute, FX, MIDI routing, sequencing, and transport lifecycle. Preserve project revision 13 and current song-file layout. This lets performers create a sustained, spacious timbre without adding a part or changing saved songs.

## Execution

1. Inspect current engine selection, audio allocation, lifecycle, UI, serialization and tests. Locate/review the original Plaits String Machine implementation and its MIT license before porting. Establish CPU/RAM baseline for 25.1.
2. Add a bounded, fixed-allocation Drone DSP implementation and integrate it into the existing synth-slot dispatch with conservative polyphony, note-off/voice-steal, mute/STOP/PANIC behavior and no shared per-block stalls. Add Drone to engine selector using existing UI controls and validate actual display.
3. Add behavioral and DSP tests: existing songs unchanged, slot/FX ownership, notes/releases, engine switching, clipping/noise, lifecycle, CPU and no heap on the audio path.
4. Run the complete host suite (0 fail, 0 required skip), native Elite and Pro builds (pinned SDK V1.2.14), pool headroom >= 8,192 B, and `emu360 fm1-live` (worst block < 11,609 us, zero late). Compare heavy-load cost and audio-path latency directly to the pinned 25.1 baseline; no regression permitted. No unverified result counts as a pass.

## Restrictions

This PR contains only 26.1–26.2. Sequencer chord recording (26.3), Juno/chorus (26.4–26.5) and other engines are separate work. No change to `main`, nightly, `.github/`, bootloader/protected sectors, `bass303/wobble.c`, saved-song layout or firmware binaries. Do not commit proprietary SDK/toolchains, secrets or raw captures. Keep the PR draft until its exact-head gates pass.
