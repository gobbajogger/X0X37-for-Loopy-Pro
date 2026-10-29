# Offline sequencer candidate

GPL-3.0-only. Dedicated page: `index.html`. Serve this directory from an HTTP development origin with ES modules and workers enabled. No shared website route, installer link, deployment or device adapter is included. This source-only fork of website PR #93 repairs exact-grid MIDI releases and stale file reads, prevents silent multi-tab overwrites, and adds bounded Redo. Only revision-13 firmware project files are supported elsewhere: this editor handles local draft JSON, not device project files or backups.

The page edits a local32-pattern draft: separate Synth1/Synth2,64 stored synth/drum steps,16 BREAK gates/slices, source-backed sound drafts and engine bindings. Drag note bodies/edges with bounded edge auto-pan, use the Start-step inspector as a keyboard/touch alternative, edit the velocity lane, use keyboard actions, switch patterns directly, and export/open local JSON. Legacy velocity0 is preserved on note moves and shown as effective100. Complex tie-cell metadata rejects direct edits without flattening. Native browser panning and custom early pinch cancellation were exercised in Chromium phone emulation; real phone hardware remains untested. Zoom is horizontal; pitch navigation scrolls vertically. Dense-note selection has a separate Notes inspector.

SMF0/1 PPQN import uses a worker,1MiB/256track/100000event limits, explicit track/channel/window selection, replacement/loss inspection and one Undo commit. BREAK import is disabled. Drum hits import without per-hit velocity storage; this loss is explicit. No automatic polyphony reduction, overflow wrapping or cross-pattern splitting.

New/Mutate use the local TB-3PO port.432 exact seeded outputs match an isolated host C oracle using pinned `tb3po.c` at7e4bbfd95da1d9ad86c80541b9bceae48231d006. This validates sampled generator math, not firmware/audio/device parity. New resets all64 stored cells. Mutate can introduce notes into an all-rest line; initially empty lines do not force a fallback audible anchor. Auto-bars is a draft setting; auto-mutate never runs here. No dedicated drum generator or stateful BREAK output is fabricated.

Sound drafts use fixed source descriptor bounds. Original18 Miami/22 Chicago identities use human-facing labels; runtime extensions remain reachable separately. Miami missing original selector modes are disabled. Chicago amp/filter ADSRs remain independent, with no unified Env write. Pitch Bend software edits remain disabled. Either synth can select Miami/Chicago, subject to one Miami context. Shared FX/master controls have explicitly global keys.

These files are not device backups. Probability/condition, motion/p-lock, per-note sound, song/chain and live routing/clock/transport are not represented or implemented. No live triggering, playback, MIDI-port request, device read/transfer/update or sample-file operation exists. Unset values remain visibly unset; no real engine state is inferred from a draft.

Validation:

- `node --test web/offline-sequencer/*.test.mjs` from repository root runs all three editor Node suites (model, review, safety).
- `node web/test_web.mjs` runs the existing host Web MIDI checks; any missing build-dependent package is reported as skipped.
- `.github/workflows/dnbox-editor-host.yml` requests both checks on PRs targeting `nightly`. A workflow definition or unsuccessful startup is not a PASS.
- `python3 web/offline-sequencer/browser.test.py` and `render_actual.py` use system Chromium/Playwright and an intercepted in-process HTTP fixture origin serving exact owned source bytes; no file navigation or public server.
- **No shared bundler/website build is defined for this isolated firmware-repo page.** The shared website is maintained in a separate repository, and this source branch does not integrate or publish it. Earlier `npm run build` / `npm test` evidence applies only to the upstream website PR input.

Keyboard: arrows move the grid cursor; Enter inserts; Shift+left/right changes selected-note length; Alt+up/down changes velocity; Delete removes; Escape cancels; Ctrl/Cmd+Z undoes; Ctrl+Y or Ctrl/Cmd+Shift+Z redoes. Space does not start any transport.

Provenance: website draft PR #93, source `7b962858d802588ed1733c07572429e91bb5639f` (separate site repository). This candidate is committed and pushed to the isolated firmware repository branch `codex/web-editor` targeting `nightly`. It has not been merged, built into the device firmware, routed to the public site, or device-tested.

Recovered implementation review R1 corrections (2026-10-29): melodic MIDI import rejects a destination TIE crossing the replacement-window boundary before source mapping. It preserves the entire draft and offers no usable replacement candidate; choose a window covering the complete destination note. Direction/Transpose are visible and enabled only for Synth1/Synth2, and unsupported synthetic change events on909/808/BREAKS are ignored without changing the draft or Undo history. The complex-tie edit guard remains unchanged. TouchOSC continuous-layout review is resolved at design level; native Lua/runtime remains unrun.

Affected checks: `node --test web/offline-sequencer/review-regression.test.mjs`, `node --test --test-name-pattern="SMF|sourcepolyphony|GM drum" web/offline-sequencer/model.test.mjs`, and `python3 web/offline-sequencer/review-browser.test.py`. Upstream browser test receipts belong to the separate website PR and are not re-attributed to this branch. When CI has no executed steps or logs, host Node tests are unverified until a real runner executes them.
