# Inactive original-state Decimator2 component

Independently written portable source from sanitized original evidence. No active wobble/backend/factory/UI/engine inclusion. Not a full Crush replacement, original acoustic acceptance or device performance claim.

## Exact references and boundaries

Initial mechanism report SHA256 `1ecece582c92df0b085c0fbf344f6bb32cf76d6bfbe3ec636c223e9e9bda61f9` established phase/hold and separately shifted Q15 arithmetic. The explicit-input kernel and its initial proof were preserved when materially new state/timing data arrived.

Private closure manifest SHA256 `e5cbbe1541fb8f963d6935c9dfb599dfe60e32d63a25bc10b310db5c9ddda490` supplies all128 frequency and128 wet coefficients, original constructor/reset and invocation-local latch timing. A separate adapter consumes that closure; no guessed coefficient or global sample clock. Generator verifies manifest and both raw table hashes before atomic output. Generated functional coefficients stay PRIVATE under existing publication holds, not public source/assets.

## Explicit arithmetic kernel (unchanged after closure)

Signed16 stereo inputs; shared uint16 phase and explicit uint16 step. Compute sum in32bits, retain low16; carry bit16 captures both inputs independently, clamping captured−32768 to−32767. Dry input remains original signed16.

For explicit wet0..32767, dry=32767−wet. Per channel: `clamp(floor(held*wet/32768)+floor(input*dry/32768),−32767,32767)`. Separate shifts are mandatory. Portable negative-floor helper avoids implementation-defined signed shifts; all intermediate products fit signed32. A staged explicit step latches when the caller-supplied local index is divisible64 and dirty. Explicit seed initializes caller-chosen phase/held/step and clears pending. Seed is NOT the original-effect reset API.

Wet0/fullwet are not identity shortcuts. Gain32767 is less than unity: positive32767 produces32766; negative floors differ. Held1/input2/wet16384 produces0; shifting the combined product would produce1. Do not optimize these differences away.

## Original-state/timing adapter

Constructor: phase/held0, active step0, pending step6041, dirtytrue, Frequency integer64, Wet integer127/gain32767, Bit integer0. Empty process invocation returns without committing pending state. First nonempty call latches6041 at local0 before sample accumulation.

Original integer setters1/2/3 mask input via `(uint32_t)value &127`. Frequency looks up `frequency[127-p]`, stages step and sets dirty; Bit stores its integer and restages the frequency step. Selected process does not apply bitmask, so unused pending/active bitmask fields are deliberately omitted; this is a functional projection, not original object layout. Wet lookup is immediate, with no dirty flag,64-sample delay or ramp.

Selected-effect reset clears phase/held ONLY in this projected state. Active/pending steps, dirty, wet, integer controls remain. The original report also names auxiliary/mask fields; no unproven effect of these unused fields is implemented.

Every `adapter_process` call begins local index0, then applies the kernel at each sample. No persistent modulo64 counter. Typed caller-provided reset-frame offsets are sorted/validated before mutation and applied before that sample's latch/arithmetic. This is an independent functional event schedule, NOT a parser for original event bytes or host automation. It reproduces the reported reset ordering only. No concurrent setter/render use is supported.

Adapter buffers are disjoint in-place planar signed16 channels, a convenient portable interface rather than a claim about original wrapper ABI. State/buffers/event storage must not overlap. A valid initialization is required. Invalid API parameters/events reject without state/output mutation. Stable controls preserve output/state under partitions1..256. With changes pending, invocation boundaries intentionally reset latch timing; arbitrary partition-invariance under changed control delivery is NOT claimed. The explicit kernel separately proves invariance for unchanged supplied sample indices/events.

## Proof and host resource measurements

Run `ASAN_OPTIONS=detect_leaks=0 sh tests/host/miami_decimator_component_test.sh OUTPUT_DIR`.

Normal and ASan/UBSan each check3,768,320 arithmetic/phase oracle cases: all65536 inputs across selected wet/held edges; all32768 wet values across input/held edges; all65536 steps at five phase boundaries; separate-rounding witness. Also staged64-boundaries/latest-wins, invalid-input/output nonmutation, sentinels, explicit seed, independent stereo contexts and all partitions1..256.

Adapter tests cover256 exact table lookups, signed/masked parameters, constructor/empty/firstcall, partial reset, immediate wet, per-invocation local0, stable partitions, reset events/duplicates/order and malformed final event transactionality. The integer oracle uses independent64bit quotient/remainder floor, not production's magnitude-ceiling helper. Table test regenerates byte-identical header, verifies all transferred values by raw-table hashes and rejects a modified manifest without output replacement.

Host state sizes: explicit kernel12bytes; adapter18bytes (not original ABI). Object code/rodata and host CPU time are recorded by the runner, not native estimates. No allocation/global mutable state. Coefficient arrays cost512 read-only bytes. Prior legacy source files remain byte-identical to reviewed561daf and no active call sites reference this component; prior normal/sanitized legacy evidence therefore remains valid. No further optimization justified before review/native measurements.

## Remaining integration dependencies — precise, separate

- Original outer normalized macro/parameter quantization/smoothing, AU automation delivery, FX wrapper subdivision/transitions/crossfades, float cleanup/multiply32767/conversion/low16 wrapping and full preset context.
- Frequency/wet tables, constructor state, effect reset and invocation-local timing are NOW CLOSED for this isolated path, not outstanding requests.
- Full Crush also changes native resonance25. Original MS20 coefficient/filter implementation is a separate stage. New math inventory v2 SHA`5ac76e0d5c4589abc4950c97a9fc22effb647a1249a80f66066bb346f60a4ae1` traces coefficient recurrence/scheduling and corrected FMSUB sign; nonlinear513 table/generator and outer smoothing/modulation remain separate. Do not substitute tanh or mix filter work into this component.
- Authentic named-factory musical A/B and target CPU/dropout/RAM/build qualification remain separate. This component does not capture audio, operate hardware, replace current Crush or make a likeness claim.

Existing Mac reference worker and maintained port-map owner consume this exact result; no duplicate reference retrieval or second plan.
