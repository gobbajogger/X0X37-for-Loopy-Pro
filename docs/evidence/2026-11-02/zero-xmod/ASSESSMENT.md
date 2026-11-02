> Historical pre-execution assessment. Later standalone arithmetic results are in arithmetic-results.json: those do not establish kernel/state equivalence, applicability, or performance.

# Zero-XMOD assessment packet — NOT_RUN

Scope: assessment and focused tests only; no production patch or ownership reassignment. Existing cache/triangle/test ownership remains intact.

ROADMAP read: docs/ROADMAP.md at aeeb93d6a1c6a74dd5d679bb172c775d6af98aa2, blob90ae51dc16f5c224ca300da99742bb45c5a18ee3. Items #1 / engine caching retain existing analyst and sole executor, measured attribution first; all native/headroom/sound gates remain distinct. Source baseline remains 8be16486e33f65b31c76710e1d825b6aa701c2c9, not ROADMAP commit.

The historical assessment used a sparse source closure, not a usable firmware checkout. Any future execution requires a verified exact source/input closure; this documentation packet supplies neither proprietary inputs nor a build environment.

## Equivalence argument (not executed proof)

original_oscillator_prepare in the existing oscillator_motion.c admits increments 1..INT32_MAX, finite width 0..1, shapes 0..3 and finite xmod 0..0.899999976. Prepared state is documented immutable; direct field mutation is unsupported. Coefficients come from bounded integer conversions and the small triangle coefficient; phase is int32 scaled by 2^-31. Thus all admitted k_wave values are finite, all intermediate wave calculations bounded. b=float(unsigned increment)*2^-31 is positive normal, including increment=1; INT32_MAX may round to exactly 1.0. With xmod +0 or -0, finite_wave*xmod is signed zero; adding it to positive b reproduces b exactly. The original clamp and scaling are then identical to pstep1/pstep3, including the required INT32_MAX saturation at positive 2^31 BEFORE casting. Never replace pstep with a raw increment cast: float rounding makes that wrong for some increments.

The old zero-gain branch already reuses these prepared steps; the candidate would extend this ONLY to audible even oscillators with zero xmod, leaving their audible wave computation intact. Nonzero xmod keeps all old arithmetic. No phase/noise/filter/gain advance may be skipped. The selected-voice preparation_tail path was freshly read at 8be (miami_selected_voice.c:156–173, blob74d928eca49f2708e2fe7592b61f65d54e1e6c12): it builds oc from pitch increments/current controls, calls original_oscillator_prepare whenever not ready or controls differ, and returns failure if preparation fails. This supports the normal runtime invariant; ready alone is not a full validator for arbitrary externally forged structs. Malformed internal state is not evidence for a permissive guard. If reviewers require support outside the documented prepared domain, preserve original fallback rather than silently widening this proof.

## Test-first execution sequence, once sole executor is free

No compilation, test execution or timing occurred while preparing this packet. Do not claim RED/GREEN yet. No production implementation is included. Use an existing verified source closure and command manifest to identify the actual source root; do not guess it.

1. Verify existing source/input closure and baseline blob. Run baseline versus baseline to validate fixtures (this is NOT candidate proof):

    python3 run_focused.py --source "$SRC" --candidate-kernel "$SRC/experiments/original_synth/miami_ms20.c" --out "$OUT/self-o2" --opt O2

2. Demonstrate the kernel test detects a genuine state bug BEFORE any candidate: copy only the baseline kernel to a disposable OUT file and change slave phase subtraction to addition. Run it as --candidate-kernel; EXPECT kernel_diff_test exit1 with a mismatch, not a compile/input error. Record this negative-control receipt. Delete/retain that mutant only as labeled test evidence, never as source delivery. Arithmetic literal anchors additionally reject unconditional step reuse on nonzero XMOD.
3. Applicability: run prepare_count_copy.py against the verified kernel, generating a disposable TU. Reuse the existing characterization compile manifest with ONLY that TU replaced and original source include directory retained. Count default and preset15, active/release separately where the fixture supports it. Compare the exact output PCM against pristine; count-only timings are NOT performance evidence. No benchmark timers added. Report audible_even_zero/pairs and waveform distribution; if negligible, stop this optimization direction.
4. ONLY after separate admission, original producer/scoped assignee writes the minimal zero-xmod candidate. The focused driver then compares the real unmodified reference kernel and real candidate kernel. It checks output arrays, oscillator phases, complete filter/noise/ramp state and gain state across all four shapes, width endpoints, ±0 and nonzero XMOD, increment rounding endpoints, n=0/1/2/28/63/64 and repeated transitions. Same-source fixture equality is not enough; use mutation receipt plus candidate SHA.
5. Repeat --opt O2/Os with and without --soft-fma, and --sanitize where supported. Strict -fno-fast-math and -ffp-contract=off are explicit; do not silently enable new FMA policy. Test compiler warnings/failures remain visible. Full 30-program/runtime-event/audio tests, native builds/pool and performance gates remain NOT_RUN until separately executed. This packet's structural arithmetic tests do not establish sound equality by themselves.

## Minimal candidate recommendation status

Analytical equivalence is supported for valid immutable prepared controls. Executed equivalence, fixture validation and applicability are NOT_RUN. Therefore recommend only the scoped next assessment/test execution, NOT a production change yet. After passing proof, the minimal prospective source scope is original_miami_voice_render_add in miami_ms20.c only, selecting precomputed pstep for zero XMOD while preserving wave calculation, phases and all downstream recurrence. No persistent allocation; no PR814 overlay or triangle/cache edits.

No benchmarks, device operations, source push, PR, review round or native qualification occurred. Future execution requires explicit scoped admission and existing producer handoff.
