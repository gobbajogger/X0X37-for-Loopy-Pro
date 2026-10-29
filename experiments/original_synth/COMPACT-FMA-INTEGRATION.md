# Exact compact FMA: inactive measured optimization

The original18-limb finite binary32 fused multiply/add remains unchanged as `original_fma_finite_bits_reference`. Public finite dispatcher first attempts compact exact integer arithmetic; unsupported alignment uses that same fallback. No unfused approximation, software double, altered coefficient, quality reduction or FP exception-flag emulation.

## Accepted compact domain

Unpack exact significands/exponents. Multiply24×24 into64 bits. A strict less-than-quarter-encoded-ulp shortcut returns the addend only when no nearest-even midpoint can be reached, including the smaller interval below a binade boundary; it does not consume ties. Remove only exact trailing zero factors from product/addend. Align only when each unsigned magnitude fits below2^62, guaranteeing addition below2^63. Otherwise fallback without output mutation.

Add/subtract magnitudes exactly, preserve result sign and cancellation+0, then perform one explicit RNE ties-even rounding to normal/subnormal binary32. Overflow produces signed infinity, gradual underflow and signed rounded zero are retained. Zero-product/zero-addend signs are handled before shortcuts. Nonfinite inputs retain existing unsupported semantics. This is not proof of original target FPCR or exception behavior.

Compact helper has no data-dependent loops, dynamic allocation or external calls in the native object; width/trailing-zero classification uses fixed bounded branches. General fallback retains its fixed18-limb arrays, high-bit/sticky scans and work bound. Domain admission does not remove worst-case fallback cost; CPU deadline proof is still required.

## Exact-input proof

52,335,750 comparisons in each normal and ASan/UBSan mode: exhaustive positive/negative cleanup inputs in[0.5,1] and all three nested FMA operands,2M finite randomized triples,4,096 edge triples. Each accepted compact result AND dispatcher result is checked against both unchanged18-limb implementation and host fmaf; rejected compact calls leave sentinel intact. Final campaign:51,506,489 compact /829,261 fallback comparisons. Earlier candidate runs and warning-only compile failure are preserved separately.

Eight affected host modes (motion and Drive/MP4, native/software FMA × normal/sanitized) pass on final source. LSan unavailable under ptrace is explicitly disabled; ASan/UBSan remain enabled. An additional exact profile output stream compares8,847,600 bytes before/after, SHA256 a312c51b16adbc1f5de7bab95ea06f7b3c0ef2c52c7725e119f402a120743989. The profile is explicit component input, NOT a complete named original preset.

## Measured hotspot and cost

Identical441,000-sample component workloads, five alternating host process-CPU runs: median full-reference/compact ratio8.893× for Miami MS20,9.306× for hot one-channel cleanup/conversion,8.872× for6,900 Chicago control updates. Unchanged oscillator/MP4 sample stages stay approximately unchanged. These are stage-specific host ratios, not device CPU percentage or a whole-engine speedup.

Separate untimed branch instrumentation on these exact operands: Miami filter441,000 compact/0 fallback; hot cleanup1,323,000 compact/0 fallback; Chicago preparation27,600 compact/0 fallback. It does not claim all presets/edits have zero fallback. Prepared four-oscillator rendering and Chicago MP4 audio sample use no FMA; Miami nonlinear table interpolation uses one/sample/voice, hot cleanup three/channel/sample. At44.1k two Miami lanes plus stereo hot cleanup imply352,800 sample-rate FMAs/s, before controls/other effects. Complete both-synth/drum/MIDI worst-case scheduler behavior is not yet integrated or timed.

Original CPU meter `firmware/src/app/main_fm1.c:render_block` measures elapsed audio ISR render/volume time over512/Fs budget (about11.61ms at44.1k), EWMA15/16. It includes wall time and nested interrupt/wait effects, not total-machine CPU. Historical70% is owner report without independently pinned live workload. Original old Miami path still uses harmonic waveform evaluations; reviewed cache optimization and this inactive original port are distinct candidates.

## Target and combined resource boundary

pi32v2 r3 compact text1294 B, dispatcher30 B, retained reference960 B, adapter46 B. Only unresolved symbol is existing memset; no software-double or64-bit division library dependency. Target instruction/call listings are retained, not measured target cycles.

Actual B application + retained inactive MP4 preparation/provider probe, using current FMA: full app417,652 B both models, exact same stock131,152 B aligned to417,664 leaves3,856 B. Compared with prior second-order probe this costs1,328 B Pro /1,312 B Elite after link alignment. Baseline loadable sections reproduce exact B. It does NOT fit combined boot(+about33,008 B), SMC updates and remaining original engine. No stock, feature, guard or sample removal to force fit. Root owns combined baseline; source activation remains held pending exact whole-app resource and audio/deadline qualification.
