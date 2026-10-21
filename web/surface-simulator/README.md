# x0x Surface Lab

Open **index.html** in a desktop or mobile browser. It is a self-contained, offline interactive field guide. No server, account, dependencies, network calls, hardware permissions or firmware installation are required. It never connects to MIDI/Bluetooth/USB devices.

## What is implemented

- Photo-referenced SMK-37 Elite and SMC-Mixer surfaces: 146 individually addressable controls. Original vector artwork, not embedded manufacturer photos. Fit/2× zoom and a large selected-control editor supplement the detailed surface on phones.
- Mouse/touch press/release; drag faders, drag/wheel encoders, focused control arrow keys. The computer keyboard A W S E D F T G Y H U J K plays the first octave; Space operates play.
- Source-informed SMC main mix, 909/808 MIX and SEQ, 11-track banks, ±2/crossing fader pickup, pickup reset after external edits, held Solo ownership across page/bank changes, User MMC latch fallback, mute/record/select, transport and navigation. Source-led button feedback is shown on the mixer.
- Elite fader banks and pickup, clip-mode toggle, physical pad fold, stored-note pad releases, source palette/idle lifting, explicit simulated host response, host-owned clip colors, example 8×4 and 4×4 layouts, local viewport, transport press/release.
- SCALE/CHORD/ARP settings pages, source scale/chord calculations and note lifecycle examples. Source-backed SCALE/CHORD calculation passed 10,000 comparisons against the unchanged firmware C header.
- Signal monitor distinguishes direct SMC input, local x0x actions, USB/APC output and illustrative Loopy feedback. The depicted candidate uses one direct SMC BLE link and USB Loopy, not simultaneous Loopy BLE or three physical SMC links.

## Try it

1. Move SMC F1 away from its target. It says PICK and leaves the destination unchanged. Reach/cross the target; it takes over. Turn K1: pickup resets.
2. Choose 909 MIX. SELECT a drum; pads edit its steps. Hold Solo while changing bank, then release. The original target clears. Bank 9–11 leaves strips4–8 inactive.
3. Press KNOB BANK, then Load8×4 feedback. Pad presses emit APC events. Disable “Simulate host response”: the firmware-side press no longer changes returned clip state. OCT controls switch local pad rows; no APC navigation commands are sent.
4. In clip mode press SCALE or CHORD. Use the visible ON field, an SMC encoder, or the selected-control +/- buttons; play keys and inspect the processed notes. SMC encoders do nothing on the bare clip grid rather than changing hidden x0x fields.
5. Change SMC preset to USER. Solo is now a press-toggle; the stock MMC preset has no release message. The monitor shows the six-byte envelope.

## Accuracy and limits

Firmware reference: `81459573792a68edd02fa1ee5658796b55597ba0`, with `smc_mixer.c`, `smc_protocol.h`, `smk_apc40.h`, `smk_clip_keys.h`, `smk_clip_colour.h` and committed palette tables as the implementation authority. `BEHAVIOR-MATRIX.md` gives file/line references. Older APC implementation notes and SMK docs contain superseded behavior; this guide follows checked source for its modeled cases.

This is **not compiled firmware, an audio engine, a hardware emulator, an installer, or proof that BLE works**. It does not model hardware security/GATT, queues/IRQ timing, latency, stability, stored samples, audio synthesis, all sound/menu/song actions, full APC footprint inference/flash-detection, ARP scheduling, strum timing, swing or humanization. ARP settings are inspectable but played keys show a chord/note preview rather than timed arp output. General sound-field edits are normalized illustrations, not complete parameter emulation. Demo note velocity is100, not a measured pad velocity. Local x0x key behavior is indicated rather than a complete key/UI command interpreter. LED bytes are source-derived, but display pixels cannot predict physical LED color.

The user's **actual current Loopy project/layout was not found**. The Loopy panel is clearly marked provisional and is not an asserted 1:1 replica. Fixture layout mapping covers origin0, 8×4 and4×4 only. The host response checkbox is browser simulation, not firmware-side local clip-state prediction.

Physical counts, layout, labels and approximate proportions are based on inspected official photographs/manual diagrams. See `REFERENCES.md`. The drawings are not measured engineering CAD. The detailed device remains photo-referenced; LCD content is an enlarged teaching abstraction, not a pixel-identical firmware screen.

The requested Anatomy skill was read at commit `1482e8469b0a9e0d8cc18ea2740a01423d29ae80` (SKILL, craft/motion/verify references and desk-computer example). Its truth-first model, purposeful physical parts, coherent geometry, accessible interaction and close-up checks informed this implementation. Its invented-object/isometric aesthetic was deliberately subordinated to the owner's request for actual surface anatomy. No Anatomy installer, supplied kit or third-party script was executed. All application/build/test code here is locally authored.

## Validation

- 27/27 local model checks passed (`evidence/model-tests.json`).
- 10/10 independent source-focused regressions passed (`evidence/independent-model.json`).
- 10,000/10,000 scale/chord calculations matched compiled unmodified source (`evidence/key-differential.json`).
- Full app execution with a minimal DOM stub and 6 specific UI-flow checks passed (`evidence/static-ui.txt`). This is **not** a browser test.
- Anatomy count/identity checks passed: 146 unique controls,37 keys,16 pads,8+8 encoders,4+8 faders,43 assignable SMC buttons plus BT/SHIFT (`evidence/anatomy.txt`).
- SVG surface renders and a2× detailed Elite view were visually inspected.

**Real browser interaction, responsive layout and accessibility verification remain pending.** Local Chromium could not create its process socket in the execution sandbox. The supported escalation failed during environment setup with a read-only denied-path error; that path was not accessed or changed. The cloud browser disallowed file URLs; an authorized localhost-only server was unreachable from that browser. No restriction was bypassed. `tests/browser.cjs` is supplied for the next permitted browser environment, not claimed as run. The included surface PNG is a vector render, **not** a browser screenshot.

## Build / tests

Uses Node built-ins only, plus installed C compiler for the optional source oracle. From this folder:

```sh
node build.mjs
node test-model.cjs
node tests/independent-model.cjs
node tests/static-ui-smoke.cjs
node tests/anatomy.mjs
```

Within the full repository, run the source oracle from repository root:

```sh
cc -Wno-attributes -I . web/surface-simulator/tests/key-oracle.c -o /tmp/x0x-key-oracle
node web/surface-simulator/tests/key-differential.cjs /tmp/x0x-key-oracle
```

`extract-data.py` reads only the committed firmware color/note tables and regenerates `data.js`. The standalone archive includes generated tables; no SDK/toolchain, raw capture or firmware binary is included. Run `node render-preview.mjs OUTPUT_DIR` to render SVG evidence. Browser tests require an existing trusted Playwright/Chromium installation; no automatic install occurs.

## License and scope

Simulator source: GPL-3.0-only, aligned with derived x0x source behavior/tables. Manufacturer names identify referenced hardware; no endorsement is implied. Original photos/manuals are provenance links, not bundled assets. No firmware or install-artifact file was changed.
