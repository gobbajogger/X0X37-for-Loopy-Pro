# Original wobble combined DEV integration

Purpose: exercise the original selectable alternative through the frozen engine
foundation, with preserved stereo and an actual native fit result before exposing
selection to a musician. This is a private integration fixture, not a finished
selectable product or a claim to reproduce Miami.

## Frozen source and ownership

Foundation PR212 at `6f717e216390ffca0d20be9e2204ab401ad09792` is the
only integration base. Imported only `experiments/wobble` from prototype PR211:
remote `74167d4a1b2f2e7b961134b3c71dea4c7098de58`, local
`fb302be506e458821f02ad7e1980c767f6160ced`, identical tree
`d700cc094c022eb11936bb2d71243f3ec977fe50`. DSP/header/PolyBLEP bytes are unchanged.
The canonical trial contract hash is
`2dd8889e0843b4bbd9686cb42d3936921d7e23a4dde688bfea9107e1b8a556b3`.
One source integrator owns this branch. Independent QA and baseline native
measurement used separate scratch outputs. The separate sound-refinement owner
was not edited or consumed. A refined snapshot needs an explicit future receipt.

## Implemented boundary

`X0X_DEV_WOBBLE=1` together with `X0X_DEV_PADCAL=1` enables a boot-only fixture:
bass part A requests original WBL1/schema1 at the normal audio boundary, part B
stays303. Default builds exclude alternate object/registration. Versioned or
non-DEV builds reject the flag. No public menu, switching/control API or alternate
motion/lock interpretation exists. Old303 control keys cannot modify WBL1.

Every synth callback is implemented. Notes use explicit velocity; accent/slide do
not import303 semantics. Note-off releases; panic/stop clears sound and retains
settings. Stereo writes distinct caller buffers. The experimental mixer waits
for core2, renders/mixes each bass sequentially into existing output scratch,
preserves L/R dry audio and sends exactly `(L+R)/2` to reverb/delay/meters. This
changes overlap scheduling only in the experimental build; target timing is unproven.
Mono interface compatibility chunks through a fixed16-float/64-byte array.
No extra stereo scratch BSS, heap, libm or proprietary reference code is added.

A retained WBL1/schema1 bank is exactly16 bytes of7-bit controls. Loads validate
identity/schema/length/all values before mutation. Tests roundtrip it through the
existing explicit LXSEv1 envelope while preserving opaque legacy bytes and303
bank identity. This is an isolated codec boundary: project_t, X0XPv1, file/flash,
editor transfers and undo are unchanged. They do not persist/select this fixture.

## Host verification

Independent review: `evidence/wobble-combined/independent-review.md`.
Backend normal+ASan/UBSan checks cover fragments1..256, stereo/direct equality,
mono equality, switch ACK/saturation/back-to303 cleanup, stale identity/schema/
index rejection, note/off/panic, malformed transactional bank and LXSE roundtrip.
Default/release exclusion preprocess probes pass without running a release build.

Existing526336-sample303 checks and70509-check state codec pass. Default whole-app
WAV remains byte-identical1267756 bytes, SHA256
`a75af3fc060631d1140b3970ae7a7a492e9937a60f88f852e4e4f85f8d7f7030`.
Independent exact recovered mixer with nonzero side and reverb/delay sends matches
its dedicated-buffer reference1267756 bytes, SHA256
`db993853d8f1fd2788a4a8d22247a4a048849da7723c50ef996c481b32e94d99`.
Reproduce with `sh tests/host/run_wobble_backend.sh`,
`sh tests/host/run_synth_engine.sh`, `sh tests/host/run_synth_engine_review.sh`,
`sh tests/host/run_synth_state.sh`, then
`python3 tests/host/check_wobble_stereo_mix.py` after generated host headers.
No private WAV/reference audio is committed.

## Actual native failure and bounded recovery

Both Elite and Pro application objects compile with pinned JieLi clang4.0.1,
pi32v2/r3 single-precision FPU. DSP uses `-Oz`, same size optimization already
used by303; engine remains `-O2`. Flags/SDK/toolchain/image/object hashes are in
`evidence/wobble-combined/receipt.json` and `object-measurement.json`.

First Elite link failed by1836 XIP bytes and1828 RAM bytes. Bounded recovery
removed2048 bytes of added stereo scratch and used sequential caller buffers;
DSP size optimization reduced code. Both recovered real links still fail by716
XIP bytes. No budget/window/reserve was changed. This is **BUILD_FAILED / NO_GO**,
not BUILD_ONLY, DEV CHECKED, installable or a package result.

