# Original Chicago and Miami firmware profile

The current `X0X_ORIGINAL_SYNTH=1` profile connects the original Chicago and
Miami runtime to two independent synth parts. It replaces the old Bass303 and
experimental Wobble DSP allocation in this profile. The compact Chicago owner
supports Bypass, Chorus, Phaser, Ring, Compressor, Decimator and Peaking EQ,
with 55 admitted Chicago programs and all 30 Miami programs. The sound-cell
schema is 2 and project revision is 13; removed legacy sound formats are not
silently migrated or reinterpreted.

The profile retains exact supplied factory/control values, explicit rounded
arithmetic, free-running clocks, effect state and note ownership. The recent
idle-work repair and semantic UI restoration are described in
[the performance note](../../docs/testing/original-idle-repair.md) and
[the UI implementation note](../../docs/engineering-delivery/original-synth-ui-restoration.md).
The historical producer notes below describe earlier component stages; they do
not override the current profile or current owner-authorized source-delivery
scope. Actual hardware CPU/deadlines and stack high-water remain unmeasured.

## Exact external data inputs

The public source includes authored implementation, interfaces, tests and
available generators. Ten generated data headers required by the active
profile are supplied separately. Two further data headers are used only by
retained alternate/reference paths. Their exact filenames, sizes, SHA256
values, generator pins and known generator gaps are recorded in
[EXTERNAL-INPUTS.json](EXTERNAL-INPUTS.json). These data payloads are not part of
the public Git tree. A public-only checkout therefore needs the exact local
inputs before it can reproduce a complete original-profile build.

Place the supplied headers at the manifest paths, then verify them:

```sh
python3 tools/check_original_inputs.py --scope active
```

Use `--scope all` when running reference tests that consume the alternate
headers. Normal host/native build preflight verifies the pinned manifest and
ten active headers before compilation. Missing or different bytes fail
explicitly. The checker does not fetch, invent or approximate replacements.
Toolchain/SDK setup remains documented in the project's BUILDING instructions.
Source delivery and a successful native build are distinct from physical-device
qualification and firmware catalogue publication.

## Historical producer record

The following retained record describes the initial reference-layer stage.
Later component integration, compact-policy, runtime, UI and performance notes
supersede its progress statements while preserving its original provenance.

### Initial integration reference layer

This is the retained Miami/Chicago producer's coherent integration lane on runtime B5b934a. It reuses independently accepted typed factory metadata, Decimator and conversion components, then joins the closed parts of the original graph. No engine/UI/audio caller is changed. It is not yet a full port or a delivered CPU improvement.

## Connected and executable here

- `fixed_fx`: original constructor/parameter16/explicit internal enable, shared stereo gain recurrence, residual persistence, reset-all callback and processed/dry Q15 wrap/mix. The explicit internal off method is not an exposed control: actual Miami reachability is unproved. Oversized calls reject before mutation; inner zero calls commit enable and invoke the effect, unlike native outer zero calls.
- `miami_fx_chain`: accepted cleanup/scaling → Decimator → its wrapper → selected Bypass plus its non-unity wrapper → FDIV. External outer invocation/reset is explicit. No algorithm switching, outer countdown policy, master mute, or hidden effect vector is inferred. Caller-sized scratch uses 12 bytes per frame, not a mandatory 2048-frame allocation.
- `control_math`: explicit helper-entry 690 counter, recovered finite VCA EG/GATE effective-control equations, pending/retire actions, ordered eight-route accumulation from an immutable previous-tick source snapshot. It is not a complete allocator, arpeggiator, VCF envelope, or event queue. Effective GATE controls must come from the engine-specific recovered context rather than bypassing the state machine.
- `miami_ms20`: exact private N513 table, finite checked original recurrence, original noise generator and nonzero epsilon noise gain, filter ramps, note reset preserving noise, and four MG shape equations. Caller must supply authentic A/B/R/cap arrays and authentic oscillator mix; external inputs are not generic substitutes. Full-note reset requires immediate coefficient preparation. MG tempo/phase advance is still a dependency.
- `master_unmuted`: only the recovered unchanged-unmuted branch with explicit initial gain, per-invocation ramp increment before the stereo sample, cleanup and forced target at block end. Original constructor/mute policies are not inferred.
- `fused_reference`: independently written finite binary32 RNE multiply-add using exact integer accumulation and one final rounding. This is a target-portable correctness baseline, not yet a real-time qualified implementation. NaN/Inf inputs are unsupported by this helper; the adapter returns quiet NaN on violation. Used only after finite checks or in recovered bounded expressions.

