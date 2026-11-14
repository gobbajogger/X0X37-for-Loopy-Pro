# Fresh current-firmware design/emulator review

**Result:** The bounded current-firmware exercise passes on both models. The proposed UI is not implemented or interaction-validated by this review. Whole-instrument integration readiness remains **FAIL/open**, because the proposal itself leaves control visibility, visual grammar and result ownership unresolved. No score is assigned.

## Scope and provenance

- Proposal: PR #921, `a8986ac2d364bb59a38e068381e18ca068fe7c8d`, assigned tree `1a79869e1df001561a1a8ac3e4571c8e4c708fde`. Its local HTML matches the assigned SHA-256. Historical embedded .25 captures are not current-firmware results.
- Actual firmware: exact Elite and Pro 0.0.26 BUILD_ONLY packages for `3c41ef4bedf9701f549ce2752b73306e2fab1eb8`. Package sizes and hashes match. The existing authentic source receipt closes 5,412 tracked blobs to tree `fa0c3fe75483534f12839eeb8f5e29ec9cb52c6d`; the five source files used here were rehashed against that receipt.
- Actual execution: Node v24.22.1, existing pinned WASM and AudioWorklet, 360 MHz emulation and JIT enabled. I wrote an isolated harness and explicitly loaded each current package. I did not run the historical `probe.cjs`.
- Fifteen isolated boots/runs, including retained stimulus-calibration attempts, total **183,572 emulated ms**. All emulator commands exited 0; every final state has halted=0, no reported trap/error, JIT failures=0 and no held keys. Full exercise receipts also report CPU1 fault=0. Each run ended with 13 short render quanta, already present at boot. This is not browser/audio latency qualification.
- Maintained policy was read at nightly `83fc6d60289869116f93352cf845a9a7e271cdba`, rather than relying on the older AGENTS policy in the firmware checkout. Existing owners were preserved.
- Requested reviewer configuration was GPT-6.1 Sol / ultra. Model selection is parent-owned; these execution receipts do not independently attest the review model. No service-tier selector/query is exposed here, so **Standard tier is unverified**, not asserted.

## Findings, in priority order

### 1. Open integration gate: Settings hides still-live performance controls

The current actual AUDIO, SYSTEM and Global screens retain four PERFORM cards: TEMPO, SWING, FILTER and PUMP. The proposed Settings cards remove them, while the whole-instrument Home proposal retains those controls and uses a different font, header and persistent power treatment. This is a concrete, visible break in control-context and visual continuity, not evidence that the controls ceased to exist. The HTML acknowledges this tradeoff.

**Required disposition:** Keep the existing integration gate open. The design owner must resolve the visibility/routing contract and common grammar before calling the combined UI ready. Do not silently remove working controls or infer approval of a new routing policy. No CP1/CP2/CP3 edits were made here.

Evidence: `pro-settings-held-02-usb-follows.png`, `pro-settings-held-07-battery.png`, `proposal-settings-0.png`, `proposal-settings-4.png`, `proposal-CMP-HOME-POWER.png`.

### 2. Current firmware symptom: clip mode advertises an unavailable confirmation

On both current packages, the dirty LOAD question survives entry into clip mode. PLAY and physical SCALE then leave its raw LCD hash unchanged, while the LCD still displays red **SCALE YES**. PATCH cancels to the same LOAD row. The proposal's component source correctly masks SCALE and delegates clip PLAY; its standalone LOAD picture is explicitly a normal stopped fixture, not a tested clip variant.

**Required disposition:** Preserve the clip exception and make any integrated clip question's native confirmation hint match its actual input mask. Do not present PLAY as a harmless universal Back. Normal PLAY cancellation was observed on press; source confirms the accompanying normal start/stop command. This review does not infer audible transport from a cancelled dialog alone.

Evidence: both `*-load.json`; `pro-load-05-load-question.png`, `pro-load-08-patch-cancel.png`, `pro-load-16-clip-scale-masked.png`; source `ui.c` 3970–4055 and 4497–4533. Normal GLOBE and `<` also left the question unchanged.

### 3. Current held-note loss is real in this emulator; the proposed badge does not fix it

In an otherwise empty Chicago B / sound-page fixture, F4 generated nonzero PCM while transport was running. Entering Global left physical key 17 held, but the next 2,000 emulated ms produced exactly zero PCM on both packages. Leaving Global without releasing the key remained silent. A release and fresh onset produced nonzero PCM again. The stopped fixture gave the same ownership boundary.

The proposed Settings **KEY RETIRED** badge is therefore a truthful warning for this exercised current boundary. It is not continuity implementation. RUN and the continuing clock indicator do not prove that a held voice survived.

