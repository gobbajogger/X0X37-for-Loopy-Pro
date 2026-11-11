# Chicago SOUND layout

## Owner-approved change

This source candidate implements the accepted three-row Chicago second screen.
It does not alter the frozen 0.1.0 delivery, firmware catalog, controller, device,
project format, sample format or calibration.

| Row | K5 | K6 | K7 | K8 |
| --- | --- | --- | --- | --- |
| LIVE ARP | ENABLE | DIRECTION | OCTAVES | RATE |
| FX / DAMPER | TYPE | Algorithm edit 1 | Algorithm edit 2 | DAMPER |
| SOUND | ENGINE | PRESET | Display-only STATUS | Display-only STATUS |

DAMPER moves from K6 on the removed SUSTAIN row to K8 on FX / DAMPER.
Its existing parameter, threshold, history and audio behavior do not change.
The first four Chicago rows and Miami's OUTPUT/DAMPER mapping stay unchanged.
Row selection still wraps over the real rows, and screen-bank dots still indicate
screens, not the selected row. The printed control captions retain real indices.

Only the accepted Chicago second screen uses the dark palette and shallow panel
edges. It has three 66-pixel panels at y=24, 91 and 158. Drawing uses the existing
strips, fills, lines and bitmap fonts. There is no new framebuffer, theme engine,
setting, blur, real-time 3D or animation.

## Preset identity and status

SOUND stays the section heading. PRESET shows the requested factory preset name,
not only its index; `Acid Lab 1` appears as `ACID` / `LAB 1`. The formatter uses
measured native-font widths and two lines. All 111 current factory names fit in
full. A future over-wide bounded name receives an explicit `..` suffix rather
than drawing outside the cell. Miami shares this name formatting without a
layout or control remap.

An unnamed user sound displays USER. A retained unknown engine displays UNKNOWN.
During an interrupted preset publication, the incoherent requested identity is
shown as UPDATING. Pending requests show their requested name, while STATUS and
its existing wait reason still distinguish that request from the active sound.
PATCH cancels; the right-half STATUS remains display-only. The parameter bars,
ARP inactive cues and existing undo behavior remain.

## Checks

The host tests execute the production UI and engine. They check physical-default
panel mapping, K8 DAMPER versus K6 FX, both voices, Miami's unchanged OUTPUT,
complete names, requested/USER/unknown/publication states, exact native pixels,
all wait reasons, cancel, inert STATUS knobs, row wrap, non-mutating rendering,
undo/redo, tagged storage and SMC/UI interactions.

```sh
X0X_TEST_JOBS=1 X0X_TESTS='sound-layout chicago-presentation chicago-readout matrix-knob-caption sound-matrix bass-presets-live bass-tagged-storage smc-ui ui-303-entry preset-navigation-noop' sh tests/run_tests.sh
X0X_TEST_SMK_CAPTIONS=0 sh tests/host/matrix_knob_caption_test.sh
SAN_FLAGS='-fsanitize=address,undefined -fno-omit-frame-pointer' sh tests/host/sound_layout_test.sh
```

The source-render scenario check uses the tagged preview host build and
`tests/host/test_sound_matrix_display.py`. Native qualification uses the official
DEV app-only runner for both models, PREVIEW=1, TAGGED=1, NOTE_SOUND=0 and
`--no-controller`. The matching candidate receipts, source/tree identities and
budget results belong in the handoff. Historical build results are not proof of
this changed UI. No host or native check establishes hardware behavior, frame
rate or audio/MIDI latency on a device; those remain unmeasured.