## Provenance and a rejected input

Consolidated v15: `955aa3e12f7d0a60b0890113fbbd27a3ae90f9dc001c049d874cb5abdc2e50af`.
Fixed wrapper: `b435224c02f12fd53652dee3816840111a09083e21d890710794c4b55bc84bd2`.
MS20 manifest: `26bffc368d7bedba4bb5cb8df17dda9e2e18e13c2216a4affc5f5987704eef98`.
N513 little-endian bits: `4880416fdae25c3fef0c3885c358f65a5f42d4470f0f6e7d8d8922a115de800c`.

The MS20 manifest's cutoff-cap entries108/109 contradict its embedded setup recipe (zero/subnormal versus expected normal endpoints). The producer rejected that array for integration and sent exact offsets back to the same original tracer. Only independently hash-checked N513 is generated here. Cap remains external. Do not repair it by guessing or silently relabel the historical manifest.

Synthetic MS20 test coefficient arrays are clearly labeled numerical fixtures. They do not represent authentic A/B/R or a musical original comparison. Native-fmaf and integer-fused builds must agree on those fixtures; real named factory musical A/B remains necessary after complete graph closure.

## Target and arithmetic limits

The pinned JieLi compiler has no `fenv.h`, and checked pinned libc/compiler_rt lacks `fmaf`. Reference conversion now has explicit build switches: `MIAMI_FX_NO_HOST_FENV` makes the environment probe return failure/unknown, never false success; `ORIGINAL_USE_SOFT_FMA` uses the exact finite reference helper. Default host behavior remains unchanged. Target arithmetic qualification, actual FP environment and real-time cost remain gates. No soft-double helper is introduced by the new target objects.

Use binary32 round-to-nearest with gradual underflow and masked exceptions; preserve explicit FMA versus separate rounded multiply/add order, and actual division. Live AU/DAW FPCR is unknown. Exception flags/traps, other rounding/default-NaN modes and unsupported nonfinite DSP state are not emulated.

All effect/state objects require valid initialization, exclusive caller ownership and the documented nonoverlapping buffers. No implicit locking or concurrent setter/render safety is claimed. Repeated finite checks prioritize correctness; optimize only from measured bounds without changing outcomes.

## Activation / persistence / delivery gate

Keep original engine identity distinct from existing experimental saved bytes; preserve the accepted legacy/current EDITED union and exact selector ordinal mapping in ordinary and P-lock paths. Existing engine has queued preset request publication with MIDI/control queue marks, held-note/wheel/lock/idle gating; per-note sound mode remains disabled in the B build. A complete original whole-note/preset snapshot must be independently reviewed before replacing bindings. No automatic migration or source-only preset relabel is admitted by this layer.

Remaining exact dependencies include actual oscillator/pitch/portamento/PW width and MG timing, corrected cap and original A/B sample-rate evidence, Chicago MP4/sync/ARP contexts, outer gating/switching, master mute and final performance/data continuity. Maintain one source owner and retained reviewer; root owns admission/ledger. Boot and SMC routing scopes remain separate. Private coefficients/assets and actual public-write holds remain protected. Source review, native objects, full model builds, musical acceptance and device CPU are distinct states.
