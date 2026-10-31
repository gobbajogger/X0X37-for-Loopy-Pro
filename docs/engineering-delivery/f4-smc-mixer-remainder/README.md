# F4 SMC/mixer remainder

Source candidate only. Tests, native builds, static RAM measurements and new framebuffer captures are **NOT_RUN** because the target `AGENTS.md` forbids builds and tests in Codex. The owner's brief assigns those runs to the Orchestrator's box. The owner's 2026-11-01 instruction removes previews and any visual-approval wait for this change. Finish source delivery; the Orchestrator verifies the exact head on the box. Draft status reflects pending box qualification only.

The later [row 41 held-FX arrow rule](FX_HELD_ARROW.md) supersedes HOME+button remapping and the main SELECT activation delay. It records the existing arrow conflicts, held-effect top-bar glyph, changed test expectations, additional capture pairs and the written reason for a 4 B per-peer original-page ownership mask. It also supersedes earlier claims of no mutable-state increase for this follow-up; actual Elite/Pro RAM remains NOT_RUN.

The [owner glyph-rule follow-up](UI_GLYPHS.md) uses pre-change pin `97282d7fed8859f337d606d4880cb095b1ce2591` for its additional before/after scenes. It replaces action/navigation prose with compact pictures while keeping exact values and names. Its captures and runtime qualification are also pending; no historical image is presented as new-head evidence.

## Stack and overlap checked first

Pinned base: #789 `02c5224d12c7a96c0061fea71f13de04de58741b`.
#788 is open at `5b5974b667eaaacb904ef69391877f6779021543`; #789 is also open and explicitly excludes #788. GitHub comparison reports divergence at `bb1907494b8d1b52d52ac220dfeb9a0663ea3ee2`. Therefore #788 is not inherited by #789.

#788 covers clip-stage navigation, encoder-bank handling, locked-kit keyboard focus, fader unmute/pickup feedback and clock visuals. #789 covers the independent existing SMC popup, explicit peer context/cache, lock guards, unlock page retention, modal answers, Help/HOME controls, captured-bank routing and readable strip labels. This candidate ports only uncovered F4 changes and preserves #789's keyboard independence.

| Row | Verified source on #789 | This candidate |
| --- | --- | --- |
| 20 | `smc_beat_lit` already reads the accepted sequencer sample clock; `seq.metro_view` is the audio-owned observer. Headers still read playback/track-step state. | Port #788's two header paths, pickup marker blink and Pause LED phase. Preserve stopped-clock fallback. |
| 30 | `smc_ui_grid` already draws eight physical strips, value labels, active/selected distinction and drum banks; #789 supplies the label/contrast fixes. | Retain layout. Only the ported pickup blink and navigation captions change pixels. |
| 31 | Selection highlight is independent of touched-control focus; SELECT dispatch deliberately chooses a part/mode. | Retain; regress explicit selection and second-press routing. |
| 32 | Captured fader/knob identities, physical Pause note 0x5D, plain 900 ms unlock and lock LED path already exist. | Retain; phase feedback follows row 20. No hardware LED identity claim. |
| 36 | Bluetooth settings already show `R_BTSTAT` and joined peer names through `plat_ble_peers`, `peer_join`, clipped marquee and `peer_offset`. These names are in settings, not the popup header. | No port. Existing `ui-peer-names` selector remains required. |
| 37 | R8 HOME/SONG and S8 PART/SEQ/GEN already exist. Ordinary Up/Down already reaches `smc_knob_bank` in clip mode through the final switch. SELECT8 still hardcodes MASTER; clip strip-8 stage routing is absent. | Port #688's SELECT8 MIX cycle and #788's clip strip-8 PREV/NEXT. Do not duplicate the redundant Up/Down early-guard change. Preserve HOME+Up/Down cutoff save/reset and drum/FX precedence. |
| 38 | First SELECT chooses a drum part; a repeat enters that kit's individual mixer. Repeating the selected sound returns to main MIX. | Retain and regress both kits. |
| 41 | Own `V_FX` context, four button rows, independent two-page strip mappings, per-button HOME reassignment, held chosen effects and release ownership already exist in `smc_fx_try`/`smcfx_strip`. | Later owner amendment: held-button + Left/Right replaces HOME+button remapping, main SELECT plays immediately, and the held effect has a top-bar glyph. Defaults, short-tap navigation and release ownership remain; see FX_HELD_ARROW.md. Hardware/save acceptance is not claimed. |
| 44 | `smc_pattern_delta` accumulates from pending cue; returning to the playing pattern clears `seq.pcue`. | Retain and regress forward/backward queue cancellation. |
| 67 | `smc_ui_edge_alive` and peer-local held inputs preserve the popup through held actions. Accepted activity refreshes the 1200 ms interval; release starts idle expiry. | Retain and regress held MUTE and release expiry. No synthetic fader touch. |

