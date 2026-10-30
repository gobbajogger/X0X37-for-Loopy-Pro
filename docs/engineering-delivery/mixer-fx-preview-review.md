# Mixer FX page port: visual and semantic review

**Review status:** proposal checked against the PR #688 and integration source. These are illustrative SVG screen mockups, **not captured device output**. The ported implementation has not yet been framebuffer- or hardware-verified.

- [Before screen](mixer-fx-page-before.svg): the integration screen shows the FX page context, existing LPF/RVB/DLY/MIX physical SELECT captions and fader targets, but not the knob-bank identifier or colored fader travel.
- [After screen](mixer-fx-page-after.svg): the same eight physical strips and actions, with a `Knobs CUT+FX` header and color extending from the fader target down its track. This indicates value, not a second physical fader.
- Button semantics stay as integrated: part SELECT holds **LPF**; the three bus SELECT captions are **RVB**, **DLY**, **MIX**; HOME+SELECT shows **CYC**. A main-bus short tap navigates and a hold produces temporary low-pass. The source's current multi-peer release ownership is retained.
- Held ownership follows the peer which opened the temporary screen. The `smc_held()` wrapper uses `smc_hold_any()`, and the existing `smc_ui_edge_alive()` handles the wrapping millisecond deadline.
- Original `ui.c` from PR #688 is not copied wholesale: integration already has its effects controls, readout and newer `surface_panel()`/waveform presentation. Replacing it would lose later fixes. The conflict is resolved in favor of the current integration `ui.c`; the missing changes belong to the included `smc_mixer.c`.
- Feature-list item 41 is **partial**: no undocumented FX button assignments have been added.
- The after mockup presents only the ordinary FX-page state. Invalid note, HOLD, LOCK, click, and page-zero cases use existing source semantics; they are not all illustrated.

Mockup review checks: all eight strip identities present, captions fit three character slots, before/after show the same control positions, current selected strip is visible, colors distinguish destination travel and physical pickup markers, header is clipped to 165 pixels by existing UI code, and no sound-engine or project-format change is proposed.

## Source input pins

Source PR #688: `7b2d1726f3515b319d5edcab34529007d33ab3a8`.
Integration start base: `64ed879fc5358229e5febfafa95a22b4580ad2a1`.
Original owner handoff base: `ea859a8639f7448fb53196f6feb8c8c761b114d9`.
No .github, engine DSP or private-header paths are in this change.

## Validation state

Visual review: static mockups checked for component meanings.
Compiled current framebuffer: NOT_RUN.
Physical screen capture / hardware verification: NOT_RUN.
Full host suite and Elite/Pro firmware builds: pending independent execution and exact input discovery.
