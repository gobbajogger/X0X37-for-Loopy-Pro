# Inactive Miami FX conversion reference

This independently written component closes only the stateless float/integer boundary around the accepted inactive Decimator. It is not wired into any active backend. It does not replace the current sound or prove complete original Crush, preset, output, or CPU parity.

Reference: private sanitized `Miami-FX-float-int-boundary-findings-20261028.json`, SHA256 `146cd9684f1ebbdbd0c2d5ae2e67b01b0af434a0a6fc104da6dba5b2948f18dd`. Original core identity: `bb33879a6bad7917001d70414d558391ba6252382c54072c5f324ca8d16064ca`. Evidence is a static original trace plus independent isolated ARM instruction witnesses, not an original AU render. The parent owns the original reference and integration.

## Arithmetic contract

- Host arithmetic must be IEEE binary32, RNE ties-even with gradual underflow and masked exceptions. Build without fast math, reciprocal-math substitution, or implicit FMA contraction; explicit `fmaf` calls retain original order. The environment probe checks current rounding and input/output underflow, not exception-trap enable bits. Callers must guarantee the remaining prerequisites. Original live AU/DAW FPCR is unknown.
- Cleanup is bit-preserving for NaNs, signed zero and magnitudes at most0.5. Above0.5, the exact ordered FMA polynomial softclips to0.75 at1, with odd negative behavior. Infinities plateau. No assumed clipping to1 and no algebraically expanded replacement.
- Scaling is separate binary32 multiplication by32767. Explicit `fz=0` or `fz=1` selects the two evidenced original modes. The host itself still uses gradual underflow: FZ is emulated by bitwise flushing before/after scaling. DN0 NaN quieting preserves sign/payload through the scaled-result bit interface. No FP exception flags or traps are emulated. Other rounding/default-NaN modes are unsupported.
- FCVTZS uses integer bit decoding, handles all bit patterns, saturates signed32 overflow and mapsNaN to0. No undefined float-to-integer casts or signed overflow.
- Low16 packing is modulo narrowing followed by defined signed interpretation, not saturation. Normal cleanup/scaling bounds the reachable sample to[-24575,24575]; wrap/overflow examples are direct primitive tests, not reachable normal-chain artifacts.
- Output accepts signed16 exactly, including-32768, and divides by32767. FDIV differs from rounded-reciprocal multiplication at±513, so the latter is forbidden. No assumption that-32768 returns within[-1,1].

Bit interfaces expose numerical results only. Architectural condition flags, floating exception flags, signaling-NaN traps and host FPCR mutations are outside this portable component. No allocation, tables, DSP history, channel state, global mutable data or device access is introduced.

## Verification

`tests/host/miami_fx_conversion_vectors.py` is an independent Python exact-rational oracle. It implements binary32 RNE rounding using integer quotient/remainder and computes FMA as one exact rational operation followed by rounding. It checks all supplied ARM witness rows, then generates randomized bit patterns, neighboring float values around every reachable integer threshold and all65536 signed16 output conversions. The C test consumes those fixtures under normal and ASan/UBSan builds. It separately checks low16 wrap, invalid argument nonmutation, rounding rejection and the±513 FDIV witness.

Composition tests run cleanup/scaling/narrowing → unchanged accepted integer component → output conversion over257 invocation sizes and4096 stereo frames each, with an independent int64 Q15/phase oracle. This tests the explicit supplied schedule, not the unresolved whole-wrapper scheduler. Original private frequency/wet tables and all old source remain unchanged. Original full wrapper enable fades, FX2 wrapper, algorithm switching, post-float mastergain ramps/mute, envelopes and allocator are not included.

No target build or hardware validation is required or claimed for this inactive unit. Host size/timing evidence is not device deadline or CPU-relief proof. Independent exact-head review is required before further source delivery; current public/source holds remain.
