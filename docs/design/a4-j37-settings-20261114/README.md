# A4 / J37 Settings study

Design proposal only. Open `index.html`; it needs no server, firmware package or network. Canvas pixels are 240 × 240. The adjacent view displays those same pixels at2× on desktop and scales to fit smaller screens; the enlarged PNG export is3× (720×720). The screen files and contact sheet are generated from the actual browser artifact.

This isolated lane contains Settings only. No runtime, CP1, capture-policy, shared #891, catalog or release edits are authorised. The existing approved plan is consumed, not replaced. The source receipt records the exact input closure. No firmware suite or native rebuild is needed for this bounded design study.

## Source discrepancy resolved in this study

A4 says “existing 28 refs”. `reference/recovered-ui/mocks/settings/mapping.json` at the pinned source describes an older 28-row design. It contains stale 303A/303B and ABOUT X0X labels. Current `firmware/src/app/ui.c:1740–1778` has 35 rows on SMK37, or 36 with developer PAD COLOURS enabled outside clip mode. The eight additions are TUNE, five part switches, QUEUE MUTES and NOTE LOCK. All 28 historical references are retained in the current 36-row inventory; current labels and part order prevail. This is a scoped source correction, not a new settings feature.

## Visual decision and purpose

The proposal uses flat panels, a stable musical context strip, a category breadcrumb, a solid focus bracket, a row ordinal, and a focused-row explanation. Values, actions and read-only information have distinct words and geometry. This helps the player identify what one gesture will change while the pattern continues. It replaces the existing bevel treatment in this mockup only. Category names, row names and printed captions stay source-matched. No global status or pending-sound design is introduced.

For connected peers, the source marquee makes identity take time to read. The proposal instead expands both complete fixture names into the focused native detail strip. This is a concrete musical usability choice: identify the intended controller without waiting or consulting the web inspector. Unfocused NAME remains bounded and ellipsised. STATUS and BATTERY keep their actual values; read-only is stated in the footer.

The strongest usability concern is the source’s asymmetric arrow grammar: `>` enters a category and then increments values; `<` leaves that section. The focused footer states this before input. K3 / ALGORITHM is the route for decrement. There is no claim of symmetric arrow editing. Numeric Tune uses isolated detents here; fast encoder acceleration is source-defined and is not timing-qualified by this simulation.

SCALE immediately requests SAVE PROJECT without a question. Source `ui.c:2239–2296` queues that request while playing or otherwise busy; request is not persistence. The walkthrough counts requests and does not simulate a successful write. RESET PROJECT and CLEAR PATTERN ask a question; dirty LOAD SAVED asks too. The question has no movable “Cancel selected” fiction: PATCH cancels, SCALE confirms. GLOBE is ignored in a question under the source dispatch. In clip mode physical SCALE is masked, so it cannot activate any action or confirm a question. Arrows never run actions. The prototype increments a labelled request/action simulation counter only; it never reads or writes project data. The Unsaved edits fixture exposes dirty LOAD with a question and clean LOAD without one; toggling this fixture is not an implicit save. It intentionally exposes the baseline’s one-press SAVE risk instead of adding an unapproved confirmation.

## Predeclared tasks and expectations

| Task | Initial condition and physical events | Success and failure predicates |
|---|---|---|
| J37-01 Find and leave | Playing Chicago / part B; GLOBE press/release; K2 through first/last categories; `>` press/release; PATCH press/release; GLOBE press/release | Six categories; bounded selection; entry changes no value; PATCH returns same category; GLOBE restores original view; transport fixture unchanged. Wrong category or value mutation fails. |
| J37-02 Held entry | MIDI + CLOCK focused; hold `>` beyond 400 ms and multiple 80 ms intervals; release; fresh `>` | Holding entry cannot change CLOCK SOURCE. Fresh press increments once; further hold repeats at source intervals, saturating at EXTERNAL. A first-row edit before release fails. |
| J37-03 Edit and back | MIDI CLOCK SOURCE EXTERNAL; K3 negative; `<` held; re-enter | K3 decrements to INTERNAL; `<` backs and never decrements; focused row and scroll retained on re-entry. No wrap at first/last value. |
| J37-04 Safe actions | PROJECT; select SAVE, CLEAR, RESET; arrows and K3 first; SCALE only deliberately; PATCH on question | Arrows/K3 cannot execute. SAVE has one explicit simulated activation; RESET/CLEAR remain uncommitted until SCALE; PATCH restores exact row. Any unintended activation fails. |
| J37-05 Clip and condition | Clip fixture on; enter by `>`; K2/K3; select action; SCALE; inspect SYSTEM | Menu encoders and arrows work; SCALE is visibly masked; no simulated action; PAD COLOURS absent in clip mode and production build. It appears only developer/nonclip. Non-SMK fixture omits Bluetooth and clip-channel row. |
| J37-06 Readability | Scroll each category; USB AUDIO FOLLOWS KNOB; long connected peer name; read-only STATUS/BATTERY; TUNE boundaries | All current refs reachable; no overflowing or overlapping text; name clipped within its allocated value area; both peer names remain readable in focused native detail lines. Read-only values resist K3 and arrows. |
| J37-07 Performance continuity | Pattern running; local physical F3 held in Chicago part B; enter Global; back/exit and release | Navigation does not toggle transport or execute a write in the model. Source-derived local live-key ownership is retired on V_GLO entry; no seamless held-note claim. A real audio gap or tail claim requires actual emulator audio evidence and remains unproven here. |
| J37-08 Modal transport | RESET question; GLOBE; SEQ PLAY press | GLOBE does not confirm/dismiss; PLAY answers NO then follows normal transport path. This can stop an already-running pattern; display must not promise an interruption-free escape. |

