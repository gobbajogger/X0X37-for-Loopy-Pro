# Independent muted-fader source review — 2026-10-25

Reviewer: separate `independent_review` subagent. Baseline source is
`b4f43dcbcc14f5ef059cda5c7650b5996c77160a`; exact reviewed runtime files are pinned
below. Reviewed source commit is `afa5589543a9f496c89ad991077cf4fdef3b4342`
(tree `ddc360c75672f5142f396eb4fe5bdf3ffcd2a762`). Review outcome: **PASS for scoped source behavior and host regressions**.
This is not native budget approval, package qualification, or hardware PASS.
Both normal Elite and Pro native links overflow the code budget by748 bytes,
as reported by the separate native-build owner. This is a blocking native build
failure, not release-ready source; that owner retains the measurement evidence.

## Reviewed behavior

- The sound mirror remains the authoritative intended level. Reverb, delay, and
  master mute use independent runtime output bits. `fxbus_init` clears them;
  project reset clears them without writing a replacement level. Zero remains
  zero through an explicit mute pair; no fallback100 is resurrected.
- Actual part and drum mappings are checked, including all22 drum voices and
  both banks. Their existing `seq.mute` / `seq.vmute` ownership is retained.
  M8 controls master volume; fader8 controls FILTER and gets no mute-volume
  pickup exception.
- Genuine changed samples for the same eligible muted destination may take over
  pending pickup. Untouched repeats, first/reconnect samples without history,
  changed destinations, and outside base edits cannot inherit that exception.
  A matching target lock retains identity; explicit mute edges establish a new
  epoch even when paired between display/keyboard scans.
- Deliberate acquisition uses existing `pref_set` and `motion_touch`. The target
  gets the established one-pass hold or REC request. Lane data, held parameter
  lock values, and unrelated automation remain intact. A sufficiently long mute
  can outlast a temporary motion hold: automation may then own effective audio
  again, while the newly intended base is retained. This change does not grant
  permanent control over automation.
- DSP gates effective return/master gains in both mono and stereo processing,
  after all base/motion parameter writes. Return tails continue internally;
  muted output cannot be reopened by those writes. No implicit unmute occurs.

## Findings fixed during review

1. The first implementation granted the keyboard mute exception on an initial
   late scan without previous physical history. A noncrossing parked100 against
   saved120 could write100. The final exception requires previous known history;
   a fresh-process regression now verifies no initial write followed by a valid
   changed110 takeover. The separate preexisting synthetic-crossing startup bug
   remains owned by PR201 and is not claimed repaired by this branch.
2. Keyboard off/on edges between scans could be missed after an outside edit
   revoked the previous epoch. Shared explicit mute-edge notification now
   snapshots only the intended value and updates eligibility without inventing
   physical input. The independent probe verifies outside90 → off/on → changed112
   is accepted while still muted.
3. Legacy drum-mute tests expected FX mute to overwrite the intended pot with0.
   Those assertions were migrated to preserved pot plus explicit gate state;
   current legacy tests pass. Missing Darwin feature definitions in old scripts
   were handled with a compiler wrapper, without altering production code.

## Verification

`SAN_FLAGS='-fsanitize=address,undefined -fno-omit-frame-pointer' sh evidence/muted-fader-20261025/run-independent-extra.sh`
completed with exit0. The maintained production-linked matrix passed3958 checks
plus a process-fresh late-first-scan case. The independently authored extra main
checks initial-zero behavior and project reset for all3 FX gates; hidden keyboard
mute edges after outside editing; and first-packet versus second-movement behavior
across reconnect for part, both drum kits, reverb, and delay. See
`independent-extra.c`, replay script, and `independent-sanitized.log`. These use
actual UI/store/input/engine queues and DSP; instrumentation forwards every
recorded engine write to production.

Additional independent runs passed:

- `smc_pickup_acceptance_test.sh`:835361 assertions.
- `smc_lock_test.sh`:703 checks.
- `smc_drum_mutes_test.sh`:all22 active drum destinations,10 inactive positions,
  30 knob/fader writes, main mute/FX and ownership priorities.
- `smc_record_feedback_test.sh`:4 real pattern writes and7 emitted projections.
- `smk_fader_pickup_test.sh`:17 existing nonmute checks.

The legacy Darwin scripts were invoked with `CC` pointing to a wrapper executing
`cc -D_DARWIN_C_SOURCE "$@"`; current production-linked muted tests set that flag
themselves. No hardware, radio timing, physical electrical freshness, device audio,
GUI, flash, merge, or binary publication was performed or certified.

## Exact reviewed runtime SHA256

```
453623fbdfd594ae57af30b363565ac7ef1d2c7314eae5e30c5c4e7489797964  firmware/src/app/engine.c
5030782e9dfa0663cba970f2f92f26f44500540354b09840882735c5a2181e5c  firmware/src/app/engine.h
005c8dbbae0e311ce07102b57f5ac42563a2594ee9b3c3e7ffb74c9b0011dccb  firmware/src/app/main_fm1.c
f31a08afb960c039f0df97b01fa78ca9a80dfe249437c8c09a62b13e75c4ba61  firmware/src/app/smc_mixer.c
646cbf165b874759f0b28d37acde31939fc9ce4d0585bb368d91a80c68e2812e  firmware/src/app/ui.c
7913ede96272cff79194ec2f74e12a30ffb9d8d84042fa3eee47b02cb8e0959c  firmware/src/dsp/fxbus.c
081878fac01d97a23b9cc9f78827c0ba51a797ac36ff7fc3698cb5163955320c  firmware/src/dsp/fxbus.h
```

Independent extra source SHA256:
`e68f4cdd3343b68fc7120b9b2173516a1e9ee3cd6ee5297eef88cc0f6eabbd5e`.
Replay script SHA256:
`01129eb2613fe0fa6b51028f3e92c657561c72662ed1ab1998a58101d81a2148`.
