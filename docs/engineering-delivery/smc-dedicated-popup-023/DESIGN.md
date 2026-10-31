# Dedicated SMC pop-up — owner review

Status: PROPOSAL. No firmware implementation or code removal is delivered.

Base: `build/0.0.23`, PR #785, `bb1907494b8d1b52d52ac220dfeb9a0663ea3ee2`.
This replaces the #781 approach. Do not port #788's clip navigation, locked-kit
selection, physical-fader unmute, encoder-bank or beat-feedback changes.

## Outcome and approach

SMC activity opens a temporary mixer-owned pop-up over the current keyboard
screen. Keyboard view, part, mixer selection, page, menu cursor, selected drum
track, clip viewport and control destinations remain independent. Shared
audio/project parameters remain the authoritative musical data.

The owner selected a separate renderer and state. Merely disabling page peek,
as in #781, leaves navigation and targeting coupled. Saving and restoring the
keyboard UI around a draw also violates the no-write rule. Neither alternative
is proposed.

Port existing SMC strip drawing, arcs, value formatting, pickup markers and
button feedback; port existing clip palette/geometry where applicable. Generic
canvas, fonts and parameter helpers may be shared. Never call keyboard menu
painters, `set_view`, `build_pages`, `page_of` or keyboard input dispatch from
the mixer UI. Do not reserve a second framebuffer.

## Five proposed screens

- Faders: eight physical strips, destination values and separate physical
  pickup marks. Amber marks the touched strip; detail text gives pickup direction.
- Knobs: eight encoder values/arcs and the mixer's own encoder-bank caption.
- Mute: eight aligned MUTE/SOLO/REC columns; inactive controls stay inactive.
- Bank: mixer-owned main, 909, 808 and FX contexts select SMC destinations.
- Clip grid: an SMC-owned 8×2 viewport, selection and bank reads existing clip
  feedback through a read-only display model, without borrowing the keyboard page.

The keyboard header stays visible above the pop-up. See `mockups/after-proposal-*.png`.
Control values and clip states shown in these proposals are illustrative fixtures,
not incoming Loopy feedback. Bank/knob captions are presentation proposals, not
an implementation or duplication of #788's behavior changes.

## State and event boundary

Use small mixer-owned state: screen, context/bank, selected strip, peer, clip
viewport and expiry. The demonstration state is 12 B on the host ABI, not a
target RAM measurement or final budget. Keep per-peer state in dual-peer builds.
Do not create a shadow keyboard `ui` or copy its page table.

Resolve SMC targets from its own context and explicit parameter references.
Audit navigation, arrows, bank, lock, buttons, faders and encoders: none may
write `ui.view`, `ui.part`, `ui.mixsel` or any keyboard page, including during
lock capture/release, timeout, disconnect or reset. Keyboard list rows, selected
tracks and clip field editors cannot serve as mixer navigation storage.
Preserve musical mute/solo/record and parameter semantics through model
operations rather than keyboard menu actions.

Activity selects the corresponding screen and refreshes a 1200 ms expiry,
matching the existing peek duration. Holds retain the pop-up through release,
then ordinary expiry applies. Keyboard controls continue to operate their own
UI beneath it. Dismissal redraws the then-current keyboard screen, never restores
an old navigation snapshot. Preserve confirmation/help, project-transfer,
disconnect and reset guards without keyboard navigation writes. Use wrap-safe
time comparisons. The builder may settle helper placement, type names and spacing.

## Proposed physical navigation

Reuse the existing MCU notes and edge/hold decoder; change only their UI
destination, without importing #788's navigation code. Note 5 opens the SMC
Bank chooser; notes `0x60/0x61` move its row and note 5 confirms. Choices are
MAIN, 909, 808, FX and CLIP GRID. Note 6 backs out within the SMC UI. Outside
the chooser, `0x60/0x61` keep their existing encoder-bank operation and show
the Knobs screen; `0x62/0x63` change only the current SMC context's page or
clip viewport. Notes `0x2E/0x2F` change the SMC strip bank; `0x30/0x31` move
its selected strip. SELECT-row notes 24–31 select SMC targets. Fader motion
opens Faders, knob motion opens Knobs, and mute/solo/record actions open Mute.
Clip Grid is opened by its Bank choice and remains an SMC-owned viewport.
Preserve transport and held-FX gestures; navigation cannot consume or replay a
release owed to one of those gestures. These bindings are part of the proposal
for owner review, not already implemented behavior.

## Validation after approval

Strengthen the existing integration snapshots: remove their peek exemption and
assert invariance immediately after every event, during holds, on timeout,
reconnect/reset and intentional keyboard navigation. Cover all five screens,
SMC navigation buttons, drum banks, lock and one-/two-peer builds. Prove keyboard
context changes cannot retarget SMC controls and SMC changes cannot retarget
the keyboard. Exercise the production queue, dispatcher, model and renderer.
Verify live clip feedback through the SMC viewport without keyboard clip changes.

Run SMC and mixer suites, retaining baseline failures separately. Build Elite
and Pro using SDK V1.2.1 and `X0X_DEV_PADCAL=0`. Compare map-derived free RAM
against the same base/configuration; the owner's base reference is 2352 B.
Target no material loss and explain every added byte. Produce final PNGs from
the candidate renderer, including expiry and unchanged keyboard menus.

## Evidence limits and approval gate

Current PNGs come from the real host RGB565 framebuffer, production canvas and
fonts. `before-*` uses the unchanged base renderer. `after-proposal-*` uses a
throwaway renderer with its own state, not production integration. The capture
checks the existing complete navigation snapshot for every screen and exact
keyboard framebuffer equality after dismissal. It does not prove production
input independence, automatic expiry, live clip feedback or target RAM.

Owner mockup approval is required before code removal. Continue implementation
and final validation on this same draft PR after approval; do not open another.
Main, nightly, `.github`, GitHub Actions and devices remain untouched. No tokens,
SDK/toolchain bytes or firmware binaries belong in the PR.
