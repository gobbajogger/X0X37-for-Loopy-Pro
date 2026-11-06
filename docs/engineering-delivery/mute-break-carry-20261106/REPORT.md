# Instant-unmute and BREAK carry-forward

State: isolated source patch and local test evidence; source candidate NOT PUSHED. This evidence-only delivery does not integrate or qualify firmware. Independent exact-tree review remains pending; no new PR.
Base HEAD: c295af1554f9e80a29864cd013995eccad66cd2c.
Base tree: d98072ba1d689cb99bd16e4a6e1e682b22b04c70.
Candidate staged tree: 3b21f19e1b8dc6d4fa9000c76c368a23379a23d1.
Patch SHA-256: 13e622c5aa5e011fdc52074ee5ceb8004cc5dcf837e959757cbbba07e14e8e76.

## Donors and prior evidence

- 0.0.24.5 donor: 8be16486e33f65b31c76710e1d825b6aa701c2c9; tree f952ab4455723e4a1a68fdb3a8eea22d2da59710.
- rc1 donor used for exact extraction: c32c54ac8ec68aaca53adccf7af66f155590f2dc; tree ac490f7fae9198748aa54bd914d42f99d45444f9.
- Donor sequencer.c and the two restored C fixtures have no differences between these donor commits.
- rc1 docs/engineering-delivery/live-risks-023/README.md records independent QA at 94a7a609: synth-bus-mute 144 checks including ASan+UBSan, linked to PR810 comment 6548391398. This is historical reported evidence, not current-tree acceptance.
- rc1 docs/live-a.md documents BREAK immediate fade, held-audition override, next-real-step unmute and the bounded control-write limitation. That document explicitly records its original producer runtime checks as NOT_RUN. Current local BREAK tests were run anew.

## Minimal production change

Only firmware/src/seq/sequencer.c and firmware/src/app/engine.c change: 25 added and 3 removed lines. Synth internal NOTE/TIE progression continues behind existing mixer dry/send gain glides. MIDI is flushed/suppressed at the existing step boundary while muted. A TIE after unmute does not manufacture a MIDI Note On. This is the existing donor policy, not a new external-MIDI instant-resume feature.

BREAK receives the exact donor brk_mute_boundary helper and render-loop call. The current step fades using its existing gate ramp, independent of the command queue. Unmute waits for a real enabled, probability-accepted step; held-slice audition keeps its donor override. FX tails remain. No new production data, DSP edits, per-sample loop, UI changes, chain changes or panic changes.

Combined SEQ_PART_OFF guards remain in fire_bass and BREAK delivery/quiet control. Existing chain window, random timing, note ownership and panic recording cleanup remain untouched.

## Conflicts and adaptations

No wholesale cherry-pick: donor sequencer contains older chain code and lacks the combined OFF/panic changes. Only the two mute hunks were transferred.

The restored BREAK harness initially failed compilation on newer panic-owned symbols. Added inert panic declarations and an inert engine_notes_off_apply fixture boundary, while retaining exact production extraction and real BREAK DSP. Actual panic behavior was tested separately. The other three donor test files are byte-exact copies.

M7a part_off_test.c intentionally asserted the combined base's mute-as-G_REST behavior in two places. The original test failed exactly independent_of_mute_solo after restoration. Those two assertions now require internal progression and silent MIDI for ON+muted; OFF assertions remain unchanged. The restored real-audio donor test proves audible bus silence. This is an explicit semantic reconciliation, not a hidden expected-count change.

Shared-path integration: parent must serialize the two engine.c hunks plus its render-loop call and tests/run_tests.sh registration with other engine/test-runner writers. UI was not edited. The isolated task writes no coordination branch. Parent remains receiver and integration owner; independent exact-tree QA remains with its existing reviewer.

## Executed proof (Linux, GCC 14.2.0, Python 3.12.14)

PASS:
- Exact copied synth donor test: 144 checks; held MIDI/pedal notes, both engines and slots, sequence ties, queued SMC mute, muted faders. Repeated under ASan+UBSan: 144 checks, exit 0.
- Adapted donor BREAK production control + real DSP: PASS; abrupt-cut control exits its expected 3. Normal peak step 0.0043002 <= 0.0095605 bound; late peak 0. Muted queue saturation, render splits, song events, unmute/probability and audition covered.
- New production-linked sequencer mute check: 42 checks, 0 failures. Reuses existing panic fixture, covers MIDI flush/suppression, muted NOTE progression, unmute TIE, OFF and panic.
- Six new mutants killed by behavioral assertions (compile errors rejected): internal release, MIDI leakage, retained owned MIDI, bypassed OFF, removed synth bus gate, removed BREAK boundary.
- Existing sequencer, clk-lock, chain-longest, panic-chord, panic-notes, panic-clear, panic-route: all PASS via tests/run_tests.sh. Chain-longest also kills shortest/first-end mutants. Sequencer output includes external clock-source switching coverage; no separately registered clock-handover selector exists in this source.
- Adapted M7a: 57 cases PASS, 0 failing cases.
- git diff --check; patch reverse-apply check; no unstaged changes; three unadapted donor test files byte-identical.

Meaningful RED before production patch: synth donor test fails audible mute assertion. BREAK original harness compile failure is retained; after panic-fixture adaptation it fails audible fade, queue, split/song, unmute and audition assertions on unchanged production source. Those logs remain separate from PASS receipts.

NOT_RUN: M7a all-ON bit-exact versus historical base and cross-firmware storage (PART_OFF_BASE_REF not set); independent exact-candidate review; full suite; native Elite/Pro builds, CPU deadlines/RAM budgets, hardware and publication. The runner also reports the unrelated SMK package tests skipped for missing built package/SDK inputs; no package PASS claimed. Existing pre-existing compiler warnings are retained in logs.

## Remaining semantics and next owner action

No unresolved implementation choice within the requested donor carry-forward. Instant synth recovery means the existing bus gain glide, not zero-sample latency. External MIDI and BREAK do not have identical resume semantics to internal synths, as documented above. Any demand to change those policies needs a separate scoped decision.

Parent should apply carry-forward.patch to the exact base (git apply --index), verify git write-tree equals the candidate tree, then route immutable candidate to existing independent QA. Reconcile shared engine/test-runner hunks if its combined source has advanced. No merge, push, firmware publication or device operation was performed or authorized by this patch.
