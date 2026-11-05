# Independent review — 163796c0

Verdict: **NEEDS CHANGES** for test integration and proof reporting. Broader full-suite/native-build acceptance is **INCOMPLETE**. No production sound/state/PCM defect was demonstrated by this review.

Scope: one independent native reviewer; no Sol identity is claimed. Review only, with isolated scratch builds. No producer source, Git ref, PR, automation, device, release or publication changes.

## Exact inputs

- Repository: gobbajogger/X0X37-for-Loopy-Pro.
- Lane: lane/build-25-all-opt-struct-20261106.
- Fresh remote HEAD before and after review: f5b8faef0a00d232a236d5c8708653c8752bcc83, checkpoint-only child.
- Production commit: 163796c0f74c17471a62db44905b02292cf49559.
- Production tree: d0d8b422023cb815abf2b8cfefadbcef9437062d.
- API ancestry: 163796c0 -> 9231c26285c09356b49b2d9afbcd225377aae5c5 -> 78f4358588da8a213e8319ddeadf5cb88e7fb2de -> required a06e20189141ff5fb8766e2c6152be460f4356c3.
- Complete recursive tree API response was not truncated. Verified 4,804 extracted non-.github blobs against Git object hashes, including modes/symlink contents. No mismatches. All 96 changed blobs from original base are within the declared optimizer/prerequisite/tests/evidence scope; no deletions or .github changes.
- Exact parent commit and tree were independently reconstructed in scratch for git-show reference tests; parent commit hash matches. Existing source checkouts were untouched.
- Coordination snapshot: 4a35a6cdaa0f112db6b3f577542138afd170c20d; runtime contract, roles, requests and current-delivery read. Neither requests nor current-delivery contains this lane or either candidate SHA. This does not prove other contexts inactive. This reviewer does not impersonate registered automation.
- Environment: native exec, cwd /workspace, Linux 6.18.57, GCC 14.2.0. This executor does not expose a verifiable backend environment ID/model/effort/tier selector. Parent's checkpoint names the shared managed instance, but that is not independent runtime identification.

## Findings

**QA-01 — P1, missing exact combined-candidate qualification receipts (INCOMPLETE).**

The 336-pass, 486784-byte Elite/Pro results exist as a claim in the production commit message. The candidate's only new full-suite/native evidence directory is evidence/recut-build-25-struct-20261106. Its run-record.json:3 names cc6566db154c57ff13f1cb8c03fbedeac61b1c20; README.md:13 explicitly describes the narrow 333-check run. native-metrics.json records the narrow 475000-byte candidate, not 486784. This directory is byte-for-byte identical to the historical checkout. The new docs/scheduler-handoffs/performance-ima-delta-lut-20261024.json is explicitly an older R18 projection, also not broad current proof.

No matching combined run record, full transcript, full input/tool/reference closure, Elite/Pro build logs/maps/package identities or resource receipts was located in the complete candidate tree or shared recut scratch. The metadata child only adds the checkpoint. Therefore the reported broad PASS, RAM and pool results cannot be independently accepted. The historical 333/475000 numbers remain valid only for their narrow source.

**QA-02 — P2, optimizer regression runners are outside the full suite (NEEDS CHANGES).**

At tests/run_tests.sh:428–434 the 909 area registers drum909-adpcm and ima-delta-lut plus existing upstream-comparison runners, but never registers tests/host/run_drum909_struct_equivalence.sh. Neither that structural script nor run_drum909_emit_equivalence.sh, vca_sustain_test.sh, chicago_fx2_zero_skip_test.sh or master_idle_identity_test.sh is invoked by any other test shell. Similar new optimizer tests are also standalone. Thus a full-suite PASS does not continuously enforce these exact state/PCM/structural preservation gates. The historical narrow README's registration statement is historical and does not describe the present runner.

Register the required optimizer regressions in the maintained suite or a required, explicitly reported supplemental aggregate. Keep the exhaustive check's required status visible: the standalone structural runner currently permits SKIP_TANH_EXHAUSTIVE at line 32. This review did not set it or disable any check.

**QA-03 — P2, IMA compiler coverage can disappear silently (NEEDS CHANGES).**