Unchanged-link diagnostic maps for both models: combined `.text` (including
rodata)548912, `.ram_text`3500, `.c1_text`608, `.data`368, `.bss`97700,
`.pool`335740 bytes. XIP load553388 exceeds552672. RAM address occupancy including
alignment98084 leaves220 of98304 bytes. Pool spare8324 exceeds the8192 reserve by
only132 bytes. These map observations are not a completed post-link budget check.
Soft-double/ROM/RAM-text/package checks were not reached because link failed.
No full candidate app/package/controller pairing was produced.

Unlinked exact native objects, each model: wobble text2326/rodata100/data0/bss0/
pool0. Engine text10212/rodata221168/data304/bss20016/pool213820.
Compared with foundation engine object9820/221120/304/19872/213820, aggregate
engine+wobble delta is text+2718, rodata+148, data0, bss+144, pool0 bytes.
This object delta is not a substitute for the failed full link.

Compiler assembly confirms WBL context144 bytes; callback frame100 bytes
(36 saved+64 local), DSP render frame76 (52 saved+24 local), waveform20 and
wrap4 bytes. The direct DSP call chain adds100 bytes beyond its caller; mono
compatibility adds200. Candidate engine_render frame120 bytes includes inlined
mixer code. These are static frame observations, not ISR total stack high-water,
interrupt nesting, target deadline or CPU headroom. Stack reservations are unchanged.

For reproduction inside the pinned local Linux image with toolchain/SDK mounted
read-only, run `X0X_DEV_PADCAL=1 X0X_DEV_WOBBLE=1 python3 tools/build.py --board
smk37 --model elite`, then `--model pro`. Preserve exits/logs. Run
`python3 tests/host/measure_wobble_native.py` with JIELI_TOOLCHAIN to emit unchanged
link maps and compiled section measurements. `dev_validate.py` currently strips
experimental flags: do not use it to claim alternate validation. No Actions used.

## Sound and next implementation

Host fidelity remains **FAILED**. Seven fresh pitch errors are at most0.334 cents,
but every envelope/spectrum goal fails. Pulse+7.19dB/correlation0.362 and mixed
depth+3.84dB/correlation0.832 are material mismatches. Retained carrier/square
harmonic history is not matched. X-MOD PM, CRUSH saturation, SVF/filter/allpass
are original approximations. High-depth aliasing and high-note0.12x sample-rate
pitch clamp remain limits. Integration did not repair these and does not consume
the moving refinement branch. No KORG code/assets/presets/recordings are imported;
existing MIT PolyBLEP and GPL notices remain intact.

Next single combined owner: recover at least716 bytes of real linked XIP without
relaxing gates; keep the measured narrow RAM/pool margins. First inspect linker
retained paths/duplicate adapters and exact baseline comparison, make one bounded
cost change, rerun meaningful host/independent QA and actual Elite/Pro links.
Then define the explicit versioned selection/parameter/generation and persistence
route before public controls. Hardware, publication and protected merges remain
outside this stage. A separate sound owner retains fidelity refinement.

## Release of this stage

Reviewed source commit `fdd465a8dc4aa5125b7fe3939c12fb1b29fcfc18`, tree
`a14d385b0dea09f9be03d7aa5eca495f296dcf25`; draft PR219 targets the frozen
foundation branch. Later commits contain evidence/handoff metadata only. Source
writer ownership is released at final receipt to the authorized outcome coordinator
`01a17306-999c-73b2-8e07-f1ecab31c028`. No successor was created here: the
coordinator's latest instruction owns the single creation after release.

The separate frozen-foundation full-link attempt was interrupted before compilation
was confirmed; no compiler exit/result exists for it and it was not retried. The
object delta above uses the foundation's retained actual affected-object evidence;
no full-link baseline delta is claimed. The experimental candidate's actual model
compile/link failures remain complete and independent of that missing baseline.

A newly reported refined DSP snapshot `ad206210f4909f31f992475c58d0696fb2a115fb`
(tree `ffd67d7fdc1a18aa5bfbfb084796a89201128ae5`) was not imported. Its future
consumer must independently verify the explicit receipt and source closure, update
fidelity limits, measure actual native costs, and rerun affected independent QA.
Do not infer native fit from its reported host sizes.

The isolated303 preset owner requested future engine.c hooks. No hook scope or
engine.c writer lease was released to that owner in this stage. Route that proposed
patch through the next combined integrator; do not create concurrent engine.c
writers or silently consume a moving preset branch. Additional sidecar state cannot
be assumed to fit the observed220-byte RAM margin.