## Provenance and dispositions

Native network Git was not used. The connected GitHub API supplied the recursive tree, exact commit metadata and selected file blobs. Local object hashes reproduce base commit `02c5224d12c7a96c0061fea71f13de04de58741b`, root tree `7568ad94ed45ce62ab3e0cb98526923a50b29b91` and every materialized file blob. This is a sparse source checkout: unselected blobs and parent history are not materialized, so no complete-build closure is claimed. The shell archive request failed to connect to its proxy; API reads worked.

The pre-port check used GitHub ancestry/commit patches and exact base-source comparison. A native `git cherry` across the archived histories was unavailable in the sparse checkout; this is not claimed as a cherry-pick. Individual ported hunks are attributed below.

| Commit | Disposition and reason |
| --- | --- |
| #788 `5b5974b667eaaacb904ef69391877f6779021543` | Port clip strip-8 captions/actions, two header play icons, pickup marker blink and Pause LED phase. Omit redundant Up/Down guard. Leave row45 unmute to #788. Omit keyboard-focus restoration because #789 deliberately owns SMC navigation independently. Do not import its legacy tests that assert keyboard navigation; reuse their behavioral cases against the new context. |
| #688 `abe3fb9a7d29acc42bddca6c369051f7d6ca16b6` | Port the repeated SELECT8 MIX cycle and caption from the named implementation; adapt its page query to #789's explicit browsing context so a captured PART/GEN page cannot become a MIX page. Keep captured targets unchanged. No broad snapshot/engine changes. |
| `04650d512eecea911203bd01e68804af312a5b2e`, `7b2d1726f3515b319d5edcab34529007d33ab3a8` | Caption helpers/tests already present; newer four-row FX captions supersede the old SELECT-only assumptions. No duplicate port or forced LPF. |
| #168 `5c3d9f2fbd86030dc2f7e6d6d54a44414465b090`, `65a0588ed636e8db6b35bdea4991e8990f476ff2` | Spatial eight-strip controls, selection/mode actions, Pause capture and drum SEQ names already present. Retain newer #789 routing. |
| `60292a393c400b9be4acfd33630f9259a95fa439` | Plain 900 ms unlock already present in `smc_lock_ready`/`smc_lock_button`; no duplicate state machine. |
| `58381c8acb11575988ea79f52d043e070d7c8b9e` | Chosen held effect already published by the newer per-button FX owner; do not restore forced LPF or old captions. |
| `6a6f28e6f4c16a1416f5c042006dd8d041dbf88c`, `3d30b2d064a52d2c651b62f2a13f04e30bb21299` | Joined clipped peer-name marquee already present. Bluetooth persistence is outside F4; no connection-layer/plat_fm1 changes. |
| PR764 `000bd821cb144e1def0470f76a009790a25ce21f`, `9e237cce`, `d7f156ec` | Inspected API metadata: Chicago inserts, waveform/ADSR/PERF display, and step-snapshot runtime respectively. No uncovered F4 mixer behavior; do not import unrelated source. |

`archive-missed-0.0.23.md` is absent from the pinned recursive tree and the connected GitHub code search returned no match. Its section 5 could not be read here. Named source commits above were available and inspected directly. Historical #738 popup work is already superseded by the explicitly selected #789 base; no old page-swapping popup is restored.

## Regression and visual evidence handoff