tests/host/run_ima_delta_lut.sh:9 uses command -v "$CC" || continue. In this actual executor GCC was available and host Clang was absent from PATH; the script exited zero after only two GCC configurations and printed no missing-compiler/NOT_RUN result. A parent suite can consequently report an unqualified PASS/zero skip while the advertised GCC/Clang matrix is incomplete. Emit a distinct per-compiler NOT_RUN or fail the required compiler gate, according to the accepted coverage contract. Do not substitute the JieLi target compiler for a host Clang check.

## Executed checks

26 distinct focused checks ultimately passed. Commands, exits and timings are in focused-results.json, additional-results.json and supplemental-results.json; per-check complete text logs and SHA-256 hashes are in receipt.json. Native ASan/UBSan/LSan execution used the supported per-command network grant; no sanitizer was disabled.

- 16 runner checks: IMA (GCC optimized and sanitizer plus expected invalid-command exit 2), VCA sustain (O2/Os, native/software FMA), Chicago insert idle, five restored FX2 zero-skip types (40 seeds, IEEE and FTZ), master idle, FX-bus idle/cache, Chicago peaking cache, unchanged BPM, modulation/Ensemble hoists, A3 master/808/FX-bus at O0/O2, BREAK silent skip and clip key/arp latency equivalence.
- 10 additional checks: engine level/send LUTs; undo word scan; LZ maximum-match; bus FX invariants; BREAK retrigger LUT/trigger state; gain LUT; project streaming copy; broader 909 emit; original control LUT.
- Representative actual results: 909 emit 23280 blocks at each O0/O2, all 7648 state bytes and four PCM buses equal; bus FX 132545 cases; undo 2107; LZ 20300; project stream 274 chunk cases/15 projects; BREAK 1536 table comparisons and 24576 complete trigger states; VCA 10742008 comparisons per configuration; mixer helpers each exhaust 128 pots and compare 10 million random fallback integers.
- Initial gain-LUT compilation failed at link because the ad-hoc command omitted dead-section elimination, leaving an unrelated oscillator symbol unresolved. The log is preserved. Corrected command uses -ffunction-sections -fdata-sections -Wl,--gc-sections, without changing source/assertions; the real test then passed. This harness correction is not a repaired production failure.
- Historical tanh-only proof can be reused for that helper: firmware/src/dsp/drum909_dsp.h, tools/gen_drum_samples.py and both generated drum headers are byte-identical to the historical tested closure. The helper header SHA-256 is 43dd75fae722827a45e68cf8f3f336728bae3e492cd8f247d1979721eca824f4. This does not turn the full historical suite or native builds into broader proof. No fresh exhaustive binary32 run is claimed.

Reviewed relevant invariants/callers: repeated coefficient setters and exact boot defaults; call-owned modulation activity and queued resets; positive-zero silence/tail fallback admission; Ensemble phase/head recurrence; engine BREAK scratch reads guarded by the same brk_silent_now; bounded clip arp expansion; project flush/refusal boundaries; undo alignment-independent reads; ADPCM integer bounds; original control/voice/mixer float operation order; 808 tail thresholds; 909 hoisted values and pan signed zero; master limiter and volume transitions. Focused byte comparisons support these paths; they do not establish hardware timing.

## Separate states and next consumer action

- Source identity/scope: VERIFIED.
- Focused executable QA: PASS for the recorded 26 checks; host Clang NOT_RUN.
- Test integration/proof reporting: NEEDS CHANGES (QA-02/03).
- Broad full suite and SDK V1.2.1 Elite/Pro build qualification: INCOMPLETE (QA-01); not rerun by this bounded review.
- Device cycles, 7256 cycles/sample deadline, hardware/install/release: UNMEASURED/NOT_RUN.
- No merge or publication acceptance. App RAM <=98304 and pool_free >=8192 remain required broad build gates, not verified from the narrow receipts.

**One next action, existing producer/integrator:** deliver one qualification follow-up on the existing owned lane that fixes QA-02/03 and attaches the genuine combined-candidate full-suite/Elite/Pro receipts with complete input and artifact closure (or explicitly reruns the missing proof). Preserve the historical narrow evidence. Return that exact follow-up to this same reviewer for affected recheck; the assigned integrator retains all integration decisions. Root/reviewer must not replace the producer's production source.

