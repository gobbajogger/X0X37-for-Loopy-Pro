# Muted fader intent source draft — native capacity blocked

Source commit: `afa5589543a9f496c89ad991077cf4fdef3b4342` (tree `ddc360c75672f5142f396eb4fe5bdf3ffcd2a762`).
Isolated branch: `fix/muted-fader-intent-20261025`.
Pinned base: `b4f43dcbcc14f5ef059cda5c7650b5996c77160a`.
Contract: reviewed trial `e2482a5039527195e346925c751ac3e3a333ada9`, SKILL SHA256 `2dd8889e0843b4bbd9686cb42d3936921d7e23a4dde688bfea9107e1b8a556b3`.
State: **source/host review PASS; BOTH NATIVE APP LINKS FAIL CODE BUDGET**.
Parent owns subsequent integration; no stage, merge, hardware or publication advancement is implied.

## User-visible result

A genuine same-destination volume move during mute updates the intended project level, even if pickup was pending. Audio mute stays on. Unmute keeps that value and pickup ownership; untouched or duplicate parked samples retain the previous value. No automatic unmute occurs. FX has no cached restore value and never writes a replacement zero. Its actual DSP output gates withstand keyboard, encoder, external base and motion writes, including return tails. M8 remains master output mute; fader8 remains FILTER.

SMC parts use actual T_MIX/MX_LEVEL mappings; drum SEQ/MIX targets use each kit's actual Level descriptor, with existing seq.mute/seq.vmute ownership. Keyboard has seven fixed volume destinations and a distinct bank-B FILTER. A different destination, bank change or external base edit revokes the muted exception; normal pickup remains. Lock retains exact identity; reconnect's first packet seeds physical history. Explicit mute edges remain epochs even between display or keyboard scans. Project reset clears runtime FX mute bits; reconnect preserves them. Zero intended level stays zero through an explicit mute pair.

Accepted gestures call existing pref_set + motion_touch: the target receives its usual one-pass hold or REC behavior. Lane/held-lock bytes and unrelated automation survive. Automation may resume after the temporary hold expires, including before a later unmute; this change does not grant permanent ownership. Existing part/drum note-trigger mute and tail behavior remains unchanged.

## Verification

- `tests/host/smc_muted_fader_test.sh`: 3958 matrix checks plus 3 fresh-process checks, normal and Clang ASan/UBSan. Actual store/queue/engine and mono/stereo FX signals, not substituted mute or pickup implementations.
- Matrix covers moved/untouched, pending/acquired, same/exact target, both protocols, all five parts/all22 drum voices, FX, keyboard banks, navigation/lock/reconnect, rapid pairs, external edits, motion hold/REC, parameter-lock preservation, zero-level and reset cases.
- Independent source review and independently authored replayable probes: `INDEPENDENT-REVIEW.md`, `independent-extra.c`, `run-independent-extra.sh`, and `independent-sanitized.log`.
- Existing pickup acceptance: 835361 assertions; UI5893; target lock703; drum R/S124992; drum mute destinations/priorities; legacy keyboard17. See `host-regressions.json` and logs. Old Darwin fixture feature-macro compile failure was resolved by scoped compiler flags; its successful rerun is recorded.
- Five legacy extraction fixtures passed after migrating only changed API/LED policy. Feedback covers1883 expectations. New mute regression fails the b4 SMC source at the pending muted-movement assertion, as expected (`baseline-negative.log`).

## Native measurement — blocking

Both actual normal Elite/Pro DEV app links fail with XIP overflow748 bytes. Baseline Elite all-TU recompile exactly reproduced the accepted b4 app hash. Diagnostic-only oversized relinks measured553420 bytes per model (+784), versus552636 baseline; real code limit552672. Static RAM rises16 bytes (284 free); pool remains8324 free. The mute byte uses existing FX struct alignment padding. Diagnostic relinks retain failing real-budget checks and are explicitly non-installable. No package/controller or public binary was produced.

See `native/native-size.json`, complete quoted-include source closures and sanitized normal-link logs. No safe spare flash region was found; UI is already size-optimized. Hot-DSP regeneration or unrelated code removal was not introduced to hide the failure. Native qualification requires recovering at least748 bytes against this base, then rerunning affected checks on the exact integrated source. Subsequent PR201 startup integration may change the final amount.

## Integration boundaries and next owner

PR196's pickup fixes were already in b4 and were not duplicated or changed. The separate PR201 keyboard startup fix was not copied into this branch; its preexisting late-first-scan synthetic-crossing case remains parent-owned. This branch adds a noncrossing no-history guard specifically to the new mute exception. SMC navigation PR200 is separate. The original combined branch and other workers' checkouts were untouched.

The parent may inspect/cherry-pick the source commit and evidence, recover flash space under a separately reviewed bounded change, then validate combined source with PR201 and other owner-selected fixes. The remote combined branch was observed advancing after our b4 pin; this does not constitute validation of that newer combined tree. Do not merge, package, flash or publish this draft while its native capacity check fails.

No hardware, GUI, radio/device timing, physical fader/ADC freshness, flash, merges or public binary publication occurred. Host/static evidence cannot certify device audio or automation timing. No operation was denied or remains uncertain.
