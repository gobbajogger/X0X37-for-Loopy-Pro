# Fresh independent A4 / J37 Settings review

Reviewed 2026-11-14. Read-only reviewer; all reviewer files are under `/workspace/a4-review-fresh`. No design, firmware, Git or Library mutation. No earlier review records or scores were read. This is a heuristic review, not a musician study or firmware qualification.

The artifact makes the basic Settings journey inspectable and largely clear at native size. The asymmetric arrows are stated where they matter, the value/action/read-only distinctions survive at 240×240, and the actual browser controls preserved focus and resisted accidental action activation in the exercised cases. I found two medium source-clarity/coverage issues and one low visual defect. There is no critical or high finding from this bounded inspection.

## Verified subject and inputs

- URL: `http://127.0.0.1:8764/index.html`.
- Frozen file: `/workspace/a4-settings/design/a4-j37/index.html`.
- SHA256 before and after inspection: `c15be0952bf8fc8b16a4b48f530f82e5abe179f7c649e4b3eb14c56fefffca9b`.
- Source snapshot: `/workspace/a4-inputs/source`, attributed to commit `94197fc839b00aff47b98ca02f3c6396e4a6eabf` by the receipt. This directory is a snapshot, not a Git working tree; `git rev-parse HEAD` therefore failed. Independently hashing its `firmware/src/app/ui.c` yielded expected Git blob `eb911f58db83e417c67dd71576331803fa6c829e`.
- Read current README task cards, inventory.json, source-receipt.json, emulator-evidence.json; A4 in `/workspace/a4-inputs/plan/plan/design-admission.md:11`; `/workspace/a4-inputs/plan/physical-route-evidence.md`.
- Read source `ui.c` descriptors/labels at 419–514, dynamic part naming at 935–945, Tune formatting at 1252, Settings rows/focus at 1740–1789, save readiness/queueing at 2239–2296, list entry/arrows at 3854–3933, modal/normal dispatch at 3959–4108, live ownership at 4282–4298 and 4360–4402; `smk_arrows.h` repeat/rearm; `main_fm1.c:1116` physical routing; `plat_fm1.c:21` clip button masks; historical mapping.json. These support six current SMK37 categories, 35 normal rows and conditional 36th PAD COLOURS, and the documented 28-reference historical discrepancy.
- Plan archive and detached r22 manifest were not independently re-downloaded in this read-only review. The receipt candidly marks r22 as a supplied pin, not verified contents.

## Findings, ranked

### Medium — Native SAVE cue conflates immediate dispatch with completing or queueing a save request

Reproduce: restart; GLOBE; K2 + four times; physical `>`; SAVE PROJECT is selected while header says RUN. Native footer reads `SCALE runs immediately`. Press SCALE: web inspector counts a simulated activation, and native pixels provide no queued-request state. Evidence: `g4-r0-native.png`, `13-save-inert-native.png`, `14-save-deliberate-native.png`, observations records of the same names.

Source `ui.c:2239–2266` rejects immediate persistence while `seq.playing`; `save_project()` at 2269–2296 retains `save_wait` and displays a queued message when not ready. The model explicitly disclaims storage and never draws a false SAVED success badge, so this is not evidence that it wrote or falsely claimed an actual write. The problem is that the native instruction and README's “immediate” wording do not distinguish the one-gesture request from deferred persistence in this running fixture. That distinction matters when a musician expects the current project to be durable before leaving.

Action: retain the source's one-press risk, but say `SCALE requests save`; expose a bounded illustrative queued-request state for the running fixture, or expressly label request outcome as outside simulation. Do not imply actual storage completion or add an unapproved confirmation. Cite source readiness/queue behavior next to this scope distinction.

### Medium — Clean LOAD SAVED branch is absent from the interaction artifact

Reproduce: start a PROJECT session and activate LOAD SAVED; it asks. Confirm its simulated load and activate it again without editing; it asks again. The UI offers no clean/dirty project fixture. `dispatch()` opens any row with `r.question` unconditionally; `S.dirty` begins true and remains true. Evidence: `15-load-question-native.png`, `32-load-confirm-native.png`, `33-load-after-load-native.png`, `extra-observations.json`.

Source `ui.c:3886` asks only when `ui.dirty`; otherwise `run_action(ACT_LOAD)` follows directly. The artifact labels itself as a model of source navigation rules, while only a permanently dirty example is inspectable. Since action effects are expressly counters, the second question alone is not a claim of completed firmware load; the missing condition/fixture is the review issue.

Action: make dirty versus clean an explicit design fixture, branch on it, and let the reviewer exercise both question/cancel/restored-row and direct clean LOAD dispatch. Keep all storage effects simulated. Do not pretend SAVE has succeeded merely to clear dirty while playback is still running.

### Low — Underlying focused-row bracket leaks beside the modal

Reproduce: task checkpoint `Reset / cancel`, or select RESET PROJECT and press SCALE. The underlying amber focus strip at x4–6 remains visible approximately y150–170 to the left of the question rectangle. It also persists in clip-mode question rendering. Evidence: `09-reset-question-native.png`, `09-reset-question-large.png`, `34-clip-modal-mask-native.png`.