These are heuristic design tasks, not musician observations or firmware tests. Rejections and continuity limitations are part of the evaluation, not waived to improve a score. Existing host-test source is read as an oracle; its assertions have not been re-executed or counted as passes.

## Source citations

All links pin `94197fc839b00aff47b98ca02f3c6396e4a6eabf` in the authorised repository:

- [Rows, conditional inclusion and retained focus](https://github.com/gobbajogger/X0X37-for-Loopy-Pro/blob/94197fc839b00aff47b98ca02f3c6396e4a6eabf/firmware/src/app/ui.c#L1740): six categories; part order at lines 94–102; descriptors at 419–506; dynamic names at 935–945.
- [Bounded list and arrow behaviour](https://github.com/gobbajogger/X0X37-for-Loopy-Pro/blob/94197fc839b00aff47b98ca02f3c6396e4a6eabf/firmware/src/app/ui.c#L3854): entry, questions, asymmetric back and action inertness.
- [Button and modal dispatch](https://github.com/gobbajogger/X0X37-for-Loopy-Pro/blob/94197fc839b00aff47b98ca02f3c6396e4a6eabf/firmware/src/app/ui.c#L3959): PATCH NO; SCALE YES; PLAY cancels then transport; GLOBE at 4091; normal buttons act on release at 4518–4528.
- [Arrow 400/80 ms repeat and rearm](https://github.com/gobbajogger/X0X37-for-Loopy-Pro/blob/94197fc839b00aff47b98ca02f3c6396e4a6eabf/firmware/src/app/smk_arrows.h#L18), [physical routing and consumed entry](https://github.com/gobbajogger/X0X37-for-Loopy-Pro/blob/94197fc839b00aff47b98ca02f3c6396e4a6eabf/firmware/src/app/main_fm1.c#L1116).
- [Clip masks](https://github.com/gobbajogger/X0X37-for-Loopy-Pro/blob/94197fc839b00aff47b98ca02f3c6396e4a6eabf/firmware/src/app/plat_fm1.c#L21); [encoder ownership](https://github.com/gobbajogger/X0X37-for-Loopy-Pro/blob/94197fc839b00aff47b98ca02f3c6396e4a6eabf/firmware/src/app/main_fm1.c#L1502).
- [Physical captions and default mappings](https://github.com/gobbajogger/X0X37-for-Loopy-Pro/blob/94197fc839b00aff47b98ca02f3c6396e4a6eabf/firmware/src/app/panel.c#L28): GLOBE ID10, PATCH ID12, SCALE ID8, BT ID11, arrows IDs2/3; K2 encoder1 / K3 encoder2. Calibration can alter mappings; no calibration performed.
- [Current held-key ownership limitation](https://github.com/gobbajogger/X0X37-for-Loopy-Pro/blob/94197fc839b00aff47b98ca02f3c6396e4a6eabf/firmware/src/app/ui.c#L4282) and retirement at 4360–4402. Physical F3 is key index5 / central key0 under `smk_input.h`’s C3 chromatic key map.
- [Existing Settings test oracle](https://github.com/gobbajogger/X0X37-for-Loopy-Pro/blob/94197fc839b00aff47b98ca02f3c6396e4a6eabf/tests/host/ui_settings_sections_test.c): inspected, not rerun.

## Evidence and remaining limits

`source-receipt.json` records verified file digests. The supplied archive is verified. The owner confirmed this ZIP is intentionally a clean subset: its archive/member hashes govern consumed content. The separately supplied full r22 manifest digest remains an unverified pin and does not delay this design or imply whole-plan acceptance. `qa/` records actual browser tasks, renders and independent reviews. The complete private ZIP includes all PNGs; the non-main Git branch carries reviewed text sources, receipts and a lightweight native-screen HTML index. Run `python3 serve.py` (local port8764, explicit UTF-8 headers), then `python3 verify.py` to reproduce PNGs; `python3 export-screen-index.py` regenerates the HTML index. No firmware build is invoked. `emulator-evidence.json` records the historical published baseline separately. The simulation has no DSP, storage, MIDI or device timing; illustrative context is never an emulator pass. Physical caption legibility, display glare, tactile burden and audible continuity are not established by browser images.
