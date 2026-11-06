# Independent UI state-reduction continuation

Result: no new blocking defect found in this exact buffer-ownership/API delta. Existing UI/combined finding IDs and review budget remain unchanged (UNKNOWN/UNCHANGED). This is not native-fit approval or release acceptance.

## Identity

Packet commit `d5c328862929b14a49104011a843115a7f42d27e`, directory `evidence/ui-ram-reduction-0260cd3f`.

- Base tree `fba9b1f2df4ca7c075a0f757e60319d04c3a02f9`.
- Verified incremental patch SHA256 `558d038601ff8afdc63097bdd1c5e92d5bf7bddf50bae6022d1c93d01a3add19`.
- Independently reconstructed exact tree `0260cd3f78a1b3717ef988d54aca767d65b6a2bb` in an isolated index.
- Verified all ten changed-file hashes against manifest and all 5,120 tracked leaves against Git after tests: no mismatches. No production source edits.

## Ownership, lifetime and concurrency review

The central-mode getter now returns its caller's buffer, eliminating the shared 43-byte joined-name storage. The NAME descriptor is immutable with a null names table; the UI formats valid value 0 directly, retains empty '-' fallback and rejects other values as INVALID. Active firmware/HAL/host signatures and callers were audited. The NAME item remains read-only; the connected-peer list renderer retains its separate bounded snapshot/marquee path. Other small numeric formatting paths use engine/control refs, not this NAME list item.

The existing guarded peer copy remains unchanged: it copies at most two 20-byte names with NULs, then joins the local snapshot. No pointer to a temporary array escapes. Peripheral mode intentionally still returns the existing global advertising-name pointer, immediately copied by the UI; it is not a newly guaranteed immutable peripheral snapshot. BLE role/state sampling and the name copy are not one atomic transaction across every transition, as already recorded in the prior UI review. No broader concurrency guarantee is inferred.

An independent external harness extracted the exact three production peer/name functions and used only test interrupt stubs. Under ASan+UBSan it injected disconnect/name mutation immediately after irq_restore, then reconnected and switched peripheral mode. The first caller retained the pre-transition name; a second call observed disconnection/reconnection without overwriting the first caller's buffer. Bounds/canaries and peripheral return-pointer behavior passed. No candidate file was modified by this harness.

The waveform cache, peer-scroll state, section navigation, render cadence, LCD dispatch, SMC ownership, mute/BREAK, M7a persistence, panic and runner are unchanged. Their prior exact-input evidence retains its scope; no unrelated audit was reset.

## Independently executed checks

- Confirmed the old/new getter bodies in the supplied equivalence reproducer equal actual base/candidate source (renaming only). Executed with ASan+UBSan: 2,646 outputs/canaries PASS across central/peripheral, zero/one/two peers, all paired lengths 0..20 and comma-containing strings.
- Independent lifetime/disconnect/reconnect harness: PASS under ASan+UBSan, including production bounds/snapshot checks.
- Actual focused runner: ui-settings-sections, ui-peer-names, ble-peer-name-bounds, original-release-ui, ui-perf-identity and lcd-bounded all PASS, exit 0.
- Settings: 494 checks. Peer UI: 65 checks; separate ASan+UBSan execution also PASS, 65 checks, including generic fallback/invalid/canary assertions.
- Original-release UI: 23,176 checks normal and ASan+UBSan; 40 normal/sanitized PNG pairs identical.
- Compared freshly generated candidate images directly to prior independently generated exact-v6 images: all 63 byte-identical (12 peer, 11 settings, 40 original UI). Prior visual inspection is therefore reused on identical pixels.
- Legacy peer fixture remains FAIL: redefined plat_ble_name and missing obsolete bt_peer_text, the same failure classes previously established on base. Its changed API-call sites do not repair that pre-existing fixture. Failure retained in legacy-peer.log, not counted as PASS.

## Memory evidence and limitations

Independently recompiled supplied isolated storage reproducers using `cc -m32 -O2 -fno-pic -fno-pie -c`. Symbol inspection confirms old 43-byte names and 4-byte nm objects are absent in the candidate; const descriptor stays 12 bytes. This is 47 bytes of removed 32-bit static objects, not measured native linker savings. Alignment-dependent section sizes must not be substituted for this figure.

Actual full-host symbols confirm ui584, original_wave156 and peer_scroll68 unchanged, with mutable nm absent. Host no-radio stubs do not measure native BLE storage. No new static/pool/dynamic allocation or new stack array is introduced by this patch; the existing 42-byte local peer snapshot remains.

Native recompilation/fit NOT_RUN here; separate Build owner must establish current-tree static RAM, code, pool, stack and timing. The producer's parent-attributed v6 RAM98324/98304 and pool8268 are not independently measured in this review. Do not infer a native PASS or final headroom from subtracting 47.

Package checks SKIPPED for absent app/loader/SDK inputs. Full unfiltered suite NOT_RUN; DEVICE_NOT_TESTED. Prior legacy-format2, ROADMAP, UI provenance/selection and remaining distribution/release gates remain open with existing owners. No merge, publication, device action or source edit occurred.

Evidence: packet/, results.json, focused.log, equivalence.log, lifetime.c/log, ble_peer_names.inc, peer-san.log, legacy-peer.log, isolated object files, and actual source/build test logs and captures.
