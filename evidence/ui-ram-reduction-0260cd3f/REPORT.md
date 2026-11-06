# V6 UI static-RAM reduction

Base tree `fba9b1f2df4ca7c075a0f757e60319d04c3a02f9`, reconstructed from evidence commit `66cfa3d0724a039f29803c7770ae0d6233f7fada` and source base `c295af1554f9e80a29864cd013995eccad66cd2c`. Candidate tree `0260cd3f78a1b3717ef988d54aca767d65b6a2bb`. Original source stage was isolated and unpushed. This follow-up publishes the incremental patch and nonsecret evidence only; it does not apply source changes to a shared branch. Apply `ui-name-storage.patch` to exact v6; SHA-256 `558d038601ff8afdc63097bdd1c5e92d5bf7bddf50bae6022d1c93d01a3add19`. Clean-v6 application check PASS.

## Change

Remove the redundant `plat_ble_name` static 43-byte joined-name buffer. The internal getter now takes a caller-owned output buffer (at least 43 bytes); all production HAL/host/no-BLE call sites updated. `pref_value_of` supplies its existing numeric/text output buffer. The UI NAME descriptor is now immutable and has no writable names pointer. Preserve empty fallback `-`, invalid values `INVALID`, peripheral advertising names, two-peer ordering, duplicate names and 20-byte bounds. No new stack array, dynamic allocation or pool storage. Existing peer-scroll state, waveform cache, settings state, renderer, cadence, dispatch and mute/safety code are unchanged.

Five production files plus five affected test files change; 44 insertions, 27 deletions. Legacy test call sites are adapted to the signature but its unrelated preexisting failures are retained.

## Memory evidence

Actual GCC `-m32 -O2` compile of exact extracted production getter and descriptor blocks: old `names` object 43 bytes plus `nm` pointer 4 bytes; both absent after change. **47 bytes of static objects removed in the 32-bit object check.** Old/new isolated BSS sections 75/0 bytes include GCC-specific 32-byte alignment padding and must NOT be interpreted as 75 bytes native savings. No change to descriptor const size (12 bytes).

Actual full host symbols: mutable `nm` pointer 8 -> 0 bytes; peer_scroll 68, original_wave156 and ui584 unchanged. Host builds use the no-radio stub, so they do not measure the native BLE name-buffer removal. Exact source contains no new static state and does not move bytes into X0X_POOL. `storage-layout.json`, `storage.log`, and the extracted C reproducers retain the evidence. Compile the storage reproducers with `cc -m32 -O2 -fno-pic -fno-pie -c`; inspect with `nm -S`. Compile name-equivalence.c with ASan/UBSan and execute it.

Parent-reported native v6 failure: RAM98324/98304 (20 over), pool8268. These were NOT measured in this task. Removing 47 object bytes would nominally leave 27 bytes headroom before compiler/linker effects. **Native final fit, RAM/stack/code budgets and pool free remain NOT_RUN/UNKNOWN.** Existing native Build owner must link this exact tree and inspect its map. No SDK/toolchain or denied helper was accessed, retried or bypassed.

## Actual checks

PASS: 2,646 old/new production name outputs and canaries with ASan+UBSan across central/peripheral mode, 0/1/2 peers and all pairs of name lengths0..20 (including comma-bearing names). PASS: production bounds/snapshot fixture with output canaries and ASan+UBSan.

PASS: settings494; peer-display65 including added generic NAME fallback/invalid/canary checks. Baseline and candidate produce byte-identical 12 peer and11 settings framebuffer pairs. No raw captures included in this packet. Full original-release-ui PASS23,176 in normal and sanitized runs;40 normal/sanitized PNG pairs equal. Focused runner PASS play-question-ui, sound-reset-ui, ui-perf-identity, ui-peer-names and lcd-bounded. Package checks explicitly skipped; not a native pass. Reuse unaffected v6 proof; no full unfiltered suite claim.

Existing legacy `smc_peer_names_test.sh` remains a baseline failure from the previous stage: redefined platform name getter and missing obsolete bt_peer_text helper. Its API call sites were updated; no deletion or PASS claim. No independent QA approval claimed by this author.

## Next owner

Parent/existing RAM/build owner: apply only this incremental patch on exact v6, verify candidate tree, then measure the native link with already-authorized inputs. Existing QA may review the small internal buffer-ownership/API change. No shared source writes, merge, firmware publication or devices.
