# F6b source handoff to the Orchestrator

Source base: `build/0.0.23@bb1907494b8d1b52d52ac220dfeb9a0663ea3ee2`.
Source branch: `chat/f6b-web-editor-20261101`. The PR body records its exact head.
The owner assigned **code and test authoring only** to this Codex session.
No test, browser, layout generation, firmware build or RAM measurement ran here.

Compound Engineering source review completed with eight lenses and independent
source validation. Review findings led to snapshot-bound project provenance in
both UI/controller, workflow-token invalidation, Group keyboard isolation and
corrections to two test expectations/locators. Added regressions remain unrun.
The source review is not a substitute for the box's required execution.

One lower-priority review concern remains: catalog requests have byte limits and
explicit cancellation, but no completion deadline; a stalled network request
stays pending until cancellation or a new selection. Existing draft data remains
untouched. This does not enable any live device transport.

## Delivered source

- Atomic melodic note groups: rectangle selection and accessible checklist,
  complete-cell clipboard, cut/delete/paste, move/transpose, velocity Set and
  revisioned preview with one Undo. Hidden cells, release endpoints, TIE metadata
  and legacy velocity survive. Collisions and bounds errors reject the candidate.
- Root/scale and time snap for **new** notes, labelled read-only other-synth ghost,
  local touch keyboard, mobile Editor/Sound view switch and central cancellation.
- Raw revision-13 project-file projection retaining opaque original bytes for export.
  Portable .x0xproject bundles are not supported by this editor file action.
  Unsupported representations reject instead of silently losing data.
- Bounded community-draft catalog browser, metadata/rights display, same-origin
  payloads, SHA-256 verification and explicit stale-safe draft replacement.
- TouchOSC performance/feedback Lua source and deterministic layout packer.

PR #685 is already merged. All 22 editor paths match its head in the base; no
commit was re-ported or dropped. No F6a or B1 source, firmware, installer connection
sequence, `.github/`, protected device region or binary is in this change.
The feature registry and host suite receive only the scoped F6b additions.

## Incomplete acceptance

This is a source delivery, **not full acceptance of rows 90–105 and 119**.

- **104:** active PROJECT transport has no client owner/CAS token or atomic
  export-to-import transition. A new get/compare/write sequence would race.
  Both `DeviceSync.read()` and `.write()` fail before calling the device.
  The firmware/protocol owner must provide the capability before live sync can
  be enabled. See [SYNC.md](SYNC.md), including the unsafe blind-ABORT race.
- **103:** local sound descriptors remain offline. The project adapter refuses
  sound/engine reassignment and runtime-RNG export rather than guessing encodings.
- **105:** source packer and Lua are provided; `.tosc` generation and actual
  TouchOSC validation remain for the box/runtime. It is a MIDI performance
  companion, not device pattern synchronization.
- **119:** the catalog is empty and supports current offline drafts. There are
  no invented community songs or full device song-chain imports. Full song-format
  support needs the F6a contract; voting/contribution audit are outside this brief.
- **97/102:** actual iPhone Safari/VoiceOver acceptance remains pending.
- Actual browser before/after PNGs are pending. No drawings or resized browser
  images are presented as 240×240 firmware framebuffer captures. No firmware UI
  source changed; the brief's framebuffer requirement must remain explicitly
  unfulfilled unless applicable actual captures are supplied.

## Box validation

Use a complete checkout of the exact PR head, including its unchanged web data
modules, catalogs and generator fixture. The local sparse authoring checkout is
not a complete build input. Record exact source, V1.2.1 SDK and toolchain pins.
Install the box's required Node, Python Playwright/Chromium and Lua 5.1 using its
normal dependency setup. New host entries intentionally do not hide missing tools.
The host runner isolates HOME per test: set `DNBOX_CHROMIUM_EXECUTABLE` or
`PLAYWRIGHT_BROWSERS_PATH` to the box's shared installed browser location.

Run the full registered host suite:

```sh
sh tests/run_tests.sh
```

New selectors (already registered in that script): `offline-sequencer`,
`offline-sequencer-browser`, `offline-touchosc-pack`, `offline-touchosc-lua`.
The Node selector includes every `web/offline-sequencer/*.test.mjs` suite.
The browser selector runs `f6b-browser.test.py` at desktop, portrait and landscape
sizes and writes screenshots/receipts to `DNBOX_EDITOR_TEST_OUTPUT_DIR`.

Also run the existing editor browser regressions against the exact head:

```sh
python3 web/offline-sequencer/browser.test.py
python3 web/offline-sequencer/review-browser.test.py
python3 web/offline-sequencer/render_actual.py
```

Capture the **before** browser from the pinned base in a separate clean checkout
and the **after** from this candidate. Label each with its source SHA. Run actual
Safari/iPhone interaction and accessibility checks separately from emulation.

Build only the requested SMK-37 targets with pinned SDK V1.2.1:

```sh
X0X_DEV_PADCAL=0 MODEL=elite ./build.sh --board smk37
X0X_DEV_PADCAL=0 MODEL=pro ./build.sh --board smk37
```

Record each exit and static free RAM from its actual linker/build receipt.
Elite/Pro RAM here is **NOT_MEASURED / NOT_MEASURED**. The brief's 2352 B baseline
is a reference, not a newly measured result. No firmware inputs change in this PR.
Do not claim a current-head native PASS from a historical artifact.

Generate the source-only TouchOSC project on the box, then open it in the actual
supported TouchOSC application and verify the routing, release and feedback:

```sh
mkdir -p build/touchosc
python3 web/offline-sequencer/touchosc/pack.py --output build/touchosc/DnBOX-companion.tosc
```

The generator refuses an existing output. Do not commit the generated artifact.
Follow [touchosc/README.md](touchosc/README.md); no hardware access follows from
this handoff. Keep the PR open pending box results and owner pre-merge approval.
