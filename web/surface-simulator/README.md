# X0X Surface Lab — isometric hardware and actual C host

Open `index.html` for the self-contained offline interactive guide. No install,
network access or hardware connection is required in that mode.

The raised SMK-37 Elite and SMC-Mixer are original, photo-referenced vector
solids, not a skewed flat drawing. The 13-inch iPad Pro frame contains an
explicitly modeled Loopy example, not the owner's saved project. See
`REFERENCES.md`, `geometry-notes.md` and `evidence/loopy/REFERENCES.md`.

## Two deliberately distinct execution modes

**Offline model:** all 146 physical controls can be inspected; source-informed
SMC pickup/mapping, clip routing, SCALE/CHORD settings and simulated host LED
feedback are interactive. Full sound engine/menu/ARP timing are not implemented.
Loopy replies and all transport are in-memory examples.

**Actual C host:** compile the bundled adapter against the project checkout,
serve this directory locally, and press **Connect actual C firmware**. The real
unchanged x0x UI, project, sequencer, DSP and SMC mixer source executes. The
screen is the C framebuffer, not HTML text. Physical middle 27 keys, K1–K8,
function buttons, arrows and SMC MIDI controls route to C. Actual SMC pickup and
LED output is displayed. Unsupported controls are reported, with no silent
fallback to the model. Clip/Loopy fixture controls are disabled in this mode.

This host is not the Elite hardware main loop, JieLi CPU, BLE radio, USB driver
or hardware-timing emulator. SMK clip callbacks are not hosted. C pure clip-key
and APC mapping endpoints exist but are not a claim of complete integration.
DSP runs, but browser audio is not streamed. A successful native gesture does
not prove hardware BLE or stability. See `native/README.md` for precise coverage.

## Run actual C locally

From a complete project checkout containing this directory:

```
sh web/surface-simulator/native/build.sh
python3 web/surface-simulator/native/test_core.py
python3 web/surface-simulator/native/server.py --web web/surface-simulator --port 8765
```

Open `http://127.0.0.1:8765`, then **Connect actual C firmware**.
Requires existing C compiler, Python 3.9+ and Pillow (font generator). No
installer or download is executed. Build outputs stay under `native/build/`.
The source-only delivery does not include the full firmware checkout or compiled
libraries: keep this folder under its project `web/` directory. Standalone HTML
on iPhone/iPad runs the model only, not native C. No public hosting or tunnel.
The server only binds loopback and rejects foreign Host/Origin requests.

## Interactions

- Touch/drag faders; drag or wheel encoders; keyboard arrows adjust focused controls.
- Press/release keys, pads and buttons. Keyboard A W S E D F T G Y H U J K maps
  physical keybed positions 0–12 (only 5–12 fall inside the native host key range).
- Native host advances bounded simulated time, not a real-time clock. Its
  **Advance 100 ms** button and **Save gesture trace** support reproducible tests.
- Host connection events are simulated at the queue boundary; unplugging the
  link clears actual SMC temporary Solo state. Blur/release-all clears holds.
- Offline clip mode: load an example feedback layout, press a lit Elite pad,
  inspect APC events and modeled return LEDs. Turn automatic response off to
  see that firmware pad presses alone cannot create host feedback.

## Rebuild and test

```
node web/surface-simulator/build.mjs
node web/surface-simulator/test-model.cjs
node web/surface-simulator/tests/independent-model.cjs
node web/surface-simulator/tests/static-ui-smoke.cjs
node web/surface-simulator/tests/native-ui.cjs
node web/surface-simulator/tests/anatomy.mjs
node web/surface-simulator/tests/geometry.mjs
```

Geometry: 146 unique controls; 1,704 bounded projected polygons; projection
round trips and full fader travel tested. Native C and HTTP security tests pass.
Native UI dispatch is tested with DOM stubs/fake transport, not a real browser.
The SVG preview PNGs are static geometry renders, **not browser screenshots**.
Browser interaction/layout QA remains pending: local Chromium sandbox launch and
separate browser access to loopback were unavailable; no bypass was attempted.

Firmware inputs match baseline `81459573792a68edd02fa1ee5658796b55597ba0` for
inspected core files. Native build creates a SHA-256 source manifest. Anatomy
methodology pinned at `1482e8469b0a9e0d8cc18ea2740a01423d29ae80`; our own SVG
implementation uses fitProjection, real volumes, four-tone shading and depth
ordering. No remote Anatomy installer or kit scripts were executed.
