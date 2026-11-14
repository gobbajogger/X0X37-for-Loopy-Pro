# J1 exact-head host qualification, 2026-11-14 UTC

**Result: scoped host PASS; J1 product/integration qualification remains HOLD.** No observed failure in the bounded checks below. No production source was changed. The exact candidate still contains the earlier Juno `/6` gain, so these headroom passes do not accept the later approved louder gain or a future integrated candidate.

## Exact inputs and recovered evidence

- Repository: `gobbajogger/X0X37-for-Loopy-Pro`.
- Candidate: `lane/j1-juno-stock-curated-20261113@247dd2621068906d47e90056100f9193bd318cb1`, draft [#905](https://github.com/gobbajogger/X0X37-for-Loopy-Pro/pull/905). Root tree: `1f1dae9339df3b71e2380b5320c0f590a04065a5`.
- Base: `94197fc839b00aff47b98ca02f3c6396e4a6eabf`. Canonical [ROADMAP](https://github.com/gobbajogger/X0X37-for-Loopy-Pro/blob/32b0cacd17c89f23bb2f16915732750d158b33e7/docs/ROADMAP.md) was consumed at `32b0cacd17c89f23bb2f16915732750d158b33e7`, including J1, F1, gain and owner boundaries. Maintained development policy read at `nightly@66073c68dd80d51a7053432c2c161a8b720a1341`; exact candidate instructions also read. Current delegated Work authorization governs this scoped execution over older Classic-only routing text.
- Complete source recovered by the public GitHub API archive, independently checked against the authenticated recursive tree: **5,349/5,349 blobs matched, zero mismatches**, before and after execution. Source files were made mode 0444 and directories 0555. Build products and generated assets stayed outside source. Existing shared checkouts were read only and not used as runtime input.
- Important exact blobs: `juno6.c e8295cd6c87efe0ce8a528aeefb3154c2a52564e`; `fastmath.h 666d8e1325d32a76f7b4b964f2808420d3987104`; `original_sound.c a250a77f9d20fb1973b1251dd9f9b5c205776b45`; `original_runtime.c 45f4bc53008ca2d65a979dc90634e731d9aeeb09`; `juno_stock_bank.h 3cd1e4175bda6506ae7312c0a5af8b5b5583b88e`.
- Recovered [independent QA 6865957073](https://github.com/gobbajogger/X0X37-for-Loopy-Pro/pull/905#issuecomment-6865957073): exact old rows and exhaustive 256-value pattern codec PASS, with a declaration shim and extracted factory-count function. Reused as scoped historical proof; its codec-only matrix was not repeated.
- Read candidate `docs/sounds/jun60-stock-bank-j1.md` and PR body. Their earlier 16-scene renders used bootstrap fastmath `7d2e41bf...`, and remain real DSP evidence with that dependency limitation. The new exact-dependency runs below replace that gap; they do not retroactively make the old run exact-tree.
- Read F1 #900 ownership/QA receipts through `d963c3925a5b05f329e24c1a793a6326118fa466`. Its 1,068/407 predecessor assertions and 180 repair assertions were not rerun or transferred to this head. #900 has received the appended-selector contract in comment 6860644190, but that handoff alone does not establish completed J1 integration.
- Library search recovered the historical preview WAV identity but no additional J1 qualification report. No preview listening or listening judgement was performed. The live coordination ref returned 404; historical role names are not a verified active registry.

## Executed checks

Host: Linux x86_64, GCC 14.2.0, glibc 2.41. No target SDK, native build, emulator or physical device was used. All listed compile/run exits were **0**.

| Check | Result | Coverage and limits |
|---|---|---|
| Exact committed `juno_stock_bank_test.sh`, GCC O2 | PASS, 16 scenes | Eight factory sounds, musical and six-voice velocity-110 scenes, 44.1 kHz, actual Juno and stereo chorus, two seconds per scene. Exact candidate fastmath. |
| Same stock test, GCC ASan/UBSan | PASS, 16 scenes | No sanitizer diagnostic; no local math patch. Maximum dry and chorus peak `0.40387064`, zero samples at or above unity. No gain normalization. |
| Existing production runtime smoke | PASS | Actual six voices, sustain/release/all-off and render; runtime/backend sizes reported as host structure sizes only. |
| New `j1_cell_runtime.c`, O2 | PASS | Eight factories; 640 bounded edit cases; 2,344 malformed cells; 230,852 assertions including sample-finiteness checks. Counts are assertion operations, not independent musical scenes. |
| Same cell/runtime fixture, ASan/UBSan | PASS | Identical case summary, zero diagnostic. Full production `original_sound.c`, `original_runtime.c`, Juno and declared original-synth/backends linked without replacement implementations. |
| New `j1_saved_patterns.c`, ASan/UBSan | PASS, 1,454 assertions | Actual production project save/load/validation, UI pattern tick, sequencer, selector and sound runtime with repository host platform substitutes. Generated fonts, drum tables/samples and built-in BREAK from exact source/assets outside source. |

The cell fixture verifies all eight schema-2 32-byte cells, exact metadata/index/name, unedited values, live-only bend/damper neutrality, ignored historical performance bits without input mutation, and ordinary edits at values 0/1/63/64/127 for each of 16 stored controls. All four legacy cells are compared with complete fixed expected bytes derived from base `original_sound.c@6268b4191c9f348d7d78354a92d95c87b6341e63`, including old names and indices. Bad parameter bytes, unused bytes, unsupported mask bits, magic/schema/engine/index/flags and selectors 8–255 are refused. Failed value/set/init operations preserve their checked destination and input state. Binary cell save/reload adopts through real runtime init and renders positive finite energy, then all-off silence.

The integration fixture selects all eight factories in both slots and checks the other slot is unchanged. It stores old/new `0xE1..0xE8` codes using real project stream14, simulates reboot, and recalls each through production `pattern_sound_tick` plus the gated selector on both slots, comparing actual parameter bits with the real cell reader. It covers cancellation, out-of-range selectors, saved `0xE9..0xFF` rejection, old stream13 reserved-field clearing, and edited SUB BASS cell/tag save and reboot. This is broader than the recovered codec-only harness; it is not an actual firmware upgrade or power-loss test.

The host replaces physical HAL, MIDI and flash with the repository implementations; flash is RAM-backed object storage. No production DSP, cell codec, factory function, selector, project codec or pattern consumer was mocked or extracted in these new fixtures. Whole-app compilation emitted existing UI/host warnings (signedness, indentation, fallthrough and an unused variable); the full log is retained, and no warning-clean claim is made. No UI render or browser acceptance is claimed.

## Findings and remaining gates

1. **Louder-gain composition is unqualified.** Exact J1 `juno6.c` still uses `mix *= macro[15]/6`. The owner-approved [gain commit](https://github.com/gobbajogger/X0X37-for-Loopy-Pro/commit/d451ab10f8bfb761a92f1028ea23b0ee4627b4e5) changes that to `/1.85f` and raises Drone output. It is a different source input. Do not lower or normalize approved gain to obtain a pass. As an explicitly unmeasured linear inference, the new SUB BASS six-voice dry peak would scale from about 0.404 to about 1.310 if only that divisor changed; this is a risk signal, not a render at the louder gain. The existing producer/integrator must supply one actual composed immutable candidate, retain approved gain, measure clipping/chorus input and mixed-output headroom, then disposition any measured clipping. No array/DSP fix was applied or silently proposed as accepted.
2. **Stale source comment:** `bass_pattern_preset.h` documents only `0xE1..0xE4`; actual count-based code accepts through `0xE8`. Minimal proposed fix is a comment-only range correction by its incumbent preset/pattern owner. No encoding change is needed for the passing scoped cases.
3. **NOT_RUN:** exact-head Elite/Pro SDK V1.2.14 builds and >=8,192-byte flash/pool gates; full mixed two-part/drum/FX headroom and output conversion; CPU/deadline/XRUN; hardware listening, USB/speaker levels, physical controls, long-duration/power-interruption and live transport/storage acceptance. Existing #895/base budgets cannot qualify J1. Full unrelated suites were not run. No native/device/listening PASS follows from these host checks.

## Release state and consumer

Source producer retains `lane/j1-juno-stock-curated-20261113` and all stock/DSP paths. F1 #900 owns native preset/storage adoption; pattern owner retains saved-pattern implementation; the assigned integrator owns source composition and model builds; #891 owns UX. This worker has released all shared writes and claims only its isolated evidence. No main/nightly/producer/integration ref changed, source takeover, SDK workaround, hardware operation, firmware publication or merge occurred.

The creator can consume this report and exact fixture/log bundle now. **Next action:** route the gain-composition/headroom finding to the incumbent J1 producer and integrator; keep #905 draft/HOLD until the actual composed candidate and remaining gates are qualified. Later producer consumption is not verified by merely writing this report.

Standard `thread_management.create_thread/read_thread` and environment configuration creation/update tools were not exposed in this worker's tool catalog. Environment status alone is available. No successor was created or admitted, no browser creation or uncertain admission was attempted. No distinct unowned ready source stage is inferred while incumbent composition ownership remains. Return structured completion to the existing creator for the next authorized scope.

Evidence: `exact-stock*.log`, `cell-*.log`, `patterns.log`, compile logs, command receipts, `archive-verification.json`, `post-run-source-verification.json`, `dsp-metrics.json`, source manifest and recovered receipts. Fixtures reproduce only their stated host coverage.

## Delivery disposition

The qualification bundle was saved to Library. The first GitHub `create_blob` for this report was denied by automatic approval review before execution. After reading newly recovered direct owner instructions requiring necessary work artifacts in Git, the identical report upload was retried once. Review denied it again, stating that explicit approval for this sensitive report and the specific repository was still required. No blob, commit, branch, ref or PR was created. Git delivery is **BLOCKED / NOT PUSHED**; no alternate route or split-payload bypass was attempted. The proposed path remains `docs/evidence/j1-247dd262-host-qualification-20261114/` on isolated `chat/j1-247dd262-host-qualification-20261114`, with 22 report/fixture/log/receipt files and no production changes. `DELIVERY.json` records the exact action and next owner.
