# DnBOX public browser trial (EM1)

This is a **source checkpoint**, not a deployed/qualified demo. The DnBOX player is a
dedicated `/dnbox/` page in the existing [fm1-emu](https://github.com/joakimeriksson/fm1-emu)
MIT-licensed WebAssembly runtime. It uses the real `fm1_web.wasm`, the shared
AudioWorklet and Rust SMK-37 front-panel renderer. The JavaScript control surface
sends hardware matrix/analog messages to that emulator. It does **not** synthesize
DnBOX audio in JavaScript, display a prerecorded video or flash a keyboard.

## Source and ownership

- Public-emulator branch: `emulator/dnbox-public-try-20261113`, based on
  `emu/fm1-emu-360mhz` at `6b60618c8bf344d65a166905fb746f46ab708fcb`.
- Website redesign PR #890 and `website-ux/v2-preview` have an independent
  writer. This branch does not touch that preview or the live installer.
- Native DMA/audio-dropout qualification is owned by the existing firmware
  thread and PR #892. Browser timing is neither an acceptance replacement nor
  evidence of physical SMK-37 performance.
- ROADMAP row EM1 in #887 is the **only** product tracker. This document is
  runtime/integration evidence, not another request list.
- Upstream fm1-emu is MIT, copyright © 2026 Joakim Eriksson. Preserve
  `LICENSE`, upstream links, and compatibility symbols. DnBOX firmware
  remains GPL-3.0, with third-party rights preserved.

## Exact firmware identity

Only published **DnBOX 0.0.25.1**, source
`79280e763341d03ba43266d3e25dad66f12de860`, is in the catalog:

| Model | Firmware image SHA-256 | Size |
|---|---|---|
| SMK-37 Elite_900 | `5e24e0d4afccc30906b334fbe73bcad3621df8df3c325790b7af2f94accffb1a` | 651721 B |
| SMK-37 Pro_900 | `6f135a3d03bb1a2a4d09d63040c719313612b85590a6af69b293a928e22285bf` | 651721 B |

These values and artifact URLs come from the existing `gh-pages/versions.json`
published entry `nightly-79280e76-dev` and GitHub release `v0.0.25.1`.
The page fetches published firmware **on Start**, not from this source tree,
and verifies exact model-specific bytes including IndexedDB-cached copies.
No 0.0.26 build is advertised or silently substituted. The runtime can boot
the two different boards only if the upstream emulator supports their images.
A catalog/hash match is not a successful boot result.

## Build and run (not executed in this checkpoint)

The original runtime requires the Rust `wasm32-unknown-unknown` target,
the upstream crate and matching build toolchain, not the JieLi firmware SDK.
```bash
node --test web/dnbox/*.test.mjs
web/build.sh web/dist
(cd web/dist && python3 -m http.server 8642)
# In desktop Chromium: http://localhost:8642/dnbox/
```
Do not commit `web/dist`, the `.fwsc` files or `fm1_web.wasm` into the
repository. The build script copies the built WASM, worklet and supporting
JS into `web/dist/dnbox` and retains the original FM-1 and SMK-37 pages.
An existing, identified MIT build of `fm1_web.wasm` can be reused locally,
but only pinning its build provenance proves which version was tested.

## Audio and interaction boundaries

The initial true-device clock is **360 MHz**, with `AudioContext` at 44100 Hz,
interactive latency hint and an emulated `fm1_ahead` queue of **512 frames**
(11.61 ms of *queue allowance*, not measured key-to-speaker latency). Users
may compare `?ahead=256` and `?ahead=1024`. Do **not** hide a slow CPU
behind a 2.5-second buffer. The existing worklet exposes short audio quanta,
runtime load and speed. The DnBOX page also samples occasional key input
messages to measure **main-thread → worklet → main-thread turnaround**:
median/p95 is displayed separately from real audible latency. The probe
runs on key input only, never in the per-sample render loop. It still needs
browser measurement and must not be called an audio latency result.

The real controls cover all 37 notes C3–C6, 16 pads, 4 faders, pitch/mod
wheels, 8 encoder pairs and SEQ PLAY. Per-pointer hold/release and
page-blur/cancel cleanups prevent controls getting stuck in the adapter;
the firmware remains responsible for its own state. Boot/reset disables
controls until the worklet reports the correct SMK-37 board. User gesture
starts the Web Audio context; a failure/unsupported browser retains a
visible status. The page has no device-flashing routes.

Initial **target**: current desktop Chromium/Edge via HTTPS or localhost.
Firefox, Safari, mobile and physical-hardware performance are **NOT_RUN**
and must not be advertised as supported merely because feature detection
passes. Web MIDI is optional.

## Executed evidence vs remaining gates

**EXECUTED** in a source-pinned V8 harness on this branch: 77 / 77 focused
checks, 0 fails. They covered pinned positive/negative identity, URL and
model tampering, all 37 key note numbers, bad inputs, chord release, short
audio queue bounds, missing AudioWorklet, required DOM IDs and build/runtime
asset inventory. In a second run, the **exact three repository .test.mjs files**
executed **14 / 14 test bodies PASS** with compatible Node assertion/fs stubs;
the changed JavaScript source and tests parsed 8 / 8. The connector V8
runtime substitutes URL/SearchParams because browser globals are unavailable.
These are **not** native Node, Rust, browser boot or acoustic audio tests.

**EXECUTED (QA continuation 2026-11-14)**: 41/41 PASS, 0 FAIL, 0 SKIP,
Node.js **v24.22.1** on authorized DnBOX cloud, exit 0. Command:
```bash
node --test web/dnbox/controls.test.mjs web/dnbox/controls.dom.test.mjs \
  web/dnbox/hold-regression.test.mjs web/dnbox/integration.test.mjs \
  web/dnbox/identity.test.mjs web/main.test.mjs
```
The isolated test checkout was assembled from exact GitHub source blobs,
each checked against its Git blob SHA before execution, plus a temporary local
`web/package.json` with `{"type":"module"}` (not committed). The native
Node tests include the existing 14 `web/main.test.mjs` page lifecycle
cases. Scope: held-owner source event routing and accessible DOM listener
shim, package identity, wiring and existing browser page unit behaviors.
The first run failed due only to a test harness missing `await` and one
omitted `build.sh` input; those harness/setup errors were corrected and
**the actual rerun passed 41/41**.

**Fix for [QA #899 comment 6867997420](https://github.com/gobbajogger/X0X37-for-Loopy-Pro/pull/899#issuecomment-6867997420):**
`web/main.js` now uses one counted owner registry for canvas pointer IDs,
computer keyboard physical-key IDs, accessible key/pad/button aggregates and
per-device/channel MIDI notes, so neither release order steals a shared hold.
It clears owners on page blur, tab hide and firmware boot/reset. The
`web/dnbox/controls.js` held router now counts unique pointer/keyboard
tokens per physical key/pad/button; `aria-pressed` stays true until the last
pointer lifts. Tests execute the extracted **production handlers** and
mounted accessible-button events, covering both input orders, two pointers,
glissando, repeat, cancellation, button blur, page blur, reboot and pad
sharing. Upstream FM-1/SMK-37 behavior is unchanged by the DnBOX-only
routing branch.

**NOT_RUN**: compiling the exact `fm1_web.wasm`; actual Elite and Pro
browser boot, sound amplitude, audio short-quanta/dropouts, key-to-output
acoustic latency, Chrome interaction screenshots, heavy scenes, browser
compatibility and hardware measurements. The source repository has no
compiled WASM; the allowed Library search did not expose a named
`fm1_web.wasm` artifact. No firmware was rebuilt, no hardware was flashed,
and there is no main/nightly merge or deployment. Earlier reported Pro
~0.9× realtime/audio breakup and Elite ~1× remain unverified and must
not be converted into PASS.

## Browser qualification before integration

Run both **exact** packages on a pinned browser/CPU and WASM SHA. Record
emulator boot `fm1_board() === 1`, LCD changes, sound RMS/PCM capture after
real note/pad actions, both model identifiers, sustained note release,
sequencer/play/reset/flash persistence, first-sound time, input turnaround
p50/p95, `AudioContext.baseLatency`/`outputLatency` where exposed,
quantum duration vs `128/44100 = 2.90 ms`, audio `short` counters,
realtime drift and memory. Repeat with a heavy Chicago/Juno/BREAK/909
scene only once it can be reproduced with audible BREAK in **that** image;
do not infer native hardware acceptance. No mobile/Safari PASS without
own observations. If Pro stays ~0.9×, report the breakup openly and
profile the actual worklet/JIT without changing sound.

## Website integration contract (for #890, not a takeover)

After exact browser validation, the website owner can link a clearly
labeled `Try DnBOX` route or a reviewed embedded player hosted at
`/dnbox/`. Keep `web/dnbox/{index.html,entry.js,identity.mjs,controls.js,
firmwares.json,style.css}` beside the upstream worklet, WASM and
dependencies. The website PR must not duplicate the emulator logic.
A tested preview is not a license to deploy or change the installer.
