# Independent scanner review

Source: installed `e75fb2a584aa22978f352697a5e36fc9f886c3c5`.
Scope: read-only review and independent execution of `repo/investigation/scanner_test.py`; no device, GUI, runtime edits or PR196 branch access.

Result: PASS for the bounded host scanner properties tested. `python3 investigation/scanner_test.py` exited 0 with Clang address/undefined sanitizers; 959525 assertions. No additional production scanner defect reproduced.

- Test extracts actual `smk__analog` and `fm1_input_tick`, preserving fader field widths and arithmetic. Stubbed GPIO/ADC/mux implements the expected transaction order, so sample attribution assertions validate software slot sequencing against that model.
- 24000 tick steps use distinctive constant raw values per slot and periodic missing conversions. Current production checks conversion completion, discards unfinished conversions, and starts the next slot without dispatching stale ADC_RES. Fader indices/addresses are correct under the modeled latch semantics.
- Four faders in both directions, raw dwell 1/2/4/8/16 samples, remain monotonic for the supplied smooth complete-range ramps; decoded transitions are at most two counts in those streams. Four channels at all 1024 parked raw positions stabilize and reject one isolated full-range raw spike.
- Arithmetic is bounded: median 0..1023, scaled target 0..16368, signed quarter-step IIR remains in the same interval and fits stored uint16_t. Division truncation and the existing one-count output deadband explain quantization/lag; they are not overflow. Review found no navigation path that resets scanner history; only `smk__analog` writes these fader fields.

Limits and useful discriminator:

- Missing conversions retain old raw, median history, accumulator and decoded value until the next successful conversion. A long gap can hold a stale position and produce later catch-up. That is an observable scanner behavior, not evidence that such a gap occurred or that its current policy is defective. Correlate timestamped `adc_miss` changes, `fad_raw`, `fad_acc`, decoded fader and accepted write at the actual symptom.
- Tick test does not model electrical mux settling, ADC peripheral completion/restart behavior, SPI completion/underrun, ISR scheduling, or volatile cross-context observation. It uses constant raw values per slot, so does not establish freshness timing within a slot beyond successful-versus-missed sequencing.
- Two consecutive erroneous samples can pass a median-of-three filter, and rapid legitimate input or a sample gap can yield decoded steps larger than two counts; the <=2 result applies only to tested complete monotonic ramps. No arbitrary smoothing change is justified.
- Tests do not connect scanner output to main_fm1/UI/engine; parent owns that separate stateful path check.
- First-call initialization remains a separate main-loop question: keyboard `last[4]` initially encodes pending plus previous value zero. If its first call bypassed the <200-frame settling section, the first reading could appear to cross a target from a synthetic zero. Need actual startup timeline before declaring any defect.

Hashes at review:

`firmware/hal/smk_input.h`: SHA256 `0f7eee09e8c27a1e422d1f26e846d7b8eae767d6d4a1646a77d545decd1c79bd`

`investigation/scanner_test.py`: SHA256 `721cb97ad8cfc4bbd8e3a3678eac90a3984ccbcc912dd2007dbb132ba49168cf`
