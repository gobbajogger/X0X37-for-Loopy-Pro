# Native firmware bridge (not WebAssembly)

This runs the repository's existing C host implementation: actual app UI,
project, sequencer and DSP source with its simulated FM-1 HAL. The screenshot
comes from that C UI framebuffer. No JavaScript copy of the app logic is used.

## Run

Requires an installed C compiler, Python 3.9+ and Pillow for font generation.
No package installation is done by the launcher. The server itself uses only
Python's standard library. Run from this folder:

```
sh build.sh /path/to/X0X37-for-Loopy-Pro
python3 test_core.py
python3 server.py --web /path/to/web/surface-simulator --port 8765
```

Then open http://127.0.0.1:8765. Without `--web`, the small native-core diagnostic
page is served. This must be served locally; a static public upload cannot run
this native helper. In particular, standalone HTML on iOS cannot run the native C helper. The server binds only to loopback, rejects non-local Host
headers and cross-origin requests, and has no device or shell-command API.
One process has one simulated device. Restart for a clean device state.

`build.sh` compiles existing firmware sources without changing them. It writes
only this adapter's `build/` directory. Generated assets are rebuilt from the
repository's licensed sources; no proprietary SDK/toolchain is needed.
`build/source-manifest.json` identifies source revision and SHA-256 inputs.
Keep `build/`, binaries and Python cache files out of commits.

## Browser contract

`demo/native-core.js` exports `FirmwareCore`, with serialized command requests.
`GET /api/info` gives control names and limitations. `GET /api/state` gives
native state and LED bitmasks. `GET /api/frame.png` returns actual 240x240 UI.
`POST /api/command` accepts a JSON object with `op` and integer-array `args`:

- `input [kind,id,value]`: kind 0 = button id 0..13, value 0/1;
  kind 1 = FM-1 key 0..26, value 0 release or 1..127 velocity;
  kind 2 = encoder 0..6, -64..64 detents; kind 3 = MASTER 0..4096;
  kind 4 = arrows bitmask 0..3; kind 5 = release all held host controls and SMC Solo targets, dropping pending SMC inputs while preserving connection and preset.
- Button order: FX SEL ENV LFO EDIT GLO HOME SAVE ARP SEQ PLAY REC OCT- OCT+.
- Encoder order: SELECT ALGO PRESET K1 K2 K3 K4.
- `advance [milliseconds]`, 0..1000: run actual engine/UI for simulated time.
  Each input also advances 20 ms. Time is explicit, not hardware real-time.
- `notes [note,scale,root,snap,chord,inversion]`: unchanged `ck_notes` logic;
  note 48..84, scale 0..23, root 0..11, snap 0..2, chord -1..29, inversion 0..3.
- `pad [columns,rows,originColumn,originRow,view,physicalPad]`: unchanged APC
  physical/logical and viewport pad mapping. Invalid/outside note is 255.
- `relative [byte]`: unchanged SMC relative MIDI decoder, byte 0..127.

Browser UI should label firmware-host mode visibly and avoid showing a
JavaScript-generated screen as firmware output. Release controls on pointer
cancel, window blur, mode change and disconnect. Read API errors; never silently
fall back while displaying a firmware-active badge.

## Actual SMC controller path

The adapter includes unchanged `smc_mixer.c`, as the repository's existing
`smc_drum_mutes_test.c` host fixture does. These calls run its real message queue,
button routing, fader pickup, relative encoder handling, target selection and
LED feedback generation against the real app/project/engine state:

- `smc-link [up,user]`: simulated connection edge, preset 0 DAW / 1 User.
- `smc-midi [status,data1,data2]`: enqueue a bounded 3-byte controller message.
- `smc-mmc [command]`: enqueue User-mode MMC command 1..43.
- `state.smc`: actual `up`, `picked`, `seen`, `solo`, `rxDiag`, `values[8]`,
  `leds[128]`, and feedback message count.

Only the transport boundary is simulated. Direct MIDI queue injection is not a
BLE connection, radio emulation or USB host test. Clip callbacks return inactive
as in the existing fixture; SMC clip-mode navigation is not covered. Poll/frame
callbacks run at adapter command boundaries, not the target interrupt schedule.

## Source-authenticated non-clip surface mapping

`firmware/src/app/panel.c` defines the default SMK mapping:
FX/SCALE/CHORD/NOTE REPEAT/PARA/GLOBE/PATCH/BT/ARP/SEQ/SEQ PLAY/SEQ REC/OCT-/OCT+
map respectively to host button IDs 0..13. K1 is MASTER; K2..K8 map to encoders
0..6. `firmware/hal/smk_input.h` defines SMK_KEY_F3=5, so physical keybed indices
5..31 map to host 0..26. Do not map unsupported outer keys onto other notes.
Pads depend on the hardware grid/piano setting; that glue is not hosted.

## Boundaries

This is **not** a full SMK-37 hardware emulator, not a JieLi CPU emulator, and
not proof of an installed firmware image. The existing host app uses a 27-key
FM-1 platform interface. SMK's hardware `main_fm1.c` clip-mode screen/input glue,
BLE controller, USB, flash drivers and device timing are not run. `notes` and
`pad` execute real isolated SMK firmware functions, not the complete SMK loop.

The actual DSP runs while simulated time advances; its output is not streamed
to the browser by this adapter. Do not label browser oscillator tones as firmware
audio. Host overload is held at zero as in deterministic host tests; native CPU
performance does not measure device timing. No Chromium sandbox workarounds.

WASM was not built: clang, wasm-ld and Emscripten were not installed. An actual
browser-WASM port is a separate build target, not an already completed feature.

## Evidence

Fresh native build succeeded against repository `b647530fea05a45096f67bebbc811e724da03076` (firmware baseline `8145957` supplied by the integration owner; this adapter does not build that device image). `test_core.py` passes
transport transitions, framebuffer changes, input validation/release, C scale
snap/chord output, APC mapping and SMC decoding. Additional tests pass production SMC DAW Play/Stop, fader pickup and movement, releaseAll held-Solo cleanup with connection preserved, disconnect held-Solo cleanup, pickup reset/drop, and User MMC transport. Existing `smk_clip_keys_test.c`
and `smk_apc40_map_test.c` were compiled and passed. The unchanged existing `smc_drum_mutes_test.c` also passed: 22 active drum mute/light destinations, 10 inactive positions, 30 knob/fader writes, main mute/FX and ownership priorities. HTTP smoke tests passed
state/input/frame requests and hostile Host/cross-origin rejection. Frame
inspection is separate from a browser interaction test, which was not run.