Evidence: `elite-continuity.json`, `pro-continuity.json`, frames 04–10 and `pro-continuity-contact.png`. Running held-note RMS was 0.073149; the stable Global and still-held exit intervals were exactly 0; the fresh onset RMS was 0.070820. These are local emulator-output observations, not sound parity, measured hardware tails or physical acceptance.

### 4. Home and Chicago have working semantics worth preserving

Actual four-hit 909 entry, moving playhead, HOME five-part content lanes, global `#1→2` cue, and subsequent pattern-2 adoption all ran on both models. Four PERFORM values remained visible on Home. Current default identities were Chicago B and Miami A; the proposal's historical two-Chicago fixture must not be substituted for this current default.

Current Chicago simultaneously shows four neighbouring parameter groups. K2 and PARA move the active page when no step is held. With step 5 held, K2 instead controls chance. A PARA page change selected FILTER 1; K5 then changed its held cutoff from 25% to 26%, and release restored the displayed base 25%. Keep this scope distinction and fixed spatial controls. A focused proposed page sacrifices the neighbouring-page overview; that is an explicit tradeoff, not a demonstrated usability gain.

Evidence: both `*-musical2.json`; `elite-musical2-05-pattern2-pending.png`, `elite-musical2-06-pattern2-adopted.png`, `elite-musical2-12-step-cutoff-lock.png`, `elite-musical2-13-lock-released.png`; `elite-navigation2-17-sound-page-next.png` and proposal Chicago/Home PNGs.

### 5. Settings navigation and read-only semantics align in the exercised scope

Holding `>` for 900 ms entered CLOCK SOURCE with AUTO unchanged; a released, fresh `>` changed it to INTERNAL. PATCH backed to the category, full exit/re-entry reset focus, K2 selected rows, K3 changed USB AUDIO from FOLLOWS KNOB to FULL, and BATTERY FULL stayed unchanged under K3 and `>`. The actual FULL report is emulated, not a battery measurement. An 180 ms KNOB BANK tap changed clip mode; do not imply an additional long-hold threshold from the proposal's loose “held transition” wording.

Evidence: both `*-navigation2.json`, both `*-settings-held.json`, and both `*-load.json`. Encoder adapter calls were not one-to-one mechanical-detent proof: the first pulse and a reversal could be lost. I retained the mis-targeted initial navigation and oscillator-lock attempts; their filenames are stimulus labels, not assertions. The qualified Settings and `musical2` receipts establish the claims above.

## Scope disposition

| Scope | Status | Meaning |
|---|---|---|
| Assigned local hashes, scoped source closure, WASM validation | PASS | Exact local inputs verified; no exact emulator upstream rebuild claim. |
| Actual Elite and Pro boot and bounded controls | PASS | New execution, LCD frames and receipts, not historical evidence reuse. |
| Settings navigation/edit/read-only behavior | PASS | Only the exercised rows and gestures. |
| LOAD normal/clip cancellation and mask behavior | PASS | Observed behavior; clip confirmation affordance mismatch remains a finding. |
| HOME/sequence/Chicago/held-step exercise | PASS | Current semantics above, not proposed UI implementation. |
| Single integrated visual/control-context contract | FAIL / open | Settings PERFORM visibility, shared grammar and result ownership remain unresolved. |
| Proposed HTML interaction / proposed firmware UI | NOT_RUN | No browser used; component source and extracted static proposal PNGs inspected only. |
| Real peers, all power states, external clock, SAVE/LOAD persistence lifecycle | NOT_RUN | No pairing, storage acceptance or outcome ownership claim. |
| Native resource/release/hardware acceptance | NOT_RUN | Assignment states packages compile but fail required flash reserve. No release inference. |

## Evidence and limits

`inputs.json`, `source-inputs.json`, `wasm-exports.json`, `harness-mapping-hashes.json`, `run-summary.json`, `COMMANDS.md` and `LAUNCH.md` provide exact inputs and reproduction. Each run JSON records every input, emulated timing, LCD hash and end state. Actual `*.rgb565` and matching 240×240 PNGs are separate from `proposal-*.png`.

I inspected native PNGs with the image viewer. Some immediate snapshots contain partial in-progress LCD updates; settled captures were used for legibility judgments. This is not physical flicker/tearing evidence. Proposed larger Settings names/values are readable in these static pixels, but Settings has less context, and the whole proposal's small scope/help text still needs physical validation. Browser responsiveness, tactile burden, candidate font packing, audio parity, durability, resource qualification and hardware readability remain unproved.

No browser retry, socket/certificate/file-URI bypass, source repair, firmware build, Git mutation, publication, upload, release or device action occurred. Simulated flash was never written back to either source package. No prior scores or targets were used; embedded historical receipts were not treated as current execution proof.