New selector: `smc-remainder`, registered in the normal host suite and serialized generation class G. It uses `smc_ui_test.c` with the actual firmware queue, dispatcher, renderer and engine. Cases cover both header paths and clock lifecycle; strip-8 modes and clip stages; independent knob banks; locked PART and MIX routing; drum-bank precedence; existing HOME modifiers and FX hold/release; both kit SELECT transitions; pending cue cancellation; and held-popup expiry. Twelve clip cases cover both kits, MIX/SEQ contexts and all three stage keys, asserting no false action popup or REC/SOLO side effect. Both header paths are also drawn after actual STOP, Start, Continue, external WAIT, accepted tick and timeout, with WAIT checked at both frame blink phases. The initial F4 port changed or removed no existing assertion; the later row 41 amendment documents its owner-directed expectation changes in FX_HELD_ARROW.md.

Tests were written before the production port. RED/GREEN is **NOT_RUN**, as required by AGENTS and the owner's conditional instruction. Static inspection does not establish a pass.

Run on the qualified box from the full candidate checkout:

```sh
X0X_TESTS='smc-remainder smc-ui-popup smc-lock smc-lock-beat smc-select-mode smc-mixer-page smc-mode-page smc-fx-two-page-strip smc-fx-preset-preserve ui-peer-names' sh tests/run_tests.sh
sh tests/run_tests.sh
X0X_DEV_PADCAL=0 MODEL=elite ./build.sh --board smk37
X0X_DEV_PADCAL=0 MODEL=pro ./build.sh --board smk37
```

Use SDK V1.2.1 and capture separate Elite/Pro logs and link maps before the second build overwrites outputs. Baseline reference requested by the owner: 2352 B free. #789 historically reports 2432 B free for both models; #788 reports a different development configuration at 1920 B and must not be substituted. Candidate free static RAM: **Elite NOT_RUN; Pro NOT_RUN**. The later row 41 amendment adds a 4 B per-peer original-page ownership mask for safe held edits; see its written RAM reason in FX_HELD_ARROW.md. Any other measured decline requires an explanation and review before qualification.

Optional box diagnostics only: capture base and candidate using `smc_remainder_capture.c`, a separate driver that draws the same production scenes without changing test assertions. The base checkout needs copies of the three new remainder fixture files from the candidate. Set an absolute evidence directory; from each checkout run:

```sh
SMC_UI_TEST_FILE=tests/host/smc_remainder_capture.c \
SMC_UI_OUT="$F4_EVIDENCE_ROOT/before" sh tests/host/smc_remainder_test.sh
# On the candidate use SMC_UI_OUT="$F4_EVIDENCE_ROOT/after".
```

Expected real 240x240 PNG paths under `docs/engineering-delivery/f4-smc-mixer-remainder/framebuffer/{before,after}/`:

- `header-beat-lit.png`, `header-beat-dim.png`
- `part-beat-lit.png`, `part-beat-dim.png`
- `header-stop-neutral.png`, `part-stop-neutral.png`
- `header-wait-neutral.png`, `part-wait-neutral.png`
- `header-wait-neutral-blink.png`, `part-wait-neutral-blink.png`
- `pickup-lit.png`, `pickup-dim.png`
- `clip-strip8.png`, `clip-knob-bank.png`
- `select8-mix.png`

These are **optional diagnostic paths, not existing captures**. The owner requests no previews for this change; generating or attaching images is not a source-delivery or approval gate. The approved #688 visual proposal and historical #788/#789 captures are provenance only, not candidate evidence.

## Review and stopping condition

Independent pre-implementation design review completed without running tests. It caught redundant Up/Down code and the locked-context SELECT8 hazard; both are addressed in this scoped port. Compound Engineering review `ce-20261101-f4` completed with four source reviewers and an independent static validator batch. Its two P2 findings received one coupled source/test fix batch: clip-stage navigation is excluded from drum action cards, and both headers have real transport-transition pixel assertions. See [review receipt](review/ce-20261101-f4/review.json), [reviewed source hashes](review/ce-20261101-f4/source-manifest.json) and [follow-up](CODE_REVIEW.md). Execution of the fixes remains NOT_RUN. Source review, box verification and merge readiness remain separate. Historical review receipts retain the earlier approval constraint as recorded; the latest owner instruction supersedes that constraint.

No merge, auto-merge, target install, device write or firmware binary publication is authorized here. The Orchestrator owns full-source execution and box verification. No preview or owner visual-approval wait applies to this change.