This is a small but visible stale selection cue beside a modal that otherwise correctly avoids inventing a selected Cancel choice. Action: suppress list focus decoration while the modal owns input, or extend the overlay clearing mask to cover it. Preserve row focus internally so PATCH restores it.

## Actual execution and observations

`review-browser.cjs` and `review-extra.cjs` launched actual headless Chromium `154.0.8037.57` at 1440×1250, device scale 1, through Playwright. Actions used actual DOM pointer clicks, held mouse input, focused-button Enter/Space keydown/keyup, and fixture checkbox controls. Task-checkpoint buttons were used to establish some documented starting states. No action was driven by the `window.study` API; that API was read for observations only. No page errors occurred in the principal 80-checkpoint run.

89 checkpoints were recorded, with native LCD and enlarged DOM canvas screenshots for each (178 PNGs). Native screenshots are 240×240; displayed enlarged screenshots are 480×480, sampling the artifact's 720×720 enlarged canvas. I manually inspected 21 images with view_image: 18 native and three enlarged; I do not claim manual inspection of every captured image. The source backing store supports 3× exports; this review used its displayed 2× view.

Observed:

- All six categories entered through physical `>`; K2 stayed bounded at first and last category. All 35 normal rows were traversed through browser controls. Last rows remained bounded. PATCH returned to the same category; re-entry retained row and scroll across each category.
- Held pointer `>` for about one second entered MIDI + CLOCK without editing AUTO; fresh press changed to INTERNAL. Focused `>` held with Enter repeated and saturated at EXTERNAL. K3 − decreased to INTERNAL. Focused `<` held with Space backed to category without decrementing; re-entry retained the value. Simulation logged 400/80 ms rules; this is not a calibrated real-time device measurement.
- Tune isolated K3 clicks saturated at raw 0/127, rendered −40.0/+39.3 Hz, and did not wrap. Source formatting agrees. I did not test fast physical encoder acceleration.
- K3/held `>` on SAVE left activation counters zero. Deliberate SCALE increased SAVE once. RESET/CLEAR/dirty LOAD opened their questions with counters unchanged; deliberate confirmation increased each corresponding counter once.
- GLOBE, K3 and `>` did not dismiss or confirm RESET. PATCH restored exact RESET row. PLAY cancelled the question and stopped the initially running transport fixture, matching the documented hazard.
- Clip fixture masked SCALE for SAVE and for an already-open question. PATCH still cancelled and restored RESET. Developer PAD COLOURS remained absent in clip mode, appeared in developer/nonclip, and disappeared when developer was disabled. Non-SMK fixture showed five categories and eight MIDI rows, omitting Bluetooth and CLIP KEYS CH.
- NAME and STATUS resisted K3 and arrows; BATTERY also resisted them. STATUS READY and BATTERY USB POWER remained visible at native size. Both peer fixture names were readable in the two focused native detail lines. Long unfocused peer name was ellipsised within its value area. USB AUDIO FOLLOWS KNOB and long MIDI labels fit. The amber bracket, stable musical strip and ordinal make the target understandable; fine 10–11 px text still needs real display/glare assessment.
- Held-local-F3 fixture was retired on Global entry, pattern transport stayed illustrative RUN, and exit retained the retired-key statement. This correctly avoids a seamless-held-note claim. It does not establish audible continuity.

## Numerical assessment

Heuristic 0–10 scales; these are reviewer judgments, not measured task success, a target score, or an implementation gate.

| Dimension | Score | Basis |
|---|---:|---|
| Native readability and target identity | 8.5 | Read-only values, full focused peer identities and long labels read well; small type awaits actual display conditions. |
| Navigation predictability and recovery | 8.5 | Real controls preserve category/row/scroll and explain asymmetric arrows; no erroneous action in the exercised gestures. |
| Action and modal safety communication | 7.0 | Explicit SCALE, cancellation and clip masking work; save request/outcome distinction needs correction; modal bracket leaks. |
| Source fidelity and conditional coverage | 7.5 | Inventory, masks and arrow rules agree with inspected source; clean-load branch is missing. |
| Evidence honesty and reviewability | 8.5 | Proposal, simulation, historical emulator and physical limits are clearly separated; hashes and task cards are inspectable. |
| Musical continuity clarity | 8.0 | Held-key retirement and PLAY cancellation risk are candid; no audio or device evidence supports stronger continuity claims. |

## Limits and disposition

Historical emulator-evidence.json was read as separate existing evidence, not rerun or counted among these checks. It identifies historical 25.1 firmware and exact runtime asset digests, with eight exercised LCD checkpoints and explicit audio/timing limits. This review executed the proposed HTML model only. No firmware rebuild/tests, current-firmware runtime, physical device, MIDI, DSP, storage, tactile, glare or audio-tail check ran. No CP1 or capture-policy judgment is included.

The requested interaction artifact can support a useful design decision after the two medium source-clarity/coverage issues are repaired. This report gives no runtime approval. Parent remains the sole design writer; findings are proposals, and all screenshots/observations preserve the frozen reviewed state.
