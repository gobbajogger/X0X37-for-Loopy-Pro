# Independent A4 / J37 rendered interaction review — R1

Reviewed frozen HTML SHA256 `952ab41c61e7471994f3f3b34b93d2a2b68dbbbed1dcac18eb89fa188db9a730` at `http://127.0.0.1:8764/index.html`, on 2026-11-14. Reviewer was read-only on the design lane. All new evidence lives in `/workspace/a4-review-r1`.

Verdict: the navigation model is useful and mostly source-faithful, but revise the native information treatment before accepting it. Two medium findings remove information the player currently has on the device. No critical or high issue was observed in the bounded interactions exercised. These ratings are independent heuristic judgments, not musician-study results or runtime approval.

## Findings, ranked

### M1 — Read-only marking hides the actual STATUS and BATTERY values

Reproduce: Restart → GLOBE → K2+ → > → K2+ three times. Native STATUS is `READ`, although the fixture is READY. From category focus select SYSTEM → > → K2+ twice. BATTERY is `READ`, although the fixture is USB POWER. The surrounding HTML detail reveals the values; the 240×240 screen never does.

Evidence: `status-native.png`, `battery-native.png`; `results.json` records Bluetooth status and battery state. Source `firmware/src/app/ui.c:514` makes these read-only; `:689–695` supplies the values; `:6623–6641` renders `pref_value` for them. Read-only is an interaction property, not a substitute for the information. A performer cannot diagnose connection or power from this design without the browser inspector.

Action: keep READY/USB POWER in the value column, use the existing focused footer to say Read only, and verify all status/battery labels fit at native size. No firmware behavior change is needed.

### M2 — The proposed peer clipping permanently removes useful identity on the native screen

Reproduce: click Long peer name task checkpoint. The focused NAME row permanently shows `SMC-Mixer…`. Neither waiting, K3, nor SCALE provides a native route to `Stage-Left, Keys-Right`. The complete identity exists only in the web detail below the LCD. The renderer hardcodes this abbreviation, independently of the actual peer text.

Evidence: `focused-peer.png`; `extra-results.json`, long focused peer. Source `ui.c:6605–6611` clips within the row and scrolls a long peer string. A static treatment may be calmer, but the proposal currently loses the stage-side/second-peer distinction that can answer a concrete musical routing question.

Action: show the full identity through a bounded native focus detail, a labelled detail page, or a source-faithful marquee. Keep a clip mask; demonstrate two peers sharing the same prefix remain distinguishable using only the LCD and physical controls.

### L1 — Keyboard arrow holds do not exercise repeat

Reproduce: click Held entry; keyboard-focus >; hold Enter 900 ms; release; hold Enter another 900 ms. First hold safely enters; second hold changes AUTO to INTERNAL once and never reaches EXTERNAL. The same 900 ms mouse hold reaches EXTERNAL and repeats at the boundary. `S.ms` remains zero for keyboard holds.

Evidence: final two records in `results.json`; keyboard handlers do not start the tick used by pointer holds. This is a walkthrough input inconsistency, not a firmware arrow finding.

Action: run the same hold timer for keyboard activation and stop it on keyup/blur, or explicitly constrain held-repeat assessment to pointer input in the UI. Prefer consistent keyboard support.

### L2 — Linked emulator evidence is missing from this frozen deliverable

The Separate historical emulator receipt link returns HTTP404; the other four inspected evidence links return200. `emulator-evidence.json` does not exist under the served artifact at review time. README states that this file records historical baseline evidence.

Evidence: `extra-results.json`, links. This is a packaging/evidence-completeness issue. The page otherwise clearly distinguishes illustrative state, source rules and proposed pixels and does not claim DSP/audio validation.

Action: add a receipt with actual version, package hash, exercised inputs/screens and limitations, or explicitly state unavailable with the observed failure. Do not infer an emulator pass from these model checks.

### L3 — Physical control IDs in the README are incorrect

The README Source citations entry says GLOBE ID11 and BT ID6. The pinned `firmware/hal/smk_input.h:80` explicitly identifies GLOBE10 and BT11, consistent with `panel.c:35–38` default mapping. Printed control names in the interactive UI are correct; this is an evidence annotation error that could misdirect later physical/emulator probes. The parent flagged this after the rendered review; I independently read the pinned source and confirmed it.

Action: correct those two IDs in the README and search the design receipts for the same mistaken identities. Keep PATCH12, SCALE8 and arrows2/3. This review does not itself claim to have exercised the real emulator.

## Independent numerical evaluation

Scale0–10, higher is better. These are ordinal review judgments, not measured performance scores.

