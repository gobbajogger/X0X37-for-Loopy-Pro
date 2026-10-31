# F4 integration on the latest #789

Owner instruction, 2026-11-01: rebase onto the latest #789, use its transport mapping, consume arrows without editing while several FX buttons are held, and keep existing FX-strip quick-tap behaviour until the owner decides otherwise. No previews or visual-approval wait. The Orchestrator verifies on the box; no builds or tests run in Codex.

## Base and source provenance

- Latest #789 pin: `b860e160f888a3f12abb852f70ede6f3198a3d81`. The first read found `1ca03fd0`; `bff24bbd` then published the requested transport, and the final fixture-only audit advanced to `b860e160`. The final stack uses `b860e160` directly; its transport implementation is unchanged from `bff24bbd`.
- Previous F4 head: `0ec0e213397fbaf595a914f65e16d999a21ed99a`, originally based on #789 `02c5224d`.
- The five F4 commits were replayed in order. `43e8bed8` becomes `253d3e55` (see exact commit history); `97282d7` becomes `2f3944cc`; `af1a3a5` becomes `4f5ff2c0`; `28b7f72` becomes `56531e66`; `0ec0e21` becomes `74dd0785`. The integration follow-up sits above those replays. The final base advance changes four paths disjoint from every F4 path: three base-audit fixtures and their handoff. The reviewed F4 source is unchanged; its source hashes still match. The full box run includes these inherited fixture changes.
- Rebase conflicts keep F4's clip-action guard and held-FX screen preservation, with #789's transport classification and RW/FF popup preservation. No old F4 PLAY action-card mapping is replayed over #789.
- The existing PR branch is rewritten for the requested rebase using an expected-old-head lease. The old head remains named above. No main, integration branch or other PR branch is changed.

## Transport ownership

PLAY and MMC Play/Stop perform transport without opening a popup. RW/FF cue or cancel the pending pattern and use #789's existing top notification bar for **1000 ms**: NEXT, the HOME pattern glyph, an arrow and the number; a cancelled cue shows the empty glyph and `--`. RW/FF preserve an already open popup and its body. The notification, glyph projection, timeout and source tests are inherited directly from #789, not reimplemented here.

F4's old PLAY popup-toggle assertions are replaced with assertions that playback toggles and no popup is active. The optional capture driver calls the real `ui_frame` after PLAY instead of forcing the obsolete popup renderer. #789's registered five-route transport/notification, glyph-pixel, timeout and popup-preservation tests remain intact.

## Held FX ambiguity and unchanged quick taps

Both arrow routes count eligible physical FX holds across peers. With several held keys, arrows are consumed without changing any mapping, page or held-effect screen. Once only one key remains, a keyboard arrow can edit that key; an SMC arrow only edits its own peer's unique key. A single shared routing helper enforces the same ambiguity rule for both inputs and restores the caller's peer slot.

The existing one-peer ambiguity assertions remain. The optional two-peer case now expects an SMC arrow to make no edit while both peers hold FX, then checks editing after release leaves one target. This replaces the old expectation that a peer-local SMC arrow could edit despite another peer's hold.

FX-strip quick taps retain their current behaviour. No new tap decision is made here. The bodies of `smc_fx_try`, `smc_fx_main_tick`, `smc_fx_main_nav`, `smc_fx_release_note` and `smc_fx_nav_index` were compared byte for byte with the previous F4 head and are unchanged. Existing immediate momentary activation, tap navigation and chord-navigation suppression therefore remain the source contract pending the owner's decision; execution remains unverified on this head.

## Tests and qualification

The production-action extraction fixture gains inactive clip/FX context adapters for the F4 helper calls now present in the extracted wrappers. Its existing assertions are retained; real FX routing and focus are covered by the production UI/engine fixture. No test selector is removed, disabled or bypassed. All additions run through already registered `smc-remainder` and `smc-action-source` selectors.

The Orchestrator runs the affected selectors, optional `BLEC_TWO_PEER=1` remainder case, full host suite with 0 fail / 0 skip, and SDK **V1.2.1** **Elite/Pro** builds with **X0X_DEV_PADCAL=0**. Free static RAM on this F4 head: **Elite NOT_RUN; Pro NOT_RUN**. #789's 2432 B results describe its own pin only. The earlier documented 4 B per-peer original-page mask remains; this integration adds no persistent state.

Compound Engineering static review covers the rebase, transport inheritance, global ambiguity and extraction-fixture changes. Three independent simplification reviewers requested no changes. Correctness, standards, testing and adversarial review completed; the added controller/other-peer test received independent static validation. See the [review receipt](review/rebase-789/review.json), [source hashes](review/rebase-789/source-manifest.json) and [static preservation comparison](review/rebase-789/static-preservation.json). Tests, compilers, builds, framebuffer execution and hardware checks are **NOT_RUN** in Codex. The PR remains draft for Orchestrator box verification; no previews, visual approval, merge or device write are requested.
