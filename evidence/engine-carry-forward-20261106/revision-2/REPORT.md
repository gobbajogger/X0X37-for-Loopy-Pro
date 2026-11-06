# Revision 2 — cold Chicago packed-cache boundary repair

Existing independent QA reported that a cold `UINT32_MAX` lookup incorrectly succeeded. Reproduced: new regression exits 1 against prior candidate `01194a7f7fd0043833bf657756939819e23c99ca`, at its first lookup. The exact proposal's uncached implementation passes the same test.

Revised candidate tree: **`ca1658126e78a84e33c5a6a430d11bb7362ee659`**.
Original proposal base: `2e38c67d3dbf1b905085580229e395433aefd92d`.
`repair.patch` applies to prior candidate; `engine-carry-forward.patch` applies to the original proposal. Exact checksums are in `identity.json`. Repair-patch reconstruction in an independent index matches the revised tree. Source remains NOT PUSHED; this is evidence delivery for the same reviewer and integrator.

## Minimal change and regression

Add a bounds check against both packed-table counts before consulting the cache. This prevents the `UINT32_MAX` cold sentinel from matching an invalid request. No additional cache state, numerical approximation, effect, control or memory allocation is added. The existing decoder and valid-index cache behavior are preserved.

Only four paths differ from the prior candidate: `experiments/original_synth/chicago_packed.c`, new `tests/host/chicago_packed_cache_test.c` and `.sh`, and one runner registration in `tests/run_tests.sh`.

PASS: new regression checks cold UINT32_MAX rejection before any valid access, every one of 16,385 valid entries against the uncached decoder, repeat hits, warm count/count+1/UINT32_MAX rejection, invalid arguments and untouched output on rejection. **98,324 checks** each normal and ASan/UBSan. The same test passes against the exact proposal baseline. See `packed-red.log`, `packed-green.log`, `packed-base.log`.

PASS: serialized whole-engine test against freshly rebuilt exact proposal: **17 scenarios / 50 audio-state files bit-identical** (`audio-serial.log`, `audio-serial/hashes.json`, `audio-base-refreshed/hashes.json`).

PASS: one fresh exact-FMA synth differential round, 20,447,232 bytes bit-identical (`peak/result.json`). This verifies the affected synthesis path; prior timing measurements remain scoped to revision 1, and no new speedup claim is made.

PASS: native Elite and Pro DEV builds with pinned JieLi 4.0.1, SDK V1.2.1 and `X0X_DEV_PADCAL=1`; runner syntax and patch whitespace. No firmware or device publication.

## Corrected explicit per-model resource table

These are measured revised-candidate build/linker results, not an inference from another model. Pool usage is `_pool_end - _pool_start`; linker POOL length is `0x54000` = 344,064 B. Both models have start `0x01c20000`, end `0x01c71f08`.

| Metric | Elite DEV | Pro DEV |
|---|---:|---:|
| App image | 493,612 B | 493,612 B |
| Static RAM used | 96,372 B | 96,372 B |
| Static RAM limit | 98,304 B | 98,304 B |
| Pool used | 335,624 B | 335,624 B |
| Pool limit | 344,064 B | 344,064 B |
| Pool free | **8,440 B** | **8,440 B** |
| Required pool free | 8,192 B | 8,192 B |
| Margin above gate | **248 B** | **248 B** |

See `resources.json`, `native-elite.log` and `native-pro.log`. This repair adds no measured image/RAM/pool cost versus revision 1. The earlier combined-column table is superseded by this explicit table. Other lanes still require combined-budget revalidation.

## Initial failing capture and isolation

The initial revision-2 whole-engine capture (`audio.log`) differed in 45 files. It was compiled concurrently with native generation in the same checkout. Both build routes use shared `build/gen`: host generates built-in BREAK loops while native generation can restore/write the empty default BREAK bank. This run therefore does not establish matching generated inputs and is retained as a failed attempt, not silently counted as PASS.

Isolation: freshly rebuilt proposal remains identical to its prior capture; a rebuild with the exact prior packed source matches all prior hashes; an instrumented fixed lookup reports no invalid index in the 17 scenarios and also matches the prior capture. The final uninstrumented fixed source, built serially after native generation completed, matches all 50 files. No production source was relaxed to make the comparison pass. Run host generation and native generation serially within one checkout, or use distinct checkouts.

## Same-reviewer handoff and retained limits

Existing independent engine QA should review this four-file delta against prior `01194a7f…`, repeat the cold-boundary regression, and verify the revised exact tree and explicit Pro resource table. Prior independent 17/50 verification is historical; revised independent acceptance remains pending until that reviewer executes it. No replacement reviewer is appointed.

All revision-1 exclusions remain: no approximate X0X_FAST_FMA, restored-insert architecture/extra effects/81-program selection, unrelated UI/project/recording/allocator/build-policy changes, or donor drive/FX2/output/profiler additions. Newer EQ/VCA/control/idle paths remain preserved. Full-suite/release-mode/controller/package/installer qualification, native PCM execution, device CPU/dropout performance and hardware acceptance are not claimed. The existing #698 sole integrator retains source composition and follow-on gates.