| Dimension | Score | Basis |
|---|---:|---|
| Native legibility and focus | 8 | Clean high-contrast rows, stable context, readable USB AUDIO FOLLOWS KNOB, no overlap seen across all35 normal rows. Read-only values are missing. |
| Musical information access | 6 | Stable part/transport fixture and explicit held-key limitation help orientation. Status, power and peer identity require web-only inspection. |
| Source navigation semantics | 8 | Entry/back consumption, asymmetric arrows, bounded editing, retained row/category focus and conditions matched inspected source. Keyboard hold diverges. |
| Wrong-edit/action risk communication | 8 | Arrows/K3 inert on actions, SCALE masking and question cancellation are clear. One-press SAVE remains the explicitly disclosed source risk, not a new safeguard. |
| Evidence clarity and completeness | 7 | Source versions and simulation limits are candid; missing emulator receipt weakens the deliverable. No audio or physical-device claims established. |

Overall judgment: 7/10, revise native information access and evidence packaging. Do not turn this number into a release gate or runtime admission.

## What was actually run

- Headless `/usr/bin/chromium` version154.0.8037.57 through installed Playwright, viewport1440×1200, device scale1. Browser scripts `review.cjs` and `extra.cjs` use actual DOM clicks, pointer holds and keyboard events. They read `window.study` only to record resulting states/bounds, not to drive gestures.
- Viewed screenshots with `view_image`: whole page, native categories, STATUS, BATTERY, USB AUDIO, modal RESET, focused peer, final MIDI/AUDIO/PROJECT rows and PLAY cancellation. All saved native LCD captures are240×240; large captures are480×480 CSS renders of the same pixels.
- Exercised first category bound, all35 normal rows through actual K2 gestures, last MIDI bound, every category's last row, held > entry900ms, fresh held > editing900ms, K3 decrement, < back, PATCH/category/row retention, GLOBE exit, action arrow/knob inertness, explicit SAVE simulation, RESET question with GLOBE ignored and PATCH cancel, clip SAVE masking, developer PAD COLOURS inclusion/clip omission, non-SMK category and clip-channel omission, Tune upper/lower saturation, local held-key retirement fixture, and PLAY cancelling question then stopping the running fixture.
- No accidental simulated SAVE/RESET activation observed from navigation. RESET count stayed0 after cancellation; SAVE became1 only on explicit SCALE. Held entry left CLOCK SOURCE AUTO until release; next pointer hold reached EXTERNAL. Read-only peer resisted K3. Tune saturated at+39.3Hz/-40.0Hz per inspected formatter. These are design-model observations only.
- No emulator was exercised by this reviewer. No firmware suite, native build, hardware input, audio continuity measurement or storage operation was run. Existing emulator evidence cannot be checked because its linked receipt is absent. Source-based gesture validation is separate from firmware execution.

## Inputs read and identity checks

- Approved clean archive SHA256 `75c34c301d4667a116f6607a0429dfb2af8e2ecaa02991f5301bef9206410fc2`, independently hashed at `/workspace/a4-inputs/DnBOX-r22-design-intake.zip`.
- A4 paragraph in `/workspace/a4-inputs/plan/plan/design-admission.md:11`; SHA256 `184b48b582b1a297e1c43cd62ad04edbd9bd878e0b0bc8f87875e76f03107919`.
- `/workspace/a4-inputs/plan/physical-route-evidence.md`; SHA256 `38e042dd13822c1c8f0e4ff4bd63fe1ff39b560cffc6a73b919b4993e93f247d`. Read physical identities and Settings-relevant routing/held-key cautions only; no CP1 design reviewed.
- Design README, source-receipt.json and HTML. Source commit pin `94197fc839b00aff47b98ca02f3c6396e4a6eabf`. Independently verified `ui.c` Git blob `eb911f58db83e417c67dd71576331803fa6c829e`, SHA256 `b21c83aace675322c56ee84731cef430078f50307ccb86e12ffae5ebdf7a9f9b`.
- Source sections read: `ui.c:419–510` descriptors, `:810–845` setters, `:935–945` dynamic names, `:1240–1270` Tune formatter, `:1660–1789` list/category construction, `:3854–3933` navigation/arrows, `:3959–4110` question/button dispatch, `:4340–4410` local ownership retirement, `:6575–6645` list renderer; `plat_fm1.c:21–46` clip masks; `smk_arrows.h` route/repeat/rearm; `main_fm1.c:1116` arrow route; `panel.c:28–40` and `smk_input.h:80` physical identities.
- Detached r22 manifest remains stated/unverified in the design source receipt, correctly distinguished from the contained manifest. I did not independently recover it.

The reviewer did not edit design files, Git refs, Library items or firmware. No publication was performed.
